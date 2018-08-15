#include <mutex>
#include <curl/curl.h>

#include "curl_manager.h"

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