#include "wlan/wlan-sdio.h" 

struct SDIOState sdio_state; 

int sdio_cmd(uint32_t cmd, uint32_t arg) {
    uint32_t resp[4]; 
    int success = emmc_send_command(cmd, arg, resp); 
    if (success != EMMC_R_OK) {
        emmc_dump_regs("sdio command failed"); 
        return -1; 
    }
    return resp[0]; 
}

int sdio_read(uint32_t fn, uint32_t addr) {
    int resp = sdio_cmd(EMMC_CMD_IO_RW_DIRECT, cmd52_args_assemble(0, fn, 0, addr, 0)); 
    if (resp & 0xCF00) {
        printk("sdio: sdio_read(%x, %x) fail: %x %x\n", fn, addr, (resp>>8)&0xFF, resp&0xFF);
        return 0; 
    }
    return resp & 0xFF; 
}
int sdio_write(uint32_t fn, uint32_t addr, uint32_t data) {
    int resp = 0; 
    for (int i = 0; i < SDIO_WRITE_RETRY; ++i) {
        resp = sdio_cmd(EMMC_CMD_IO_RW_DIRECT, cmd52_args_assemble(1, fn, 0, addr, data)); 
        if ((resp & 0xCF00) == 0) {
            return SDIO_R_OK; 
        }
    } 
    printk("sdio: sdio_write(%x, %x, %x) fail: %x %x\n", fn, addr, data, (resp>>8)&0xFF, resp&0xFF);
    return SDIO_R_ERR; 
}

int sdio_set(uint32_t fn, uint32_t addr, uint32_t bits) {
    return sdio_write(fn, addr, sdio_read(fn, addr) | bits);
}

int sdio_perform_io(uint32_t is_write, uint32_t fn, uint32_t addr, uint8_t *buffer, uint32_t num_bytes, uint32_t addr_increment) {
    uint32_t blocksize = ((fn == SDIO_FN_WLAN) ? 512 : 64);  // RAD block size = 512, BAK block size = 64 
    uint32_t blockmode = 0; 
    uint32_t block_or_byte_count = 0; 
    uint32_t nbytes = 0; 
    while (num_bytes > 0) {
        if (num_bytes >= 511 * blocksize) {
            blockmode = 1; 
            block_or_byte_count = 511; 
            nbytes = block_or_byte_count * blocksize; 
        } else if (num_bytes > blocksize) {
            blockmode = 1; 
            block_or_byte_count = (
                (fn == SDIO_FN_WLAN) ? 
                    (num_bytes / 512) : 
                    (num_bytes / 64)
            ); // num_bytes / blocksize;  // note: division 
            nbytes = block_or_byte_count * blocksize; 
        } else {
            blockmode = 0; 
            block_or_byte_count = num_bytes; 
            nbytes = num_bytes; 
        }

        if (blockmode) {
            emmc_dma_setup(blocksize, block_or_byte_count); 
        } else {
            emmc_dma_setup(block_or_byte_count, 1); 
        }

        int cmdsuccess = sdio_cmd(
            SDIO_CMD_IO_RW_EXTENDED, 
            cmd53_args_assemble(
                is_write, 
                fn, 
                blockmode, 
                addr_increment, 
                addr, 
                block_or_byte_count
            )
        ); 
        if (cmdsuccess < 0) {
            printk("sdio CMD53 failed!\n"); 
            return SDIO_R_ERR; 
        }
        emmc_perform_dma(is_write, buffer, nbytes); 
        num_bytes -= nbytes; 
        buffer += nbytes; 
        if (addr_increment) {
            addr += nbytes; 
        }
    }
    return SDIO_R_OK; 
}

int set_ether_func_gpio() {
    printk("disconnecting emmc from sd card...\n");
    /* disconnect emmc from SD card (connect sdhost instead) */
    for (unsigned i = 48; i <= 53; ++i) {
        // printk("i=%d\n", i);
        gpio_set_function(i, GPIO_FUNC_ALT0);
    }
    printk("connecting emmc to wifi...\n");
    /* connect emmc to wifi */
    for (unsigned i = 34; i <= 39; ++i) {
        gpio_set_function(i, GPIO_FUNC_ALT3); 
        if (i == 34) {
            gpio_set_pull(i, PULL_NONE); 
        } else {
            gpio_set_pull(i, PULL_UP); 
        }
    }
    printk("done!\n"); 
    return SDIO_R_OK; 
}

int sdio_init(void) {
    dma_init(); 
    emmc_init();
    uint32_t ocr = 0;
    uint32_t rca = 0; 
    sdio_cmd(SDIO_CMD_GO_IDLE_STATE, 0); 
    ocr = sdio_cmd(SDIO_CMD_IO_SEND_OP_COND, 0); 
    if (ocr == -1) {
        printk("ocr failed to read!\n"); 
        return SDIO_R_ERR; 
    }
    uint32_t retries = 0; 
    while ((ocr & (1 << 31)) == 0) {
        retries += 1; 
        if (retries > SDIO_WRITE_RETRY) {
            printk("sdio card no response after 5 retries\n"); 
            return SDIO_R_ERR; 
        }
        ocr = sdio_cmd(SDIO_CMD_IO_SEND_OP_COND, SDIO_OP_V3_3); 
        delay_ms(100); 
    }
    rca = sdio_cmd(SDIO_CMD_SEND_RELATIVE_ADDR, 0); 
    sdio_cmd(SDIO_CMD_SELECT_CARD, rca); 
    rca = (rca >> 16); 
    sdio_set(SDIO_FN0, SDIO_CCCR_HIGH_SPEED, 2); 
    sdio_set(SDIO_FN0, SDIO_CCCR_BUSIFC, 2); 
    // this is by the CYW43430 documentation: the backplane (to access its ARM CPU) will have block size 64 
    // and the WLAN will have block size 512 
    sdio_write(SDIO_FN0, SDIO_BACKPLANE_BASE + SDIO_CCCR_BLOCK_SIZE, 64); 
    sdio_write(SDIO_FN0, SDIO_BACKPLANE_BASE + SDIO_CCCR_BLOCK_SIZE + 1, 64 >> 8); 
    sdio_write(SDIO_FN0, SDIO_WLAN_BASE + SDIO_CCCR_BLOCK_SIZE, 512); 
    sdio_write(SDIO_FN0, SDIO_WLAN_BASE + SDIO_CCCR_BLOCK_SIZE + 1, 512 >> 8); 
    sdio_set(SDIO_FN0, SDIO_CCCR_IO_ENABLE, 1<<SDIO_FN_BACKPLANE); 
    sdio_write(SDIO_FN0, SDIO_CCCR_INT_ENABLE, 0); 
    retries = 0; 
    while (
        !(sdio_read(SDIO_FN0, SDIO_CCCR_IO_READY) & (1 << SDIO_FN_BACKPLANE))
    ) {
        retries += 1; 
        if (retries > SDIO_WRITE_RETRY) {
            printk("failed to enable sdio io function\n"); 
            return SDIO_R_ERR; 
        }
        delay_ms(100); 
    }
    sdio_state.ocr = ocr; 
    sdio_state.rca = rca; 
    printk("wlan sdio_init OK! ocr = %x, rca = %x\n", ocr, rca); 
    return SDIO_R_OK; 
}


int sdio_reset(void) {
    return sdio_write(SDIO_FN0, SDIO_CCCR_IO_ABORT, 1 << 3); 
}

int sdio_abort(unsigned fn) {
    return sdio_write(SDIO_FN0, SDIO_CCCR_IO_ABORT, fn); 
} 




























