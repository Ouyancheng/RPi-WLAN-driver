#include "wlan/wlan-load-firmware.h"
const uint8_t nvram[] = 
"# NVRAM file for BCM943430WLSELG\n# 2.4 GHz, 20 MHz BW mode\n\n# The following parameter values are just placeholders, need to be updated.\nmanfid=0x2d0\nprodid=0x0726\nvendid=0x14e4\ndevid=0x43e2\nboardtype=0x0726\nboardrev=0x1202\nboardnum=22\nmacaddr=00:90:4c:c5:12:38\nsromrev=11\nboardflags=0x00404201\nboardflags3=0x08000000\nxtalfreq=37400\n#xtalfreq=19200\nnocrc=1\nag0=255\naa2g=1\nccode=ALL\n\npa0itssit=0x20\nextpagain2g=0\n\n#PA parameters for 2.4GHz, measured at CHIP OUTPUT\npa2ga0=-168,7161,-820\nAvVmid_c0=0x0,0xc8\ncckpwroffset0=5\n\n# PPR params\nmaxp2ga0=84\ntxpwrbckof=6\ncckbw202gpo=0\nlegofdmbw202gpo=0x66111111\nmcsbw202gpo=0x77711111\npropbw202gpo=0xdd\n\n# OFDM IIR :\nofdmdigfilttype=18\nofdmdigfilttypebe=18\n# PAPD mode:\npapdmode=1\npapdvalidtest=1\npacalidx2g=32\npapdepsoffset=-36\npapdendidx=61\n\nil0macaddr=00:90:4c:c5:12:38\nwl0id=0x431b\n\ndeadman_to=0xffffffff\n# muxenab: 0x1 for UART enable, 0x2 for GPIOs, 0x8 for JTAG\nmuxenab=0x1\n# CLDO PWM voltage settings - 0x4 - 1.1 volt\n#cldo_pwm=0x4\n\n#VCO freq 326.4MHz\nspurconfig=0x3 \n\n# Improved Bluetooth coexistence parameters from Cypress\nbtc_mode=1\nbtc_params8=0x4e20\nbtc_params1=0x7530\n\n"; 

void get_nvram(const uint8_t **nvram_buffer, unsigned *nvram_size) {
    (*nvram_buffer) = (const uint8_t*)nvram; 
    (*nvram_size) = (unsigned)sizeof(nvram); 
}
#if HAS_FAT32
#define PRINT_FILES 0 
int load_file_from_root_dir(const char *filename, uint8_t *buffer, uint32_t *filesize) {
    kmalloc_init();
    pi_sd_init(); 
    mbr_t *mbr = mbr_read(); 
    mbr_partition_ent_t partition;
    memcpy(&partition, mbr->part_tab1, sizeof(mbr_partition_ent_t));
    assert(mbr_part_is_fat32(partition.part_type));
    fat32_fs_t fs = fat32_mk(&partition);
    pi_dirent_t root = fat32_get_root(&fs);
    pi_directory_t files = fat32_readdir(&fs, &root);
    pi_dirent_t *file_entry = fat32_stat(&fs, &root, (char*)filename);
    demand(file_entry, "file not found!\n"); 
    pi_file_t *file = fat32_read(&fs, &root, (char*)filename);
#if PRINT_FILES 
    printk("Printing %s (%d bytes):\n", filename, file->n_data);
    printk("--------------------\n");
    for (int i = 0; i < file->n_data; i++) {
        printk("%c", file->data[i]);
    }
    printk("--------------------\n");
#endif 
    memcpy(buffer, file->data, file->n_data); 
    (*filesize) = file->n_data; 
    kfree_all(); 
    return 0; 
}
#endif
// Condense rules: 
// - remove comments (starting with '#') and blank lines
// - remove carriage returns ('\r')
// - convert newlines to nulls ('\n' -> '\0')
// - mark end with two nulls ("\0\0")
// - pad with nulls to multiple of 4 bytes total length
int condense_nvram(
    const uint8_t *input_buffer, 
    const uint32_t filesize, 
    uint8_t *output_buffer, 
    uint32_t *condensed_file_size
) {
    uint32_t output_index = 0; 
    uint32_t i = 0; 
    while (i < filesize) {
        uint8_t c = input_buffer[i]; 
        switch (c) {
        case '\r':
            // skip CR 
            break; 
        case '\n': 
            // replace LF with NUL 
            // no two consecutive '\0' 
            if (output_index == 0 || output_buffer[output_index-1] == '\0') {
                break; 
            } else {
                output_buffer[output_index] = '\0'; 
                output_index += 1; 
            }
            break; 
        case '#': {
            // skip the comments 
            i += 1; 
            while (input_buffer[i] != '\n') { 
                i += 1; 
            }
            break; 
        } 
        default: 
            output_buffer[output_index] = c; 
            output_index += 1; 
            break; 
        }
        i += 1; 
    }
    if (output_index == 0) {
        printk("config file is empty? What's happening?\n"); 
        return -1; 
    }
    if (output_buffer[output_index - 1] != '\0') {
        output_buffer[output_index] = '\0'; 
        output_index += 1; 
    }
    output_buffer[output_index] = '\0';
    output_index += 1; 
    while ((output_index % 4) != 0) {
        output_buffer[output_index] = '\0';
        output_index += 1; 
    }
    (*condensed_file_size) = output_index; 
    return 0; 
}

int dump_nvram(const uint8_t *buffer, const uint32_t filesize) {
    for (uint32_t i = 0; i < filesize; ++i) {
        uint8_t c = buffer[i]; 
        switch (c) {
        case '\r':
            printk("\\r"); // <CR>
            break;
        case '\n': 
            printk("\\n"); // <LF>
            break; 
        case '\0': 
            printk("\\0"); // <NUL>
            break; 
        case '\t': 
            printk("\\t"); // <TAB>
            break; 
        case '\v':
            printk("\\v"); // <VTAB>
            break; 
        case '\f': 
            printk("\\f"); // <FEED>
            break; 
        default:
            printk("%c", c); 
            break;
        }
    }
    return 0; 
}

// base_addr = ctl->rambase for firmware 
int upload_data(uint32_t base_addr, const unsigned char *data, uint32_t len) {
    assert((len % 4) == 0);  // must be a multiple of 4
    uint32_t addr_offset = 0; 
    const uint32_t upload_block_size = 2048; 
    uint32_t upload_size = 0; 
    while (1) {
        if (len == 0) {
            break; 
        }
        if (len < upload_block_size) {
            upload_size = len; 
        } else {
            upload_size = upload_block_size; 
        }
        backplane_perform_io(1, 
            base_addr + addr_offset, 
            (uint8_t*)data, 
            upload_size
        ); 
        addr_offset += upload_size; 
        data += upload_size; 
        len -= upload_size; 
    }
    return 0; 
}

int verify_upload_data(uint32_t base_addr, const unsigned char *data, uint32_t len) {
    assert((len % 4) == 0);  // must be a multiple of 4 
    uint32_t addr_offset = 0; 
    const uint32_t download_block_size = 512; 
    uint32_t download_size = 0; 
    uint8_t buffer[512];
    while (1) {
        if (len == 0) {
            break; 
        }
        if (len < download_block_size) {
            download_size = len; 
        } else {
            download_size = download_block_size; 
        }
        backplane_perform_io(0, 
            base_addr + addr_offset, 
            buffer, 
            download_size
        );
        if (memcmp(buffer, data, download_size) != 0) {
            printk("verification failed at addr_offset=%x, download_size=%x\n", 
                addr_offset, 
                download_size
            ); 
            return -1; 
        } 
        data += download_size; 
        addr_offset += download_size; 
        len -= download_size; 
    }
    return 0; 
}
uint8_t condensed_nvram_buffer[1024]; 
uint32_t condensed_nvram_size; 
int firmware_upload() {
    // load the firmware and NVRAM 
    const unsigned char *firmware_buffer = NULL; 
    unsigned firmware_size = 0; 
    get_firmware(&firmware_buffer, &firmware_size); 
    const uint8_t *nvram_buffer = NULL; 
    unsigned nvram_size = 0; 
    get_nvram(&nvram_buffer, &nvram_size); 
    assert(firmware_size > 8 && nvram_size > 8 && firmware_buffer != NULL && nvram_buffer != NULL); 
    // condense the NVRAM 
    condense_nvram(nvram_buffer, nvram_size, condensed_nvram_buffer, &condensed_nvram_size); 
    assert(condensed_nvram_buffer != NULL && condensed_nvram_size > 0); 
    // enable the active low-power clock, put the card into standby mode 
    const uint32_t request_ALP = 0x08; // active low-power clock!! 
    backplane_write(BACKPLANE_CLK_CSR, request_ALP); 
    const uint32_t ALP_available = 0x40; 
    while ((backplane_read(BACKPLANE_CLK_CSR) & ALP_available) == 0) {
        delay_ms(10); 
    }
    // write the size of the nvram file, it's 0 first 
    uint8_t buffer[4]; 
    memset(buffer, 0, 4); 
    backplane_perform_io(1, ctl->rambase + ctl->socramsize - 4, buffer, 4); 

    upload_data(ctl->rambase, firmware_buffer, firmware_size); 

    delay_ms(100); 

    upload_data(ctl->rambase + ctl->socramsize - condensed_nvram_size - 4, condensed_nvram_buffer, condensed_nvram_size); 

    // put the size of the nvram file, for size = 680, n = 680/4 = 170 = 0xAA 
    // then the size to write is (in little endian) 0xAA 0x00 0x55 0xFF 
    uint32_t n = condensed_nvram_size / 4; 
    n = (n & 0xFFFF) | (~n << 16);  // in little endian 0xAA 0x00 0x55 0xFF 

    serialize_int32(buffer, n); 
    assert(buffer[0] == 0xAA); 
    assert(buffer[1] == 0x00); 
    assert(buffer[2] == 0x55); 
    assert(buffer[3] == 0xFF); 
    backplane_perform_io(1, ctl->rambase+ctl->socramsize-4, buffer, 4); 
    // verify 
    delay_ms(10); 
    assert(verify_upload_data(ctl->rambase, firmware_buffer, firmware_size) == 0);
    delay_ms(10); 
    assert(verify_upload_data(ctl->rambase+ctl->socramsize-condensed_nvram_size-4, condensed_nvram_buffer, condensed_nvram_size) == 0); 
    delay_ms(10); 
    assert(verify_upload_data(ctl->rambase+ctl->socramsize-4, buffer, 4) == 0);  
    delay_ms(10); 
    backplane_reset(ctl->armctl, 0, 0); 
    return 0; 
}
