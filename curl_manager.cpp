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
        exit(1);
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

    m_need_exit = false;
    std::thread pool_thread( [this] { this->pool_thread_loop(); } );
    pool_thread.detach();
}

void curl_manager_t::stop()
{
    m_need_exit = true;
}

void curl_manager_t::pool_thread_loop()
{
    std::cout << "pool_thread_loop start" << std::endl;

    m_time_start = get_time_stamp();

    m_running = true;
    while (!m_need_exit)
    {
        bool busy = pool_once();
        if (!busy)
            usleep(10000);
    }
    m_running = false;

    std::cout << "pool_thread_loop stop" << std::endl;
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

bool curl_manager_t::add_todo_to_doing()
{
    //std::cout << "[trace] add_todo_to_doing" << std::endl;
    // add to multi handler
    std::lock_guard<std::recursive_mutex> lock(m_todo_que_lock);
    
    while (m_curl_req_doing_map.size() < MAX_CONCURRENT_TRANS_IN_CURLMULTI  // m_curl_req_doing_map.size() is O(1)
            && !m_curl_req_todo_que.empty())
    {
        auto ptr_of_req = m_curl_req_todo_que.front();
        curl_req_t* req = ptr_of_req.get();
        m_curl_req_todo_que.pop_front();
        if (req && req->get_curl_handler())
        {
            req->attach_multi_handler(m_multi_handle);
            m_curl_req_doing_map[req->get_curl_handler()] = ptr_of_req;
        }

    }
    //std::cout << "[trace] add_todo_to_doing over" << std::endl;
    return !m_curl_req_doing_map.empty();
}

void curl_manager_t::execute_all_async()
{
    std::cout << "[trace] execute_all_async" << std::endl;

    // code according to:
    // https://curl.haxx.se/libcurl/c/curl_multi_wait.html

    int still_running = 0;
    const int CURL_SELECT_TIME = 10;    //10ms for each wait

    // break conditions: 
    const int MAX_TIME_IN_ONE_POOL = 1000;
    const int MAX_NO_FD_REPEAT_TIME = 5;

    int repeats = 0;
    int times = 0;
    do {
        CURLMcode mc;
        int numfds;
        
        mc = curl_multi_perform(m_multi_handle, &still_running);
        
        if(mc == CURLM_OK ) {
            /* wait for activity, timeout or "nothing" */
            mc = curl_multi_wait(m_multi_handle, NULL, 0, CURL_SELECT_TIME, &numfds);
        }
        
        if(mc != CURLM_OK) {
            std::cout << "[trace] execute_all_async curl_multi failed, code " << mc << std::endl;
            fprintf(stderr, "curl_multi failed, code %d.n", mc);
            break;
        }
        
        /* 'numfds' being zero means either a timeout or no file descriptors to
            wait for. Try timeout on first occurrence, then assume no file
            descriptors and no file descriptors to wait for means wait for 10
            milliseconds. */
        
        if (!numfds)
        {
            repeats++; /* count number of repeated zero numfds */

            if (repeats > MAX_NO_FD_REPEAT_TIME) {
                break;
            }
            else if (repeats > 1) {
                std::cout << "[trace] execute_all_async usleep" << std::endl;
                usleep(10000); /* sleep 10 milliseconds */
            }
        }
        else
            repeats = 0;

        times++;
    
        std::cout << "[trace] execute_all_async still_running " << still_running << std::endl;

    } while(still_running && times < MAX_TIME_IN_ONE_POOL);
    
    std::cout << "[trace] execute_all_async over" << std::endl;
}

void curl_manager_t::read_and_clean()
{
    std::cout << "[trace] read_and_clean" << std::endl;

	CURLMsg * msg = nullptr;
    //  CURLMsg define: https://curl.haxx.se/libcurl/c/curl_multi_info_read.html
    //  struct CURLMsg {
    //    CURLMSG msg;       /* what this message means */
    //    CURL *easy_handle; /* the handle it concerns */
    //    union {
    //      void *whatever;    /* message-specific data */
    //      CURLcode result;   /* return code for transfer */
    //    } data;
    //  };
    // When msg is CURLMSG_DONE, the message identifies a transfer that is done, 
    // and then result contains the return code for the easy handle that just completed.

    static int fin_count = 0; //for test

	int msg_count = 0;
	while (msg = curl_multi_info_read(m_multi_handle, &msg_count), msg)
	{
		if (msg->msg == CURLMSG_DONE) 
		{
            long rsp_code;
            long data_result;
            
			CURL *easy_handle = msg->easy_handle;
			curl_easy_getinfo(easy_handle, CURLINFO_RESPONSE_CODE,  &rsp_code);
            data_result = msg->data.result;		

            auto iter = m_curl_req_doing_map.find(easy_handle);
            if (iter != m_curl_req_doing_map.end())
            {
                curl_req_t *req_ptr = iter->second.get();
                if (req_ptr)
                {
                    req_ptr->detach_multi_handler(m_multi_handle);
                    req_ptr->rsp().set_rsp_code(rsp_code);
                    req_ptr->rsp().set_curl_code(data_result);
                }
                m_curl_req_doing_map.erase(iter);

                // todo: post resut to business thread
			    std::cout << "req:" << req_ptr->req_id() 
                    << " rsp_code:" << rsp_code 
                    << ", data_result:" << data_result 
                    << " todo.size " << m_curl_req_todo_que.size() 
                    << ", doing.size " << m_curl_req_doing_map.size() 
                    << std::endl;
                fin_count++;

                if (m_curl_req_todo_que.empty() && m_curl_req_doing_map.empty())
                {
                    m_time_over = get_time_stamp();
                    std::cout << "[trace] finish: " << fin_count 
                        << " cost time: " << m_time_over - m_time_start 
                        << std::endl;
                }
            }
            else
            {
                std::cout << "cant find curl_req_t in m_curl_req_doing_map\n";
            }
		}
	}
    std::cout << "[trace] read_and_clean over" << std::endl;
}

std::time_t curl_manager_t::get_time_stamp()
{
    std::chrono::time_point<std::chrono::system_clock,std::chrono::milliseconds> tp = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
    return std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch()).count();
}