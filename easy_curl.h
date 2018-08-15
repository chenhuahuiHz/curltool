#ifndef EASY_CURL_H_
#define EASY_CURL_H_

#include <curl/curl.h>
#include <curl/easy.h>
#include <stdarg.h>
#include <string>

class easy_curl_req_t
{
public:
    easy_curl_req_t();
    ~easy_curl_req_t();

    CURL *get_curl_handler();

    CURLcode set_url(const std::string & str_url);

    CURLcode set_connect_timeout(int timeout_ms);

    CURLcode set_timeout(int timeout_ms);

    CURLcode execute_sync();

private:
    CURL *m_curl_ptr = nullptr;
};

#endif