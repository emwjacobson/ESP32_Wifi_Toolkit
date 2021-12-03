#ifndef SOFTAP_H
#define SOFTAP_H

void softap_init();
void softap_deinit();
void softap_promiscuous_enable();
void softap_promiscuous_disable();
uint16_t softap_scan_ssids();
void softap_get_scanned_ssids(wifi_ap_record_t ap_list[], uint16_t* num_aps);
esp_err_t softap_start(const char ssid[], const char password[]);
void softap_stop();

#endif