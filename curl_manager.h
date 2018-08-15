#ifndef CURL_MANAGER_H_
#define CURL_MANAGER_H_

class curl_manager_t
{
public:
    ~curl_manager_t()
    {
        curl_global_cleanup();
    }
    static curl_manager_t *get_instance();
private:
    curl_manager_t()
    {
        curl_global_init(CURL_GLOBAL_ALL);   
    }
};

#endif
