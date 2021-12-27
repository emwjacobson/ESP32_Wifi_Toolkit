#ifndef SD_CARD_H
#define SD_CARD_H

#include <stdio.h>

void sd_card_init();
void sd_card_deinit();
FILE* sd_card_open_file(char filename[]);

#endif