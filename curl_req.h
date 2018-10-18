#ifndef CURL_REQ_H_
#define CURL_REQ_H_

#include <curl/curl.h>
#include <curl/easy.h>
#include <stdarg.h>
#include <string>
#include <memory>
#include <ctime>

typedef size_t(*data_slot_func_t)(void *, size_t, size_t, void *);

/*
    curl_rsp_t contains the result and data of a curl_req_t,
    could be achieved in curl_req_t objects.
*/
class curl_rsp_t
{
public:
	static size_t write_rsp_data_func(void *buffer, size_t size, size_t nmemb, void *userp);
    curl_rsp_t(){}
    ~curl_rsp_t(){}

    void set_rsp_code(long code)
    {
        m_rsp_code = code;
    }
    void set_curl_code(long code)
    {
        m_data_result = code;
    }

private:
    char *          m_buf = nullptr;  //for test, we won't write data to buf, only print it
    long            m_rsp_code;
    long            m_data_result;
};

/*
    curl_req_t defines a http req action, and gives the result after req was done.
*/
class curl_req_t
{
    typedef enum 
    {
        EN_CURL_TYPE_GET,
        EN_CURL_TYPE_POST,
    }EN_CURL_TYPE;

public:
    static std::shared_ptr<curl_req_t> new_curl_req(unsigned int req_id);
    ~curl_req_t();

    CURL *get_curl_handler();

    void make_default_opts();

    CURLcode set_url(const std::string & str_url);
    CURLcode set_connect_timeout(int timeout_ms);
    CURLcode set_timeout(int timeout_ms);
    CURLcode set_data_slot(data_slot_func_t func, void *func_handler);

    CURLcode execute_sync();

    int attach_multi_handler(CURLM *handler);
    int detach_multi_handler(CURLM *handler);

    unsigned int req_id() {return m_req_id;}

    curl_rsp_t &rsp() {return m_rsp;}

private:
    curl_req_t(unsigned int req_id);

private:
    CURL *          m_curl_ptr = nullptr;
    EN_CURL_TYPE    m_type = EN_CURL_TYPE_GET;
    unsigned int    m_req_id = 0;
    curl_rsp_t      m_rsp;

public:
    std::time_t          m_tm_start;
};

#endif