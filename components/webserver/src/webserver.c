#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_http_server.h"
#include "esp_log.h"

static const char* TAG = "Web Server";

httpd_handle_t server;

esp_err_t handler_index(httpd_req_t* req) {
  httpd_resp_send(req, "Hello World!", HTTPD_RESP_USE_STRLEN);
 return ESP_OK;
}

httpd_uri_t uri_index = {
  .uri = "/",
  .method = HTTP_GET,
  .handler = handler_index,
  .user_ctx = NULL
};

void webserver_init() {
  server = NULL;
}

void webserver_deinit() {}

void webserver_start() {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();

  ESP_LOGI(TAG, "Starting HTTP server");
  ESP_ERROR_CHECK(httpd_start(&server, &config));
  ESP_ERROR_CHECK(httpd_register_uri_handler(server, &uri_index));
  ESP_LOGI(TAG, "HTTP server started");
}

void webserver_stop() {
  ESP_LOGI(TAG, "Stopping HTTP server");
  httpd_stop(server);
  server = NULL;
  ESP_LOGI(TAG, "HTTP server stopped");
}