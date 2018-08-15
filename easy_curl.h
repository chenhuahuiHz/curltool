#ifndef EASY_CURL_H_
#define EASY_CURL_H_

#include <curl/curl.h>
#include <curl/easy.h>
#include <stdarg.h>

class sync_curl_req_t
{
public:
    sync_curl_req_t()
    {
       m_curl_ptr = curl_easy_init();
    }

    CURL *get_curl_handler()
    {
        return m_curl_ptr;
    }

    CURLcode execute()
    {
        CURLcode ret = CURLE_FAILED_INIT;
        if (!m_curl_ptr)
        {
            return ret;
        }

        ret = curl_easy_perform(m_curl_ptr);
        curl_easy_cleanup(m_curl_ptr);
        return ret;
    }

private:
    CURL *m_curl_ptr = nullptr;
};

#endif