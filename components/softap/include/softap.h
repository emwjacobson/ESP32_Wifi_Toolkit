#ifndef SOFTAP_H
#define SOFTAP_H

void softap_init();
void softap_deinit();
void softap_promiscuous_enable();
void softap_promiscuous_disable();
esp_err_t softap_start(const char ssid[], const char password[]);
void softap_stop();

#endif