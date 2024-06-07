#include <wifi.h>

static uint8_t retry_times = 0;

// Forward definitions
static void wifi_event(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id,void *event_data);

int wifi_init() {
    int rc;

    rc = esp_netif_init();
    if(rc != 0) return rc;

    rc = esp_event_loop_create_default();
    if(rc != 0) return rc;
    
    esp_netif_t *netif = esp_netif_create_default_wifi_sta();
    esp_netif_set_hostname(netif, "Zoey's Heart");

    wifi_init_config_t wifi_initiation = WIFI_INIT_CONFIG_DEFAULT();
    rc = esp_wifi_init(&wifi_initiation);  
    if(rc != 0) return rc;

    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event, NULL);

    return 0;
}

int wifi_connect(const char *ssid, const char *password) {
    wifi_config_t wifi_configuration = {
        .sta = {
            .ssid = "",
            .password = "",  
        }
    };

    strcpy((char*) wifi_configuration.sta.ssid, ssid);
    strcpy((char*) wifi_configuration.sta.password, password);

    esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_configuration);

    esp_wifi_start();
    esp_wifi_set_mode(WIFI_MODE_STA);

    esp_wifi_connect();

    return 0;
}

static void wifi_event(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id,void *event_data) {
    switch (event_id) {
    case WIFI_EVENT_STA_CONNECTED:
        retry_times = 0;
        return;
    
    case WIFI_EVENT_STA_DISCONNECTED:
        if(retry_times > 5) return;

        esp_wifi_connect();
        retry_times++;

    default:
        return;
    }
}