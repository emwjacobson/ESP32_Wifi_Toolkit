#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "softap.h"
#include "attack.h"
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
  httpd_resp_set_hdr(req, "Content-Type", "text/html");
  return httpd_resp_send(req, (const char*)page_index_start, len);
}

esp_err_t handle_styles(httpd_req_t* req) {
  ESP_LOGD(TAG, "Handling styles.css");
  const size_t len = page_styles_end - page_styles_start;
  httpd_resp_set_hdr(req, "Content-Type", "text/css");
  return httpd_resp_send(req, (const char*)page_styles_start, len);
}

esp_err_t handle_get_ssids(httpd_req_t* req) {
  ESP_LOGI(TAG, "Handling get ssids");
  uint16_t num_aps = softap_scan_ssids();
  wifi_ap_record_t ap_list[num_aps];
  softap_get_scanned_ssids(ap_list, &num_aps);

  ESP_LOGD(TAG, "Creating SSID list JSON object");
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

  ESP_LOGI(TAG, "Sending SSID JSON data");
  httpd_resp_set_hdr(req, "Content-Type", "application/json");
  char* ret = cJSON_Print(ret_obj);
  esp_err_t r = httpd_resp_send(req, ret, HTTPD_RESP_USE_STRLEN);

  ESP_LOGD(TAG, "Freeing SSID JSON data");
  free(ret);
  cJSON_Delete(ret_obj);

  return r;
}

esp_err_t handle_start_deauth(httpd_req_t* req) {
  ESP_LOGI(TAG, "Handling start deauth");
  size_t content_type_len = httpd_req_get_hdr_value_len(req, "Content-Type") + 1;
  ESP_LOGD(TAG, "Got Content-Type length %i", content_type_len);
  char content_type[content_type_len];
  httpd_req_get_hdr_value_str(req, "Content-Type", content_type, content_type_len);
  ESP_LOGI(TAG, "Got Content-Type = %s", content_type);
  if (strcmp(content_type, "application/json") != 0) {
    ESP_LOGI(TAG, "Invalid Content-Type, sending failure");
    httpd_resp_set_hdr(req, "Content-Type", "application/json");
    return httpd_resp_send(req, "{\"status\": \"failure\", \"reason\": \"Invalid Content-Type\"}", HTTPD_RESP_USE_STRLEN);
  }
  ESP_LOGD(TAG, "Got valid Content-Type");

  ESP_LOGD(TAG, "Post data length: %i", req->content_len);
  char data[req->content_len + 1];
  httpd_req_recv(req, data, req->content_len + 1);
  ESP_LOGD(TAG, "Got post data: %s", data);

  ESP_LOGD(TAG, "Parsing JSON");
  cJSON* val = cJSON_Parse(data);
  if (val == NULL) {
    return httpd_resp_send_500(req);
  }
  ESP_LOGD(TAG, "Finished parsing JSON");
  cJSON* bssid = cJSON_GetObjectItem(val, "target");
  if (bssid == NULL) {
    httpd_resp_set_hdr(req, "Content-Type", "application/json");
    return httpd_resp_send(req, "{\"status\": \"failure\", \"reason\": \"Missing target field\"}", HTTPD_RESP_USE_STRLEN);
  }

  mac_addr_t target;
  sscanf(bssid->valuestring, "%2hhx:%2hhx:%2hhx:%2hhx:%2hhx:%2hhx", &target.o1, &target.o2, &target.o3, &target.o4, &target.o5, &target.o6);
  ESP_LOGI(TAG, "Starting attack: %s", bssid->valuestring);

  cJSON_Delete(val);

  if (attack_deauth_start(target, 10)) {
    httpd_resp_set_hdr(req, "Content-Type", "application/json");
    return httpd_resp_send(req, "{\"status\": \"success\", \"reason\": \"\"}", HTTPD_RESP_USE_STRLEN);
  } else {
    httpd_resp_set_hdr(req, "Content-Type", "application/json");
    return httpd_resp_send(req, "{\"failure\": \"success\", \"reason\": \"Attack already in progress.\"}", HTTPD_RESP_USE_STRLEN);
  }
}

esp_err_t handle_stop_deauth(httpd_req_t* req) {
  ESP_LOGI(TAG, "Stopping attack");
  attack_deauth_stop();

  httpd_resp_set_hdr(req, "Content-Type", "application/json");
  return httpd_resp_send(req, "{\"status\": \"success\", \"reason\": \"\"}", HTTPD_RESP_USE_STRLEN);
}

esp_err_t handle_start_ssid_spam(httpd_req_t* req) {
  ESP_LOGI(TAG, "Handling Start SSID spam");
  size_t content_type_len = httpd_req_get_hdr_value_len(req, "Content-Type") + 1;
  ESP_LOGD(TAG, "Got Content-Type length %i", content_type_len);
  char content_type[content_type_len];
  httpd_req_get_hdr_value_str(req, "Content-Type", content_type, content_type_len);
  ESP_LOGI(TAG, "Got Content-Type = %s", content_type);
  if (strcmp(content_type, "application/json") != 0) {
    ESP_LOGI(TAG, "Invalid Content-Type, sending failure");
    httpd_resp_set_hdr(req, "Content-Type", "application/json");
    return httpd_resp_send(req, "{\"status\": \"failure\", \"reason\": \"Invalid Content-Type\"}", HTTPD_RESP_USE_STRLEN);
  }
  ESP_LOGD(TAG, "Got valid Content-Type");

  ESP_LOGD(TAG, "Post data length: %i", req->content_len);
  char data[req->content_len + 1];
  httpd_req_recv(req, data, req->content_len + 1);
  ESP_LOGD(TAG, "Got post data: %s", data);

  ESP_LOGD(TAG, "Parsing JSON");
  cJSON* val = cJSON_Parse(data);
  if (val == NULL) return httpd_resp_send_500(req);
  ESP_LOGD(TAG, "Finished parsing JSON");
  cJSON* json_ssids = cJSON_GetObjectItem(val, "ssids");
  if (json_ssids == NULL) {
    httpd_resp_set_hdr(req, "Content-Type", "application/json");
    return httpd_resp_send(req, "{\"status\": \"failure\", \"reason\": \"Missing ssids field\"}", HTTPD_RESP_USE_STRLEN);
  }

  uint8_t num_ssids = cJSON_GetArraySize(json_ssids);
  ESP_LOGI(TAG, "Got %i SSIDs", num_ssids);
  char (* ssids)[33] = malloc(sizeof *ssids * num_ssids); // BE SURE TO FREE THIS IN attack_beacon_spam_stop() !!
  memset(ssids, 0, sizeof *ssids * num_ssids);

  ESP_LOGI(TAG, "Loading SSIDs %i", num_ssids);
  for(int i=0; i<num_ssids; i++) {
    cJSON* item = cJSON_GetArrayItem(json_ssids, i);
    memcpy(ssids[i], item->valuestring, strlen(item->valuestring));
  }

  attack_beacon_spam_start(ssids, num_ssids);

  cJSON_Delete(val);

  httpd_resp_set_hdr(req, "Content-Type", "application/json");
  return httpd_resp_send(req, "{\"status\": \"success\", \"reason\": \"\"}", HTTPD_RESP_USE_STRLEN);
}

esp_err_t handle_stop_ssid_spam(httpd_req_t* req) {
  ESP_LOGI(TAG, "Handling Stop SSID spam");
  attack_beacon_spam_stop();
  return httpd_resp_send(req, "{\"status\": \"failure\", \"reason\": \"Unimplemented!\"}", HTTPD_RESP_USE_STRLEN);
}

esp_err_t get_handler(httpd_req_t* req) {
  ESP_LOGI(TAG, "Got GET URI: %s", req->uri);
  if((strcmp(req->uri, "/") == 0) || (strcmp(req->uri, "/index.html") == 0)) {
    return handle_index(req);
  } else if (strcmp(req->uri, "/styles.css") == 0) {
    return handle_styles(req);
  } else if (strcmp(req->uri, "/api/get_ssids") == 0) {
    return handle_get_ssids(req);
  } else if (strcmp(req->uri, "/api/stop_deauth") == 0) {
    return handle_stop_deauth(req);
  } else if (strcmp(req->uri, "/api/stop_ssid_spam") == 0) {
    return handle_stop_ssid_spam(req);
  } else {
    ESP_LOGI(TAG, "%s does not exist.", req->uri);
    return httpd_resp_send_404(req);
  }
}

esp_err_t post_handler(httpd_req_t* req) {
  ESP_LOGI(TAG, "Got POST URI: %s", req->uri);
  if (strcmp(req->uri, "/api/start_deauth") == 0) {
    return handle_start_deauth(req);
  } else if (strcmp(req->uri, "/api/start_ssid_spam") == 0) {
    return handle_start_ssid_spam(req);
  } else {
    ESP_LOGI(TAG, "%s does not exist.", req->uri);
    return httpd_resp_send_404(req);
  }
}

const httpd_uri_t get_uri = {
  .uri = "/*",
  .method = HTTP_GET,
  .handler = get_handler,
  .user_ctx = NULL
};

const httpd_uri_t post_uri = {
  .uri = "/*",
  .method = HTTP_POST,
  .handler = post_handler,
  .user_ctx = NULL
};

void webserver_init() {
  server = NULL;
}

void webserver_deinit() {}

void webserver_start() {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.stack_size = 4096 * 2;
  config.uri_match_fn = httpd_uri_match_wildcard;

  ESP_LOGI(TAG, "Starting HTTP server");
  ESP_ERROR_CHECK(httpd_start(&server, &config));
  ESP_ERROR_CHECK(httpd_register_uri_handler(server, &get_uri));
  ESP_ERROR_CHECK(httpd_register_uri_handler(server, &post_uri));
  ESP_LOGI(TAG, "HTTP server started");
}

void webserver_stop() {
  ESP_LOGI(TAG, "Stopping HTTP server");
  httpd_stop(server);
  server = NULL;
  ESP_LOGI(TAG, "HTTP server stopped");
}