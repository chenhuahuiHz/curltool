
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "easy_curl.h"
#include "curl_manager.h"
FILE *fp;

// for saving curl data
// size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream)  
// {
//     int written = fwrite(ptr, size, nmemb, (FILE *)fp);
//     return written;
// }
 
int main(int argc, char *argv[])
{
    if (argc != 2)
    {
		fprintf(stderr, "usage: %s url\n", argv[0]);
		exit(-1);
    }
    
    curl_manager_t::get_instance();

    sync_curl_req_t req;
    curl_easy_setopt(req.get_curl_handler(), CURLOPT_URL, argv[1]);  
 
//     if((fp = fopen(argv[2],"w")) == NULL)
//     {
//         curl_easy_cleanup(curl);
//         exit(1);
//     }
//   //CURLOPT_WRITEFUNCTION 将后继的动作交给write_data函数处理
//     curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);

    req.execute();

    exit(0);
}