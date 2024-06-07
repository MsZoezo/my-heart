#ifndef WIFI_H
#define WIFI_H

#include <stdio.h>
#include <string.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <lwip/err.h>
#include <lwip/sys.h>

int wifi_init();
int wifi_connect(const char *ssid, const char *password);

#endif