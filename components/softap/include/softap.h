#ifndef SOFTAP_H
#define SOFTAP_H

void softap_init();
esp_err_t softap_start(const char ssid[], const char password[]);
void softap_stop();
void softap_deinit();

#endif