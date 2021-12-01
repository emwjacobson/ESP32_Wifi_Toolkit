#ifndef SOFTAP_H
#define SOFTAP_H

void init_ap();
esp_err_t start_ap(const char ssid[], const char password[]);
void stop_ap();
void deinit_ap();

#endif