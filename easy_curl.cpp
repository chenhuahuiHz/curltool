
#include "easy_curl.h"


easy_curl_req_t::easy_curl_req_t()
{
    m_curl_ptr = curl_easy_init();
}

easy_curl_req_t::~easy_curl_req_t()
{
    curl_easy_cleanup(m_curl_ptr);
}

CURL *easy_curl_req_t::get_curl_handler()
{
    return m_curl_ptr;
}

CURLcode easy_curl_req_t::set_url(const std::string & str_url)
{
    if (!m_curl_ptr)
        return CURLE_FAILED_INIT;

    return curl_easy_setopt(m_curl_ptr, CURLOPT_URL, str_url.c_str());
}

CURLcode easy_curl_req_t::set_connect_timeout(int timeout_ms)
{
    if (!m_curl_ptr)
        return CURLE_FAILED_INIT;

    return curl_easy_setopt(m_curl_ptr, CURLOPT_CONNECTTIMEOUT_MS, timeout_ms);
}

CURLcode easy_curl_req_t::set_timeout(int timeout_ms)
{
    if (!m_curl_ptr)
        return CURLE_FAILED_INIT;

    return curl_easy_setopt(m_curl_ptr, CURLOPT_TIMEOUT_MS, timeout_ms);
}

CURLcode easy_curl_req_t::execute_sync()
{
    CURLcode ret = CURLE_FAILED_INIT;
    if (!m_curl_ptr)
    {
        return ret;
    }
    
    return curl_easy_perform(m_curl_ptr);
}