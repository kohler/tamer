#ifndef HTTP_H
#define HTTP_H

#include "input.hh"
#include <tamer.hh>


typedef enum 
{
    HTTP_VERSION_1_0,
    HTTP_VERSION_1_1,
} http_version;


struct http_request
{
    char url[80];
    http_version version;
    int socket;
    int closed;
};

void http_init(http_request *req, int socket);

void http_parse(http_request *req, tamer::event<int> ev);

#endif
