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
void attack_beacon_spam();
void attack_deauth_start(const mac_addr_t sa, uint32_t ms);
void attack_deauth_stop();

#endif