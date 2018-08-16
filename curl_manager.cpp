#include <curl/curl.h>
#include <iostream>
#include <sstream>
#include <thread>
#include <unistd.h>

#include "curl_req.h"
#include "curl_manager.h"

const long MAX_CONCURRENT_TRANS_IN_CURLMULTI = 1000;

static curl_manager_t * sg_instance = nullptr;
static std::mutex sg_instance_lock;
curl_manager_t * curl_manager_t::get_instance()
{
    if (sg_instance == nullptr)
    {
        std::lock_guard<std::mutex> lock(sg_instance_lock);
        if (sg_instance == nullptr)
        {
            sg_instance = new curl_manager_t();
        }
    }
    return sg_instance;
}

curl_manager_t::curl_manager_t()
{
    curl_global_init(CURL_GLOBAL_ALL);
    m_multi_handle = curl_multi_init();
    if (m_multi_handle == nullptr)
    {
        std::cout << "curl_multi_init error!" << std::endl;
        return;
    }

	curl_multi_setopt(m_multi_handle, CURLMOPT_MAXCONNECTS, MAX_CONCURRENT_TRANS_IN_CURLMULTI);
}

curl_manager_t::~curl_manager_t()
{
    curl_multi_cleanup(m_multi_handle);
    curl_global_cleanup();
}

void curl_manager_t::start()
{
    if (m_running)
        return;

    m_running = true;
    std::thread pool_thread( [this] { this->pool_thread_loop(); } );
    pool_thread.detach();
}

void curl_manager_t::stop()
{
    m_running = false;
}

void curl_manager_t::pool_thread_loop()
{
    std::cout << "pool_thread_loop start" << std::endl;
    while (m_running)
    {
        bool busy = pool_once();
        if (!busy)
            usleep(10000);
    }

    std::cout << "pool_thread_loop stop" << std::endl;
}

bool curl_manager_t::add_todo_to_doing()
{
    // add to multi handler
    std::lock_guard<std::recursive_mutex> lock(m_todo_que_lock);
    while (m_curl_req_doing_map.size() < MAX_CONCURRENT_TRANS_IN_CURLMULTI
            && !m_curl_req_todo_que.empty())
    {
        auto ptr_of_req = m_curl_req_todo_que.front();
        curl_req_t* req = ptr_of_req.get();
        m_curl_req_todo_que.pop_front();
        if (req)
        {
            req->attach_multi_handler(m_multi_handle);
            m_curl_req_doing_map[req->get_curl_handler()] = ptr_of_req;
        }

    }
    return !m_curl_req_doing_map.empty();
}

void curl_manager_t::execute_all_async()
{
#if 1
    // triggle send reqs
    int still_running = 0;
    const int CURL_SELECT_TIME = 10;
    curl_multi_perform(m_multi_handle, &still_running);
    do {
        int numfds = 0;
        int res = curl_multi_wait(m_multi_handle, NULL, 0, CURL_SELECT_TIME, &numfds);
        if (res != CURLM_OK) {
            std::cout << "error: curl_multi_wait() returned" << res << std::endl;
            return;
        }
        curl_multi_perform(m_multi_handle, &still_running);
    } while (still_running);

#else
    // this func is from libcurl doc example
    int still_running = 0;
    const int CURL_SELECT_TIME = 10;
    int repeats = 0;
    do {
        CURLMcode mc;
        int numfds;
        
        mc = curl_multi_perform(m_multi_handle, &still_running);
        
        if(mc == CURLM_OK ) {
            /* wait for activity, timeout or "nothing" */
            mc = curl_multi_wait(m_multi_handle, NULL, 0, CURL_SELECT_TIME, &numfds);
        }
        
        if(mc != CURLM_OK) {
            fprintf(stderr, "curl_multi failed, code %d.n", mc);
            break;
        }
        
        /* 'numfds' being zero means either a timeout or no file descriptors to
            wait for. Try timeout on first occurrence, then assume no file
            descriptors and no file descriptors to wait for means wait for 100
            milliseconds. */
        
        if (!numfds)
        {
            repeats++; /* count number of repeated zero numfds */
            if(repeats > 1) {
                usleep(10000); /* sleep 10 milliseconds */
            }
        }
        else
            repeats = 0;
    
    } while(still_running);
#endif
}

void curl_manager_t::read_and_clean()
{
	CURLMsg * msg = nullptr;
	int msg_count = 0;
	while (msg = curl_multi_info_read(m_multi_handle, &msg_count), msg)
	{
		if (msg->msg == CURLMSG_DONE) 
		{
            long rsp_code;
			std::string str_rsp_code;
			std::string str_curl_code;
			CURL *easy_handle = msg->easy_handle;
			curl_easy_getinfo(easy_handle, CURLINFO_RESPONSE_CODE,  &rsp_code);
				 
			std::stringstream out,outCurl;
			out << rsp_code;		
			str_rsp_code = out.str();

			outCurl << msg->data.result;
			str_curl_code = outCurl.str();
			

            auto iter = m_curl_req_doing_map.find(easy_handle);
            if (iter != m_curl_req_doing_map.end())
            {
                curl_req_t *req_ptr = iter->second.get();
                if (req_ptr)
                {
                    req_ptr->detach_multi_handler(m_multi_handle);
                    req_ptr->rsp().set_rsp_code(str_rsp_code);
                    req_ptr->rsp().set_curl_code(str_curl_code);
                }
                m_curl_req_doing_map.erase(iter);
			    std::cout << "req:" << req_ptr->req_id() << " ResCode:"<<str_rsp_code <<", curlRspCode:"<<str_curl_code << std::endl;
                std::cout << "m_curl_req_todo_que.size = " << m_curl_req_todo_que.size() << std::endl;
                std::cout << "m_curl_req_doing_map.size = " << m_curl_req_doing_map.size() << std::endl;
            }
            else
            {
                std::cout << "cant find curl_req_t in m_curl_req_doing_map\n";
            }
		}
	}
}

bool curl_manager_t::pool_once()
{
    if (add_todo_to_doing())
    {
        execute_all_async();
        read_and_clean();
    }
    return !m_curl_req_doing_map.empty();
}

size_t curl_manager_t::push_curl_req(std::shared_ptr<curl_req_t> req)
{
    // do we need limit todo-list size ?
    std::lock_guard<std::recursive_mutex> lock(m_todo_que_lock);
    m_curl_req_todo_que.push_back(req);
    return m_curl_req_todo_que.size();
}