#ifndef ATTACK_H
#define ATTACK_H

typedef struct {
	uint8_t o1;
	uint8_t o2;
	uint8_t o3;
	uint8_t o4;
	uint8_t o5;
	uint8_t o6;
} mac_addr_t;

void attack_init();
void attack_deinit();
void attack_beacon_spam_start(char (* ssids)[33], uint8_t num);
void attack_beacon_spam_stop();
bool attack_deauth_start(mac_addr_t sa, uint32_t ms);
void attack_deauth_stop();
void attack_ip_scan_start(ip_addr_t ip, uint8_t cidr);
bool attack_ip_scan_in_progress();
void attack_ip_scan_stop();


#endif