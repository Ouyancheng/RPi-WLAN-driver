#include "sdhost/sdhost.h"
#include "gpio.h"
#include "mbox/mbox.h"


#if SDHOST_USE_DMA
#include "dma/dma.h" 
#endif
const uint32_t sdhost_sdcmd_masks[64] = {
    [0] =  SDHOST_SDCMD_NEW_FLAG | SDHOST_SDCMD_NO_RESPONSE, 
    [2] =  SDHOST_SDCMD_NEW_FLAG | SDHOST_SDCMD_LONG_RESPONSE, 
    [3] =  SDHOST_SDCMD_NEW_FLAG | SDHOST_SDCMD_NORMAL_RESPONSE, 
    [5] =  SDHOST_SDCMD_NEW_FLAG | SDHOST_SDCMD_NORMAL_RESPONSE, 
    [6] =  SDHOST_SDCMD_NEW_FLAG | SDHOST_SDCMD_NORMAL_RESPONSE, 
    [7] =  SDHOST_SDCMD_NEW_FLAG | SDHOST_SDCMD_BUSYWAIT, 
    [8] =  SDHOST_SDCMD_NEW_FLAG | SDHOST_SDCMD_NORMAL_RESPONSE, 
    [9] =  SDHOST_SDCMD_NEW_FLAG | SDHOST_SDCMD_LONG_RESPONSE, 
    [11] = SDHOST_SDCMD_NEW_FLAG | SDHOST_SDCMD_NORMAL_RESPONSE, 
    [12] = SDHOST_SDCMD_NEW_FLAG | SDHOST_SDCMD_BUSYWAIT, 
    [13] = SDHOST_SDCMD_NEW_FLAG | SDHOST_SDCMD_NORMAL_RESPONSE, 
    [16] = SDHOST_SDCMD_NEW_FLAG | SDHOST_SDCMD_NORMAL_RESPONSE, 
    [17] = SDHOST_SDCMD_NEW_FLAG | SDHOST_SDCMD_NORMAL_RESPONSE | SDHOST_SDCMD_READ_CMD, 
    [18] = SDHOST_SDCMD_NEW_FLAG | SDHOST_SDCMD_NORMAL_RESPONSE | SDHOST_SDCMD_READ_CMD, 
    [24] = SDHOST_SDCMD_NEW_FLAG | SDHOST_SDCMD_NORMAL_RESPONSE | SDHOST_SDCMD_WRITE_CMD, 
    [25] = SDHOST_SDCMD_NEW_FLAG | SDHOST_SDCMD_NORMAL_RESPONSE | SDHOST_SDCMD_WRITE_CMD, 
    [41] = SDHOST_SDCMD_NEW_FLAG | SDHOST_SDCMD_NORMAL_RESPONSE, 
    [51] = SDHOST_SDCMD_NEW_FLAG | SDHOST_SDCMD_NORMAL_RESPONSE | SDHOST_SDCMD_READ_CMD,  
    // [52] = SDHOST_SDCMD_NEW_FLAG | SDHOST_SDCMD_NORMAL_RESPONSE, 
    // [53] = SDHOST_SDCMD_NEW_FLAG | SDHOST_SDCMD_NORMAL_RESPONSE, 
    [55] = SDHOST_SDCMD_NEW_FLAG | SDHOST_SDCMD_NORMAL_RESPONSE, 
    [63] = SDHOST_SDCMD_NEW_FLAG | SDHOST_SDCMD_NORMAL_RESPONSE | SDHOST_SDCMD_READ_CMD, 
}; // NOTE: NO SDIO support!!! 
struct SDHostStatus sdhost_status; 

uint32_t sdhost_get_sdcmd_mask(uint32_t cmd) {
    if (cmd >= 64) {
        return 0; 
    }
    uint32_t mask = sdhost_sdcmd_masks[cmd];
    // if (mask == 0) {
    //     printk("sdhost_get_sdcmd_mask: CMD%d is unknown command! Notice that SDHOST currently does not support SDIO...\n", cmd); 
    //     return 0; 
    // }
    return mask; 
}

static inline void write_reg(uint32_t reg, uint32_t val) {
    (*((volatile uint32_t*)(SDHOST_BASE+reg))) = val; 
    dev_barrier(); 
}

static inline uint32_t read_reg(uint32_t reg) {
    return (*((volatile uint32_t*)(SDHOST_BASE+reg))); 
}

void sdhost_set_init_freq() {
    uint32_t div = sdhost_status.extclock / SDHOST_INIT_FREQ; 
    if (sdhost_status.extclock / SDHOST_INIT_FREQ > SDHOST_INIT_FREQ) {
        div += 1; 
    }
    if (div < 2) {
        div = 2; 
    }
    write_reg(SDHOST_SDCDIV, div - 2); 
}
void sdhost_set_sd_freq() {
    uint32_t div = sdhost_status.extclock / SDHOST_SD_FREQ; 
    if (sdhost_status.extclock / SDHOST_SD_FREQ > SDHOST_SD_FREQ) {
        div += 1; 
    }
    if (div < 2) {
        div = 2; 
    }
    write_reg(SDHOST_SDCDIV, div - 2); 
}
void sdhost_set_sdhc_freq() {
    uint32_t div = sdhost_status.extclock / SDHOST_SDHC_FREQ; 
    if (sdhost_status.extclock / SDHOST_SDHC_FREQ > SDHOST_SDHC_FREQ) {
        div += 1; 
    }
    if (div < 2) {
        div = 2; 
    }
    write_reg(SDHOST_SDCDIV, div - 2); 
}

int sdhost_init(int set_gpio) {
    /* disconnect emmc and connect sdhost to SD card gpio pins */
    if (set_gpio) {
        for (uint32_t i = 48; i <= 53; ++i) {
            gpio_set_function(i, GPIO_FUNC_ALT0); 
        }
    }
    uint32_t clock = rpi_get_clock_rate_hz((uint32_t)MBOX_CLOCK_CORE); 
    if (clock == 0) {
        clock = SDHOST_EXTERNAL_FREQ;
        printk("SDHOST core frequency unknown, assuming 250MHz\n"); 
    }
    sdhost_status.extclock = clock; 
    sdhost_set_init_freq(); 
    write_reg(SDHOST_SDVDD, SDHOST_SDVDD_POWER_OFF); 
    write_reg(SDHOST_SDTOUT, 0xF00000); 

    uint32_t temp = read_reg(SDHOST_SDEDM); 
    temp &= ~((SDHOST_SDEDM_THRESHOLD_MASK << SDHOST_SDEDM_READ_THRESHOLD_SHIFT) |
	    (SDHOST_SDEDM_THRESHOLD_MASK << SDHOST_SDEDM_WRITE_THRESHOLD_SHIFT));
    temp |= (SDHOST_FIFO_READ_THRESHOLD << SDHOST_SDEDM_READ_THRESHOLD_SHIFT) |
		(SDHOST_FIFO_WRITE_THRESHOLD << SDHOST_SDEDM_WRITE_THRESHOLD_SHIFT);
    write_reg(SDHOST_SDEDM, temp); 

    delay_ms(20);
    write_reg(SDHOST_SDVDD, SDHOST_SDVDD_POWER_ON);
    delay_ms(20);

    write_reg(SDHOST_SDHCFG, SDHOST_SDHCFG_WIDE_INT_BUS | SDHOST_SDHCFG_SLOW_CARD | SDHOST_SDHCFG_BUSY_IRPT_EN); 
    // write_reg(SDHOST_SDCDIV, SDHOST_SDCDIV_MAX_CDIV); 
    sdhost_set_init_freq(); 
    dev_barrier(); 
    delay_ms(100); 
    return 0; 
}

int sdhost_reset() {
    uint32_t temp;
    uint32_t hcfg = read_reg(SDHOST_SDHCFG); 
    uint32_t cdiv = read_reg(SDHOST_SDCDIV); 
    write_reg(SDHOST_SDVDD, SDHOST_SDVDD_POWER_OFF);
    write_reg(SDHOST_SDCMD, 0);
    write_reg(SDHOST_SDARG, 0);
    write_reg(SDHOST_SDTOUT, 0xf00000);
    write_reg(SDHOST_SDCDIV, 0);
    write_reg(SDHOST_SDHSTS, 0x7f8); /* Write 1s to clear */
    write_reg(SDHOST_SDHCFG, 0);
    write_reg(SDHOST_SDHBCT, 0);
    write_reg(SDHOST_SDHBLC, 0);
    /* Limit fifo usage due to silicon bug */
    temp = read_reg(SDHOST_SDEDM);
    temp &= ~((SDHOST_SDEDM_THRESHOLD_MASK << SDHOST_SDEDM_READ_THRESHOLD_SHIFT) |
            (SDHOST_SDEDM_THRESHOLD_MASK << SDHOST_SDEDM_WRITE_THRESHOLD_SHIFT));
    temp |= (SDHOST_FIFO_READ_THRESHOLD << SDHOST_SDEDM_READ_THRESHOLD_SHIFT) |
        (SDHOST_FIFO_WRITE_THRESHOLD << SDHOST_SDEDM_WRITE_THRESHOLD_SHIFT);
    write_reg(SDHOST_SDEDM, temp);
    delay_ms(20);
    write_reg(SDHOST_SDVDD, SDHOST_SDVDD_POWER_ON);
    delay_ms(20);
    // host->clock = 0;
    write_reg(SDHOST_SDHCFG, hcfg);
    write_reg(SDHOST_SDCDIV, cdiv);
    return 0; 
}


int sdhost_send_command(uint32_t command, uint32_t argument, uint32_t *response) {
    if (command >= 64) {
        printk("SDHOST: CMD%d doesn't seem to be supported\n", command);
        return -1;
    }
    if (!response) {
        printk("response must have at least 4 uint32_t!\n"); 
        return -2; 
    } 
    // clear the interrupt 
    // write_reg(SDHOST_SDHSTS, read_reg(SDHOST_SDHSTS)); 
    uint32_t cmdreg = (command | sdhost_get_sdcmd_mask(command));
    switch (command) {
        case EMMC_CMD_APP_CMD: 
            if (!argument) {
                // notice that initially it's normal response 48-bit (0x000)
                cmdreg &= (~SDHOST_SDCMD_RESPONSE_MASK);
                cmdreg |= SDHOST_SDCMD_NO_RESPONSE; 
            }
            break;
        case EMMC_CMD_SWITCH_FUNC: 
            // special case on CMD6 -- 
            // it may be switch func CMD6 or set bus width ACMD6, 
            // this depends on whether appcmd is sent before   
            if (!sdhost_status.appcmd) { 
                cmdreg |= (SDHOST_SDCMD_READ_CMD); 
            }
            break;
        case EMMC_CMD_GO_IDLE_STATE: 
            // CMD0 is basically a reset, switch to low speed and 1-bit width 
            write_reg(SDHOST_SDHCFG, read_reg(SDHOST_SDHCFG) & (~SDHOST_SDHCFG_WIDE_EXT_BUS)); 
            // switch to initial frequency 
            sdhost_set_init_freq();
            sdhost_status.fast_clock = 0; 
            break;
        case EMMC_CMD_SELECT_CARD: 
            // NOTE: set the frequency BEFORE selecting the card 
            // otherwise the SCR register might not be read correctly!!! 
            delay_ms(1);
            sdhost_set_sd_freq();
            delay_ms(1);
            break;
        default: 
            break;
    }
    uint32_t status = read_reg(SDHOST_SDHSTS); 
    write_reg(SDHOST_SDHSTS, status);
    // if (status & (SDHOST_SDHSTS_ERROR_MASK | SDHOST_SDHSTS_DATA_FLAG)) {
    //     write_reg(SDHOST_SDHSTS, status);  // clear the interrupt  
    // }
    sdhost_status.done = 0; 
    // issue the command 
    write_reg(SDHOST_SDARG, argument);
    write_reg(SDHOST_SDCMD, cmdreg);
    // switch (command) {
    //     case EMMC_CMD_IO_SEND_OP_COND: 
    //         delay_ms(1000); break;
    //     case EMMC_CMD_SEND_IF_COND: 
    //     case EMMC_CMD_APP_CMD: 
    //         delay_ms(100); break; 
    //     default: break; 
    // }
    uint32_t c = read_reg(SDHOST_SDCMD); 
    unsigned cnt = 0; 
    while ((c & (SDHOST_SDCMD_NEW_FLAG | SDHOST_SDCMD_FAIL_FLAG)) == SDHOST_SDCMD_NEW_FLAG) {
        cnt += 1; 
        if (cnt > 1000) { 
            break; // timeout! 
        }
        delay_ms(1); 
        c = read_reg(SDHOST_SDCMD); 
    }
    if ((c & SDHOST_SDCMD_FAIL_FLAG) != 0) {
        status = read_reg(SDHOST_SDHSTS); 
        // if (status & SDHOST_SDHSTS_CMD_TIME_OUT) {
        //     printk("SDHOST: CMD%d timeout!\n", command); 
        // }
        if (status & SDHOST_SDHSTS_CMD_TIME_OUT) {
            printk("SDHOST: CMD%d timeout!\n", command); 
            write_reg(SDHOST_SDHSTS, status); 
            return -3; 
        } else if (status & SDHOST_SDHSTS_ERROR_MASK) {
            printk("SDHOST: CMD%d ERROR! cmdreg=%x, arg=%x, status=%x\n", 
                command, read_reg(SDHOST_SDCMD), read_reg(SDHOST_SDARG), status); 
            write_reg(SDHOST_SDHSTS, status); 
            return -4; 
        }
        printk("SDHOST: CMD%d status: cmdreg=%x, arg=%x, status=%x\n", 
            command, read_reg(SDHOST_SDCMD), read_reg(SDHOST_SDARG), status); 
        write_reg(SDHOST_SDHSTS, status); 
    }
    switch (cmdreg & SDHOST_SDCMD_RESPONSE_MASK) {
        case SDHOST_SDCMD_LONG_RESPONSE: 
            // 136-bit response, note: 136 = 128 + 8 
            response[0] = read_reg(SDHOST_SDRSP0);
            response[1] = read_reg(SDHOST_SDRSP1);
            response[2] = read_reg(SDHOST_SDRSP2);
            response[3] = read_reg(SDHOST_SDRSP3);
            break;
        case SDHOST_SDCMD_NORMAL_RESPONSE: 
            response[0] = read_reg(SDHOST_SDRSP0);
            break; 
        case SDHOST_SDCMD_BUSYWAIT: 
            response[0] = read_reg(SDHOST_SDRSP0);
            status = sdhost_wait_for_interrupt(SDHOST_SDHSTS_BUSY_IRPT, 3000); 
            // status = sdhost_wait_for_interrupt(SDHOST_SDHSTS_DATA_FLAG, 3000); 
            if (status == 0) {
                printk("CMD%d has R1b response, but wait timeout, status=%x\n", command, status); 
            }
            if (status & SDHOST_SDHSTS_ERROR_MASK) {
                printk("CMD%d has R1b response, but an error occurred during the wait, status=%x\n", command, status); 
            }
            break; 
        case SDHOST_SDCMD_NO_RESPONSE: 
            response[0] = 0; 
            break; 
        default: 
            printk("CMD%d, cmdreg = %x, unknown response type!\n", command, cmdreg);
            return -4;
    }
    switch (command) {
        case EMMC_CMD_SEND_IF_COND: 
            if (response[0] != argument) {
                printk("CMD send_if_cond must have response match the argument!\n"); 
                return -5; 
            }
            break; 
        case EMMC_CMD_APP_CMD: 
            // if (argument != 0) { response[0] &= 0x00000020; } 
            break; 
        case EMMC_CMD_SEND_RELATIVE_ADDR:
            response[0] &= EMMC_CMD_RCA_MASK; 
            break;
        case EMMC_CMD_SELECT_CARD: 
            // use faster clock once the card is selected 
            // set it before!!! 
            break; 
        case EMMC_ACMD_SET_BUS_WIDTH: 
            if (sdhost_status.appcmd) {
                if ((argument & 0x3) == 0) {
                    write_reg(SDHOST_SDHCFG, read_reg(SDHOST_SDHCFG) & ~SDHOST_SDHCFG_WIDE_EXT_BUS);
                } else if ((argument & 0x3) == 2) {
                    write_reg(SDHOST_SDHCFG, read_reg(SDHOST_SDHCFG) | SDHOST_SDHCFG_WIDE_EXT_BUS);
                }
            } else {
                // from plan 9 
                /*
                 * If card switched into high speed mode, increase clock speed
                 */
                if ((argument & 0x8000000F) == 0x80000001) {
                    delay_ms(1); 
                    sdhost_set_sdhc_freq(); 
                    delay_ms(1);
                }
            }
            break;
        default: 
            break; 
    }
    sdhost_status.appcmd = (command == EMMC_CMD_APP_CMD);
    return 0;
}
uint32_t sdhost_wait_for_interrupt(uint32_t interrupt, uint32_t timeout) {
    uint32_t status = read_reg(SDHOST_SDHSTS); 
    int cnt = 0; 
    while ((status & (interrupt | SDHOST_SDHSTS_ERROR_MASK)) == 0) {
        cnt += 1; 
        if (cnt > timeout) {
            printk("SDHOST: interrupt wait timeout!\n"); 
            return 0; 
        }
        delay_ms(1); 
        status = read_reg(SDHOST_SDHSTS); 
    }
    write_reg(SDHOST_SDHSTS, status); 
    sdhost_status.done = 1; 
    return status; 
}
int sdhost_io_setup(uint32_t block_size, uint32_t block_count) {
    write_reg(SDHOST_SDHBCT, block_size); 
    write_reg(SDHOST_SDHBLC, block_count); 
    sdhost_status.block_count = block_count; 
    return 0; 
}

int sdhost_perform_io(unsigned is_write, uint8_t *buffer, uint32_t buffer_size) {
    /*
     * According to comments in the linux driver, the hardware "doesn't
     * manage the FIFO DREQs properly for multi-block transfers" on input,
     * so the dma must be stopped early and the last 3 words fetched with pio
     * -- Plan 9 
     */
#if SDHOST_USE_DMA 
    if ((buffer_size % 4) != 0) {
        printk("buffer size must be a multiple of 4! buffer_size=%d\n", buffer_size); 
        return -1; 
    }
    if ((((uintptr_t)buffer) % 4)!= 0) {
        printk("buffer address must be 4-byte aligned!\n"); 
        return -2; 
    }
    uint32_t piolen = buffer_size; // 0; 
    if (!is_write && sdhost_status.block_count > 1) {
        piolen = (SDHOST_FIFO_READ_THRESHOLD-1) * sizeof(uint32_t);
        buffer_size -= piolen;
    }
    if (is_write) {
        dma_start(SDHOST_DMA_CHANNEL, SDHOST_DMA_DEVICE, DMA_MEM2DEV, buffer, (void*)(SDHOST_BASE+SDHOST_SDDATA), buffer_size); 
    } else {
        dma_start(SDHOST_DMA_CHANNEL, SDHOST_DMA_DEVICE, DMA_DEV2MEM, (void*)(SDHOST_BASE+SDHOST_SDDATA), buffer, buffer_size); 
    }
    if (dma_wait(SDHOST_DMA_CHANNEL) == 0) {
        printk("SDHOST: DMA failed!\n"); 
        return -3; 
    }
    // sdhost_wait_for_interrupt(SDHOST_SDHSTS_DATA_FLAG, 3000); 
    if (!is_write) {
        flush_all_caches(); 
        buffer += buffer_size; 
        uint32_t dbgmode = 0;
        uint32_t word = 0; 
        while (piolen > 0) {
            dbgmode = read_reg(SDHOST_SDEDM); 
            if ((dbgmode & 0x1F00) == 0) {
                printk("SDHOST: FIFO empty after short dma read\n");
                return -4; 
            }
            word = read_reg(SDHOST_SDDATA); 
            *((uint32_t*)buffer) = word; 
            buffer += sizeof(uint32_t); 
            piolen -= sizeof(uint32_t); 
        }
    }
    return 0; 

#else 
    uint32_t piolen = buffer_size; 
    sdhost_wait_for_interrupt(SDHOST_SDHSTS_DATA_FLAG, 3000); 
    uint32_t dbgmode = 0;
    uint32_t word = 0; 
    while (piolen > 0) {
        dbgmode = read_reg(SDHOST_SDEDM); 
        if ((dbgmode & 0x1F00) == 0) {
            printk("SDHOST: FIFO empty in PIO?\n");
            return -4; 
        }
        word = read_reg(SDHOST_SDDATA); 
        *((uint32_t*)buffer) = word; 
        buffer += sizeof(uint32_t); 
        piolen -= sizeof(uint32_t); 
    }
    return 0; 
#endif 

}
























