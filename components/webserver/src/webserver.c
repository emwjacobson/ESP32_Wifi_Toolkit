#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "softap.h"
#include "cJSON.h"

static const char* TAG = "Web Server";

extern const uint8_t page_index_start[] asm("_binary_index_html_start");
extern const uint8_t page_index_end[] asm("_binary_index_html_end");
extern const uint8_t page_styles_start[] asm("_binary_styles_css_start");
extern const uint8_t page_styles_end[] asm("_binary_styles_css_end");

httpd_handle_t server;

esp_err_t handle_index(httpd_req_t* req) {
  ESP_LOGD(TAG, "Handling index.html");
  const size_t len = page_index_end - page_index_start;
  return httpd_resp_send(req, (const char*)page_index_start, len);
}

esp_err_t handle_styles(httpd_req_t* req) {
  ESP_LOGD(TAG, "Handling styles.css");
  const size_t len = page_styles_end - page_styles_start;
  httpd_resp_set_hdr(req, "Content-Type", "text/css");
  return httpd_resp_send(req, (const char*)page_styles_start, len);
}

esp_err_t handle_get_ssids(httpd_req_t* req) {
  uint16_t num_aps = softap_scan_ssids();
  wifi_ap_record_t ap_list[num_aps];
  softap_get_scanned_ssids(ap_list, &num_aps);

  cJSON* ret_obj = cJSON_CreateObject();
  cJSON_AddNumberToObject(ret_obj, "num_aps", num_aps);
  cJSON* aps = cJSON_AddArrayToObject(ret_obj, "aps");

  cJSON* ap = NULL;
  char bssid[18];
  for(int i=0; i<num_aps; i++) {
    ap = cJSON_CreateObject();
    cJSON_AddStringToObject(ap, "ssid", (char*)ap_list[i].ssid);
    cJSON_AddNumberToObject(ap, "channel", ap_list[i].primary);
    sprintf(bssid, "%02x:%02x:%02x:%02x:%02x:%02x",
          ap_list[i].bssid[0],
          ap_list[i].bssid[1],
          ap_list[i].bssid[2],
          ap_list[i].bssid[3],
          ap_list[i].bssid[4],
          ap_list[i].bssid[5]);
    cJSON_AddStringToObject(ap, "bssid", bssid);
    switch(ap_list[i].authmode) {
      case WIFI_AUTH_OPEN:
        cJSON_AddStringToObject(ap, "authmode", "Open");
        break;
      case WIFI_AUTH_WEP:
        cJSON_AddStringToObject(ap, "authmode", "WEP");
        break;
      case WIFI_AUTH_WPA_PSK:
        cJSON_AddStringToObject(ap, "authmode", "WPA");
        break;
      case WIFI_AUTH_WPA2_PSK:
        cJSON_AddStringToObject(ap, "authmode", "WPA2");
        break;
      case WIFI_AUTH_WPA_WPA2_PSK:
        cJSON_AddStringToObject(ap, "authmode", "WPA/WPA2");
        break;
      case WIFI_AUTH_WPA2_ENTERPRISE:
        cJSON_AddStringToObject(ap, "authmode", "WPA2-E");
        break;
      case WIFI_AUTH_WPA3_PSK:
        cJSON_AddStringToObject(ap, "authmode", "WPA3");
        break;
      case WIFI_AUTH_WPA2_WPA3_PSK:
        cJSON_AddStringToObject(ap, "authmode", "WPA2/WPA3");
        break;
      case WIFI_AUTH_WAPI_PSK:
        cJSON_AddStringToObject(ap, "authmode", "WAPI");
        break;
      case WIFI_AUTH_MAX:
        cJSON_AddStringToObject(ap, "authmode", "MAX");
        break;
    }
    cJSON_AddItemToArray(aps, ap);
  }

  httpd_resp_set_hdr(req, "Content-Type", "application/json");
  char* ret = cJSON_Print(ret_obj);
  esp_err_t r = httpd_resp_send(req, ret, HTTPD_RESP_USE_STRLEN);

  free(ret);
  cJSON_Delete(ret_obj);

  return r;
}

esp_err_t get_handler(httpd_req_t* req) {
  ESP_LOGI(TAG, "Got URI: %s", req->uri);
  if((strcmp(req->uri, "/") == 0) || (strcmp(req->uri, "/index.html") == 0)) {
    return handle_index(req);
  } else if (strcmp(req->uri, "/styles.css") == 0) {
    return handle_styles(req);
  } else if (strcmp(req->uri, "/api/get_ssids") == 0) {
    return handle_get_ssids(req);
  } else {
    ESP_LOGI(TAG, "%s does not exist.", req->uri);
    return httpd_resp_send_404(req);
  }
}

httpd_uri_t get_uri = {
  .uri = "/*",
  .method = HTTP_GET,
  .handler = get_handler,
  .user_ctx = NULL
};

void webserver_init() {
  server = NULL;
}

void webserver_deinit() {}

void webserver_start() {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.uri_match_fn = httpd_uri_match_wildcard;

  ESP_LOGI(TAG, "Starting HTTP server");
  ESP_ERROR_CHECK(httpd_start(&server, &config));
  ESP_ERROR_CHECK(httpd_register_uri_handler(server, &get_uri));
  ESP_LOGI(TAG, "HTTP server started");
}

void webserver_stop() {
  ESP_LOGI(TAG, "Stopping HTTP server");
  httpd_stop(server);
  server = NULL;
  ESP_LOGI(TAG, "HTTP server stopped");
}