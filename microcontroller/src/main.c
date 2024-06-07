#include <stdio.h>
#include <nvs_flash.h>
#include <setup.h>
#include <wifi.h>
#include <httpserver.h>

void app_main() {
    // Initialize NVS.
    int ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    wifi_init();

    if(setup_should_run()) {
        setup_run();
    }

    httpserver_start();

    while(true) {
        sleep(5);
    }
}