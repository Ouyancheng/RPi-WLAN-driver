#ifndef SDHOST_SD_H 
#define SDHOST_SD_H 

#ifdef SDHCI_H
#error "SDHCI adapter is included, SDHOST adapter conflicts with the SDHCI adapter!"
#endif 

#include "sdhost/sdhost.h"

#define SD_OK                0
#define SD_TIMEOUT          -1
#define SD_ERROR            -2

#define SD_SET_SDCARD_GPIO   1 

int sdhost_sd_init(); 
int sdhost_sd_readblock(unsigned int lba, unsigned char *buffer, unsigned int num);
int sdhost_sd_writeblock(unsigned char *buffer, unsigned int lba, unsigned int num);


#endif // ! SDHOST_SD_H 

