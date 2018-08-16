#ifndef CURL_MANAGER_H_
#define CURL_MANAGER_H_

#include <mutex>
#include <deque>
#include <memory>
#include <unordered_map>

class curl_req_t;
class curl_manager_t
{
public:
    typedef std::deque<std::shared_ptr<curl_req_t> > curl_req_que_t;
    typedef std::unordered_map<void *, std::shared_ptr<curl_req_t> > curl_req_map_t;
    
    ~curl_manager_t();

    static curl_manager_t *get_instance();

    void start();

    void stop();

    void pool_thread_loop();

    size_t push_curl_req(std::shared_ptr<curl_req_t> req);

private:
    curl_manager_t();

    bool add_todo_to_doing(); //if any req being executing, return true, else false
    void execute_all_async();
    void read_and_clean();
    
    bool pool_once(); //if any req being executing, return true, else false

private:
    curl_req_que_t  m_curl_req_todo_que;       // reqs waiting to be executed
    curl_req_map_t  m_curl_req_doing_map;      // reqs being executed
	CURLM  *        m_multi_handle = nullptr;

    std::recursive_mutex    m_todo_que_lock;
    bool                    m_running = false;
};

#endif
