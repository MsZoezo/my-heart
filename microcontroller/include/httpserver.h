#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include <http_parser.h>
#include <esp_http_server.h>

int httpserver_start();
int httpserver_stop();

#endif