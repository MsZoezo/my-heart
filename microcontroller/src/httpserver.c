#include <httpserver.h>

static httpd_handle_t server = NULL;

// Forward definitions.
esp_err_t get_handler(httpd_req_t *req);

httpd_uri_t uri_get = {
    .uri      = "/test",
    .method   = HTTP_GET,
    .handler  = get_handler,
    .user_ctx = NULL
};

int httpserver_start() {
    if(server) return 1;

    int rc;

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;

    rc = httpd_start(&server, &config);
    if(rc != 0) return rc;

    httpd_register_uri_handler(server, &uri_get);

    return 0;
}

int httpserver_stop() {
    if(!server) return 1;

    httpd_stop(server);
    server = NULL;

    return 0;
}

esp_err_t get_handler(httpd_req_t *req) {
    const char resp[] = "Hello World!";
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}