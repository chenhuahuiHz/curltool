#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>

#include "curl_req.h"
#include "curl_manager.h"
 
int main(int argc, char *argv[])
{
    if (argc != 2)
    {
		fprintf(stderr, "usage: %s url\n", argv[0]);
		exit(-1);
    }

    unsigned int reqid = 0;
    try
    {
        // start the curl pool thread
        curl_manager_t::get_instance()->start();

        // add tasks
        for (unsigned int i = 0; i < 1; i++)
        {
            auto req = curl_req_t::new_curl_req(reqid++);
            req.get()->set_url(argv[1]);
            req.get()->make_default_opts();
            curl_manager_t::get_instance()->push_curl_req(req);
        }        
    }
    catch(std::exception& e)
    {
        std::cout << "exception:" << e.what() << std::endl;
        exit(1);
    }

    // nothing in main thread
    //int addcnt = 0;
    while(1)
    {
        sleep(1);
        // if (addcnt < 10) 
        // {
        //     // add tasks
        //     for (unsigned int i = 0; i < 200; i++)
        //     {
        //         auto req = curl_req_t::new_curl_req(reqid++);
        //         req.get()->set_url(argv[1]);
        //         req.get()->make_default_opts();
        //         curl_manager_t::get_instance()->push_curl_req(req);
        //     } 
        //     addcnt++;
        // }    
    }

    exit(0);
}