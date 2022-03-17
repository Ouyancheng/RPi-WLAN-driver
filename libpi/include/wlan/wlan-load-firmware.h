#ifndef WLAN_LOAD_FIRMWARE_H 
#define WLAN_LOAD_FIRMWARE_H 

#include "rpi.h"

#include "wlan-backplane.h"
#define HAS_FAT32 0  // if you have FAT32 code, that doesn't help though... 
#if HAS_FAT32
#include "fat32/pi-sd.h"
#include "fat32/fat32.h"
#endif 

void get_firmware(const unsigned char **firmware, unsigned *firmware_size); 
void get_nvram(const uint8_t **nvram_buffer, unsigned *nvram_size); 

#if HAS_FAT32
int load_file_from_root_dir(const char *filename, uint8_t *buffer, uint32_t *filesize); 
#endif 
/**
 * Given an representation of NVRAM content in buffer, with size filesize, 
 * this function condenses the NVRAM content *inplace* and writes to the condensed file size, 
 * 
 * Condense rules: 
 * - remove comments (starting with '#') and blank lines
 * - remove carriage returns ('\r')
 * - convert newlines to nulls ('\n' -> '\0')
 * - mark end with two nulls ("\0\0")
 * - pad with nulls to multiple of 4 bytes total length
 * @return 0 for success, < 0 for failure 
 */
int condense_nvram(
    const uint8_t *input_buffer, 
    const uint32_t filesize, 
    uint8_t *output_buffer, 
    uint32_t *condensed_file_size
); 

int dump_nvram(const uint8_t *buffer, const uint32_t filesize); 

int upload_data(uint32_t base_addr, const unsigned char *data, uint32_t len); 
int verify_upload_data(uint32_t base_addr, const unsigned char *data, uint32_t len); 

int firmware_upload(); 
#endif 


