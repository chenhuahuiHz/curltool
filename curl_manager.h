#ifndef CURL_MANAGER_H_
#define CURL_MANAGER_H_

class curl_manager_t
{
public:
    ~curl_manager_t(){}
    static curl_manager_t *get_instance();
private:
    curl_manager_t(){}
};

#endif
