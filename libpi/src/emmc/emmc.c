#include "emmc/emmc.h"

/// the state for the emmc transmission 
struct EMMCState emmc; 
//  CMD0:  GO_IDLE_STATE       = 0,  // resp = no? 
//  CMD3:  SEND_RELATIVE_ADDR  = 3,  // resp = r6 
//  CMD5:  IO_SEND_OP_COND     = 5,  // resp = r4 
//  CMD7:  SELECT_CARD         = 7,  // resp = r1b 
//  CMD11: VOLTAGE_SWITCH      = 11, // resp = ?
//  CMD52: IO_RW_DIRECT        = 52, // resp = r5 
//  CMD53: IO_RW_EXTENDED      = 53, // resp = r5 
#ifndef emmc_cmdtm_masks_macro
#define emmc_cmdtm_masks_macro
const uint32_t emmc_cmdtm_masks[64] = {
    [0] = EMMC_CMDTM_IXCHK_EN, // 0 // 0, // 
    [1] = 0, // 1
    [2] = EMMC_CMDTM_RESP_136, // 2
    [3] = EMMC_CMDTM_RESP_48 | EMMC_CMDTM_IXCHK_EN | EMMC_CMDTM_CRCCHK_EN, // 3 // 0x03020000, //  
    [4] = 0, // 4
    [5] = EMMC_CMDTM_RESP_48, // 5, no check for CRC because the wlan card seems to return the wrong one 
    [6] = EMMC_CMDTM_RESP_48 | EMMC_CMDTM_IXCHK_EN | EMMC_CMDTM_CRCCHK_EN, // 6 
    [7] = EMMC_CMDTM_RESP_48BUSY | EMMC_CMDTM_IXCHK_EN | EMMC_CMDTM_CRCCHK_EN, // 7 // 0x07030000, // 
    [8] = EMMC_CMDTM_RESP_48 | EMMC_CMDTM_IXCHK_EN | EMMC_CMDTM_CRCCHK_EN, // 8 // 0x08020000, // 
    [9] = EMMC_CMDTM_RESP_136, // 9 
    [10] = 0, // 10
    [11] = EMMC_CMDTM_RESP_48 | EMMC_CMDTM_IXCHK_EN | EMMC_CMDTM_CRCCHK_EN, // 11
    [12] = EMMC_CMDTM_RESP_48BUSY | EMMC_CMDTM_IXCHK_EN | EMMC_CMDTM_CRCCHK_EN, // 12 
    [13] = EMMC_CMDTM_RESP_48 | EMMC_CMDTM_IXCHK_EN | EMMC_CMDTM_CRCCHK_EN, // 13 
    [14] = 0, // 14 
    [15] = 0, // 15 
    [16] = EMMC_CMDTM_RESP_48, // 16 
    [17] = EMMC_CMDTM_RESP_48 | EMMC_CMDTM_ISDATA | EMMC_CMDTM_CARD_TO_HOST | EMMC_CMDTM_IXCHK_EN | EMMC_CMDTM_CRCCHK_EN, // 17 
    [18] = EMMC_CMDTM_RESP_48 | EMMC_CMDTM_ISDATA | EMMC_CMDTM_CARD_TO_HOST | EMMC_CMDTM_MULTI_BLOCK | EMMC_CMDTM_BLKCNT_EN | EMMC_CMDTM_IXCHK_EN | EMMC_CMDTM_CRCCHK_EN, // 18 
    [19] = 0, 
    [20] = 0, 
    [21] = 0, 
    [22] = 0, 
    [23] = 0, // 19, 20, 21, 22, 23
    [24] = EMMC_CMDTM_RESP_48 | EMMC_CMDTM_ISDATA | EMMC_CMDTM_HOST_TO_CARD | EMMC_CMDTM_IXCHK_EN | EMMC_CMDTM_CRCCHK_EN, // 24 
    [25] = EMMC_CMDTM_RESP_48 | EMMC_CMDTM_ISDATA | EMMC_CMDTM_HOST_TO_CARD | EMMC_CMDTM_MULTI_BLOCK | EMMC_CMDTM_BLKCNT_EN | EMMC_CMDTM_IXCHK_EN | EMMC_CMDTM_CRCCHK_EN, // 25 
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
    // 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40
    [41] = EMMC_CMDTM_RESP_48, // 41 
        0,  0,  0,  0,  0,  0,  0,  0,  0,  
    // 42, 43, 44, 45, 46, 47, 48, 49, 50, 
    [51] = EMMC_CMDTM_CARD_TO_HOST | EMMC_CMDTM_RESP_48 | EMMC_CMDTM_ISDATA | EMMC_CMDTM_IXCHK_EN | EMMC_CMDTM_CRCCHK_EN,  // 51 // 0x33220010, //
    [52] = EMMC_CMDTM_RESP_48 | EMMC_CMDTM_IXCHK_EN | EMMC_CMDTM_CRCCHK_EN, // 52
    [53] = EMMC_CMDTM_RESP_48 | EMMC_CMDTM_IXCHK_EN | EMMC_CMDTM_CRCCHK_EN | EMMC_CMDTM_ISDATA, // 53
    0, // 54
    [55] = EMMC_CMDTM_RESP_48 | EMMC_CMDTM_IXCHK_EN | EMMC_CMDTM_CRCCHK_EN, // 55  // 0x37000000|EMMC_CMDTM_RESP_48, // 
};
#endif // ! emmc_cmdtm_masks_macro 
uint32_t emmc_cmdtm_get_masks(uint32_t cmd) {
    if (cmd >= 64) { return 0; }
    return emmc_cmdtm_masks[cmd]; 
}






int emmc_test_clock_dividers(void *arg) {
    uint32_t divider = (emmc.extclock+EMMC_INIT_FREQUENCY*2-1) / (EMMC_INIT_FREQUENCY * 2); 
    printk("init_freq divider = %d\n", divider);
    assert(divider < (1 << 10));
    divider = (emmc.extclock+EMMC_SD_FREQUENCY*2-1) / (EMMC_SD_FREQUENCY * 2); 
    printk("sd_freq divider = %d\n", divider);
    assert(divider < (1 << 10));
    divider = (emmc.extclock+EMMC_SD_HIGH_FREQUENCY*2-1) / (EMMC_SD_HIGH_FREQUENCY * 2); 
    printk("sdhc_freq divider = %d\n", divider);
    assert(divider < (1 << 10));
    return EMMC_R_OK; 
}
/**
 * NOTE: 
 * Control1 register: 
 * bit 7:6  CLK_FREQ_MS2 SD clock base divider MSBs
 * bit 15:8 CLK_FREQ8    SD clock base divider LSBs
 */
inline uint32_t emmc_get_clock_div_mask(unsigned div) {
    uint32_t v; 
    assert(div < (1 << 10));
    v = ((div << 8) & 0xFF00);     // LSBs 
    v |= ((div >> 8) << 6) & 0xC0; // MSBs 
    return v; 
}
int emmc_set_clock_divider(uint32_t divider) {
    assert(divider < (1 << 10));
    emmc_write_register(EMMC_CONTROL1, GET32(EMMC_CONTROL1) & ~EMMC_CTRL1_CLK_ENABLE);
    delay_ms(10);
    uint32_t control1_reg_cmd = emmc_get_clock_div_mask(divider) | 
        EMMC_CTRL1_TOUNIT_MAX | 
        EMMC_CTRL1_CLK_GEN_DIV | 
        EMMC_CTRL1_CLK_ENABLE | 
        EMMC_CTRL1_CLK_INTL_EN; 
    emmc_write_register(EMMC_CONTROL1, control1_reg_cmd);
    int clock_stable = 0;
    for (int i = 0; i < 1000; ++i) {
        delay_ms(1);
        if (GET32(EMMC_CONTROL1) & EMMC_CTRL1_CLK_STABLE) {
            clock_stable = 1;
            break;
        }
    }
    if (!clock_stable) {
        printk("emmc clock failed to initialize after 1000 loops!\n");
        return EMMC_R_ERR;
    }
    return EMMC_R_OK; 
}
int emmc_set_init_freq() {
    uint32_t divider = (emmc.extclock+EMMC_INIT_FREQUENCY*2-1) / (EMMC_INIT_FREQUENCY * 2); 
    int success = emmc_set_clock_divider(divider); 
    return success; 
    
}
int emmc_set_sd_freq() {
    uint32_t divider = (emmc.extclock+EMMC_SD_FREQUENCY*2-1) / (EMMC_SD_FREQUENCY * 2); 
    int success = emmc_set_clock_divider(divider); 
    emmc.use_fastclock = 1; 
    return success;
}
int emmc_set_sdhc_freq() {
    uint32_t divider = (emmc.extclock+EMMC_SD_HIGH_FREQUENCY*2-1) / (EMMC_SD_HIGH_FREQUENCY * 2); 
    int success = emmc_set_clock_divider(divider);     
    emmc.use_fastclock = 2; 
    return success; 
}

void emmc_write_register(unsigned reg, uint32_t val) {
    dev_barrier();
    // plan 9 delays first then write... strange... 
    PUT32(reg, val); 
    dev_barrier();
    delay_us((emmc.use_fastclock) ? 2 : 20); 
} 

/// reset the command handling circuit 
void emmc_reset_cmd_circuit(uint32_t ms_delay) {
    #if EMMC_DEBUG
    printk("resetting cmd circuit!!!\n");
    #endif 
    emmc_write_register(EMMC_CONTROL1, GET32(EMMC_CONTROL1) | EMMC_CTRL1_RESET_CMD_HANDLING_CIRCUIT);
    if (ms_delay) delay_ms(ms_delay); else delay_ms(10);
    // don't poll!!! 
    // while ((GET32(EMMC_CONTROL1) & EMMC_CTRL1_RESET_CMD_HANDLING_CIRCUIT)) {}
}
/// reset the data handling circuit 
void emmc_reset_data_circuit(uint32_t ms_delay) {
    #if EMMC_DEBUG
    printk("resetting data circuit!!!\n");
    #endif 
    emmc_write_register(EMMC_CONTROL1, GET32(EMMC_CONTROL1) | EMMC_CTRL1_RESET_DATA_HANDLING_CIRCUIT);
    if (ms_delay) delay_ms(ms_delay); else delay_ms(10);
    // do not poll!!! 
    // while ((GET32(EMMC_CONTROL1) & EMMC_CTRL1_RESET_DATA_HANDLING_CIRCUIT)) {}
}
/// reset the host circuit (basically resets the whole controller)
void emmc_reset_host_circuit(uint32_t ms_delay) {
    emmc_write_register(EMMC_CONTROL1, GET32(EMMC_CONTROL1) | EMMC_CTRL1_RESET_HOST_CIRCUIT); 
    if (ms_delay) delay_ms(ms_delay); 
    while (GET32(EMMC_CONTROL1) & EMMC_CTRL1_RESET_HOST_CIRCUIT) {} 
}

int emmc_init(void) {
    #if EMMC_USE_INTERRUPT
    interrupt_init(); // should already enabled: system_enable_interrupts(); dev_barrier();
    #endif 
    // use mailbox to get the EMMC clock rate 
    uint32_t clk = rpi_get_clock_rate_hz(EMMC_CLOCK_ID);
    emmc.extclock = clk;
    emmc.interrupt_val = 0;
    emmc.use_fastclock = 0; 
    emmc.appcmd = 0; 
    emmc.card_handler = NULL; 
    // #if EMMC_DEBUG
    //     printk("emmc clock rate = %u Hz\n", clk);
    //     printk("emmc external clock %u Mhz\n", clk/1000000);
    //     printk("emmc control registers:\ncontrol0 = %x\ncontrol1 = %x\ncontrol2 = %x\n", 
    //         GET32(EMMC_CONTROL0), 
    //         GET32(EMMC_CONTROL1),
    //         GET32(EMMC_CONTROL2)
    //     );
    // #endif 
    PUT32(EMMC_CONTROL0, 0); 
    emmc_reset_host_circuit(0); 
    #if EMMC_DEBUG
    printk("host circuit reset success\n");
    #endif 
    emmc_reset_data_circuit(0); 
    // #if EMMC_DEBUG
    //     printk("emmc control registers after init:\ncontrol0 = %x\ncontrol1 = %x\ncontrol2 = %x\n", 
    //         GET32(EMMC_CONTROL0), 
    //         GET32(EMMC_CONTROL1),
    //         GET32(EMMC_CONTROL2)
    //     );
    // #endif 
    // PUT32(EMMC_CONTROL1, GET32(EMMC_CONTROL1) | EMMC_CTRL1_CLK_INTL_EN | EMMC_CTRL1_TOUNIT_MAX); 
    // delay_ms(10); 
    int set_clk_success = emmc_set_init_freq(); // emmc_set_clock(EMMC_INIT_FREQUENCY); 
    if (set_clk_success != EMMC_R_OK) {
        printk("emmc_init: clock failed to set!\n"); 
        return EMMC_R_ERR;
    }
    emmc.use_fastclock = 0; 
    // 0xFFFF0000 ==> enable interrupts for all errors 
    // 0x00000100 ==> (1<<8): create a card interrupt if there's any 
    // 0x00000020 ==> (1<<5): create an interrupt if the data is ready to read 
    // 0x00000003 ==> (1<<1): create an interrupt if data transfer is finished 
    //  (1<<0): create an interrupt if cmd is done 
    // 0xFFFF0123
    emmc_write_register(EMMC_IRPT_EN, 0); // EMMC_INTERRUPT_CARD);  // please disable all interrupts... otherwise it will receive an interrupt at CMD done ...  
    // please also disable card interrupt, otherwise there will be a LOT of card interrupts after the wifi card's interrupt is enabled!!! 
    emmc_write_register(EMMC_IRPT_MASK, 0xffffffff);
    emmc_write_register(EMMC_INTERRUPT, 0xffffffff); // NOTE: write 1 to clear the interrupt state... 
    // emmc_write_register(EMMC_CONTROL1, 0);
    if (!emmc.dma_buffer) {
        emmc.dma_buffer = (uint8_t*)mbox_malloc(
            EMMC_BUFFER_MAX_SIZE, 
            EMMC_BUFFER_ALIGNMENT, 
            MEM_FLAG_COHERENT, 
            &(emmc.dma_buffer_handle)
        ); 
    }
    #if EMMC_USE_INTERRUPT
    enable_interrupt(EMMC_IRQ, emmc_interrupt_handler, NULL); 
    #endif 
    dev_barrier(); 
    return EMMC_R_OK;
}

void emmc_interrupt_handler(unsigned pc, void *args) {
    dev_barrier();
    #if EMMC_DEBUG
        printk("received emmc interrupt\n");
    #endif 
    uint32_t interrupt_val = GET32(EMMC_INTERRUPT); 
    
    if (interrupt_val & EMMC_INTERRUPT_ALL_ERROR) {
        interrupt_val |= EMMC_INTERRUPT_ERR; 
    }
    emmc.interrupt_val = interrupt_val; //& (~EMMC_INTERRUPT_CARD); 
    #if EMMC_DEBUG
        printk("emmc interrupt register value = %x\n", interrupt_val);
    #endif 
    // Please DO clear the interrupt bits!!!!!! 
    emmc_write_register(EMMC_INTERRUPT, EMMC_INTERRUPT_ALL_ERROR | interrupt_val); 
    dev_barrier();
    /// DO NOT call the card handler here, because the card handler is also sending data through EMMC!!! 
    // if (interrupt_val & EMMC_INTERRUPT_CARD) {
    //     // handle card interrupt here 
    //     if (emmc.card_handler) {
    //         emmc.card_handler(interrupt_val); 
    //     }
    //     return; 
    // }
    #if EMMC_DEBUG
        printk("done emmc interrupt\n"); 
    #endif 
    return;
}

int emmc_wait_for_data(uint32_t interrupt_mask, uint32_t timeout) {
    #if EMMC_DEBUG 
    printk("emmc_wait_for_data...\n");
    #endif 
    uint32_t mask = (EMMC_INTERRUPT_ALL_ERROR | interrupt_mask);
    #if EMMC_USE_INTERRUPT
        // use interrupt handler! 
        emmc_write_register(EMMC_IRPT_EN, GET32(EMMC_IRPT_EN) | mask); 
        dev_barrier(); 
        uint32_t cnt = 0;
        while ((emmc.interrupt_val & mask) == 0) {}
        // don't poll for the register, as the interrupt handler will clear the register 
        emmc_write_register(EMMC_IRPT_EN, GET32(EMMC_IRPT_EN) & ~mask);  // disable interrupt 
        dev_barrier(); 
    #else 
        while ((GET32(EMMC_INTERRUPT) & mask) == 0) {} 
        emmc.interrupt_val = GET32(EMMC_INTERRUPT); 
        emmc_write_register(EMMC_INTERRUPT, emmc.interrupt_val); 
        dev_barrier(); 
    #endif 
    #if EMMC_DEBUG 
    printk("emmc_wait_for_data done!\n");
    #endif 
    return emmc.interrupt_val; 
}
int emmc_wait_for_cmd(uint32_t timeout) {
    uint32_t mask = (EMMC_INTERRUPT_ALL_ERROR | EMMC_INTERRUPT_CMD_DONE | EMMC_INTERRUPT_ERR); 
    while ((GET32(EMMC_INTERRUPT) & mask) == 0) {}   // notice that we don't generate the interrupt for cmd 
    emmc.interrupt_val = GET32(EMMC_INTERRUPT); 
    emmc_write_register(EMMC_INTERRUPT, EMMC_INTERRUPT_ALL_ERROR | EMMC_INTERRUPT_CMD_DONE);  // write 1 to clear the interrupt bits 
    return emmc.interrupt_val; 
}
int emmc_wait_for_interrupt(uint32_t interrupt_mask, uint32_t timeout) {
    return emmc_wait_for_data(interrupt_mask, timeout); 
}

int emmc_send_command(uint32_t command, uint32_t argument, uint32_t *response) {
    if (command > 55) {
        printk("EMMC: CMD%d doesn't seem to be supported\n", command);
        return EMMC_R_CMD_BAD_ERR;
    }
    if (!response) {
        printk("response must have at least 4 uint32_t!\n"); 
        return EMMC_R_ERR; 
    } 
    PUT32(EMMC_INTERRUPT, GET32(EMMC_INTERRUPT)); 
    uint32_t cmdtm = ((command << EMMC_CMDTM_CMD_INDEX_SHIFT) | emmc_cmdtm_masks[command]);
    switch (command) {
        case EMMC_CMD_APP_CMD: 
            if (!argument) {
                cmdtm &= (~EMMC_CMDTM_RESP_MASK);
                cmdtm |= EMMC_CMDTM_RESP_NONE; 
            }
            break;
        case EMMC_CMD_SWITCH_FUNC: 
            // special case on CMD6 -- 
            // it may be switch func CMD6 or set bus width ACMD6, 
            // this depends on whether appcmd is sent before   
            if (!emmc.appcmd) { 
                cmdtm |= (EMMC_CMDTM_ISDATA | EMMC_CMDTM_CARD_TO_HOST); 
            }
            break;
        case EMMC_CMD_IO_RW_EXTENDED: 
            // special case on CMD53, the read / write flag 
            if (argument & (1 << CMD53_RW_FLAG_SHIFT)) { 
                cmdtm |= EMMC_CMDTM_HOST_TO_CARD; 
            } else { 
                cmdtm |= EMMC_CMDTM_CARD_TO_HOST; 
            }
            // check if single block 
            if ((GET32(EMMC_BLKSIZECNT) & EMMC_BLKCNT_MASK) != (1 << EMMC_BLKCNT_SHIFT)) {
                cmdtm |= (EMMC_CMDTM_MULTI_BLOCK | EMMC_CMDTM_BLKCNT_EN);
            }
            break;
        case EMMC_CMD_GO_IDLE_STATE: 
            // CMD0 is basically a reset, switch to low speed and 1-bit width 
            emmc_write_register(EMMC_CONTROL0, GET32(EMMC_CONTROL0) & ~(EMMC_CTRL0_HCTL_DWIDTH4 | EMMC_CTRL0_HCTL_HS_EN));
            // switch to initial frequency 
            emmc_set_init_freq();
            emmc.use_fastclock = 0; 
            break;
        case EMMC_CMD_SELECT_CARD: 
            // NOTE: set the frequency BEFORE selecting the card 
            // otherwise the SCR register might not be read correctly!!! 
            delay_ms(1);
            emmc_set_sd_freq();
            delay_ms(1);
            break;
        default: 
            break;
    }
    uint32_t status = GET32(EMMC_STATUS); 
    if (status & EMMC_STATUS_CMD_INHIBIT) {
        printk("command line is still used by previous command, interrupt_reg = %x, status_reg = %x\n", 
            GET32(EMMC_INTERRUPT), status);
        uint32_t cnt = 0; 
        uint32_t timeout = 0; 
        while (GET32(EMMC_STATUS) & EMMC_STATUS_CMD_INHIBIT) {
            cnt += 1; if (cnt > 1048576) { timeout = 1; break; }
        }
        if (timeout) {
            printk("emmc cmd inhibit wait timeout!\n");
            emmc_reset_cmd_circuit(0); 
        }
    }
    if (((cmdtm & EMMC_CMDTM_ISDATA) || ((cmdtm & EMMC_CMDTM_RESP_MASK) == EMMC_CMDTM_RESP_48BUSY)) 
        && (status & EMMC_STATUS_DATA_INHIBIT)) {
        printk("CMD%d needs to use data line, but the dataline is still busy, interrupt_reg = %x, status_reg = %x\n", 
            command, GET32(EMMC_INTERRUPT), status);
        uint32_t cnt = 0; 
        uint32_t timeout = 0; 
        while (GET32(EMMC_STATUS) & EMMC_STATUS_DATA_INHIBIT) {
            cnt += 1; if (cnt > 1048576) { timeout = 1; break; }
        }
        if (timeout) {
            printk("emmc data inhibit wait timeout!\n");
            emmc_reset_data_circuit(0); 
        }
    }
    /// issue the command 
    emmc_write_register(EMMC_ARG1, argument);
    emmc_write_register(EMMC_CMDTM, cmdtm);
    uint32_t interrupt_reg = 0;
    uint32_t mask = (EMMC_INTERRUPT_CMD_DONE | EMMC_INTERRUPT_ERR);
    interrupt_reg = emmc_wait_for_cmd(0);
    if ((interrupt_reg & mask) != EMMC_INTERRUPT_CMD_DONE) {
        printk("CMD%d arg=%x issue failed! cmdtm = %x, interrupt_reg = %x, status_reg = %x\n", 
            command, argument, cmdtm, interrupt_reg, GET32(EMMC_STATUS));
        if (GET32(EMMC_STATUS) & EMMC_STATUS_CMD_INHIBIT) { emmc_reset_cmd_circuit(0); }
        return EMMC_R_CMD_TIMEOUT;
    }
    switch (command) {
        case EMMC_CMD_IO_SEND_OP_COND: 
            delay_ms(1000); break;
        case EMMC_CMD_SEND_IF_COND: 
        case EMMC_CMD_APP_CMD: 
            delay_ms(100); break; 
        default: break; 
    }
    switch (cmdtm & EMMC_CMDTM_RESP_MASK) {
        case EMMC_CMDTM_RESP_136: 
            // 136-bit response, note: 136 = 128 + 8 
            // Plan 9 uses this, maybe 136 bit response sets the registers in a special way? 
            // response[0] = (GET32(EMMC_RESP0) << 8);
            // response[1] = ((GET32(EMMC_RESP0) >> 24) | (GET32(EMMC_RESP1) << 8));
            // response[2] = ((GET32(EMMC_RESP1) >> 24) | (GET32(EMMC_RESP2) << 8)); 
            // response[3] = ((GET32(EMMC_RESP2) >> 24) | (GET32(EMMC_RESP3) << 8));
            response[0] = GET32(EMMC_RESP0);
            response[1] = GET32(EMMC_RESP1);
            response[2] = GET32(EMMC_RESP2);
            response[3] = GET32(EMMC_RESP3);
            break;
        case EMMC_CMDTM_RESP_48: 
            response[0] = GET32(EMMC_RESP0); 
            break; 
        case EMMC_CMDTM_RESP_48BUSY: 
            response[0] = GET32(EMMC_RESP0);
            #if EMMC_DEBUG
                printk("resp48b, resp=%x\n", response[0]); 
            #endif 
            interrupt_reg = emmc_wait_for_data(EMMC_INTERRUPT_DATA_DONE, 3000);
            if ((interrupt_reg & EMMC_INTERRUPT_DATA_DONE) == 0) {
                printk("CMD%d has R1b response, but it's too busy so that it times out... interrupt_reg = %x\n", command, interrupt_reg);
            }
            if (interrupt_reg & EMMC_INTERRUPT_ERR) {
                printk("CMD%d has R1b response, but an error has occurred! interrupt_reg = %x\n", command, interrupt_reg);
            }
            #if EMMC_DEBUG
                printk("resp48b, status = %x interrupt = %x\n", GET32(EMMC_STATUS), interrupt_reg); 
            #endif 
            break; 
        case EMMC_CMDTM_RESP_NONE: 
            response[0] = GET32(EMMC_RESP0); 
            break; 
        default: 
            printk("CMD%d, cmdtm = %x, unknown response type!\n", command, cmdtm);
            return EMMC_R_CMD_BAD_ERR;
    }
    switch (command) {
        case EMMC_CMD_SEND_IF_COND: 
            if (response[0] != argument) {
                printk("CMD send_if_cond must have response match the argument!\n"); 
                return EMMC_R_ERR; 
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
            // delay_ms(1); 
            // emmc_set_sd_freq(); 
            // delay_ms(1);
            break; 
        case EMMC_ACMD_SET_BUS_WIDTH: 
            if (emmc.appcmd) {
                // do we need argument & 0x3? 
                if (argument == 0) {
                    emmc_write_register(EMMC_CONTROL0, GET32(EMMC_CONTROL0) & ~EMMC_CTRL0_HCTL_DWIDTH4);
                } else if (argument == 2) {
                    emmc_write_register(EMMC_CONTROL0, GET32(EMMC_CONTROL0) | EMMC_CTRL0_HCTL_DWIDTH4);
                }
            } else {
                // from plan 9 
                if ((argument & 0x8000000F) == 0x80000001) {
                    delay_ms(1); emmc_set_sdhc_freq(); delay_ms(1);
                }
            }
            break;
        case EMMC_CMD_IO_RW_DIRECT: 
            // Func0 = Card Common IO Area 
            // Address 7 = Card Common Control Register: busifc 
            // Data 2 = Bus width = 4 
            // (1<<CMD52_RW_FLAG_SHIFT|0<<CMD52_FUNC_NUM_SHIFT|7<<CMD52_REG_ADDR_SHIFT)
            if ((argument & ~CMD52_WRITE_DATA_MASK) == cmd52_args_assemble(1, 0, 0, 7, 0)) {
                // setting bus width 
                if ((argument & 0b11) == 0) {
                    emmc_write_register(EMMC_CONTROL0, GET32(EMMC_CONTROL0) & ~EMMC_CTRL0_HCTL_DWIDTH4);
                } else if ((argument & 0b11) == 2) {
                    emmc_write_register(EMMC_CONTROL0, GET32(EMMC_CONTROL0) | EMMC_CTRL0_HCTL_DWIDTH4);
                }
            }
            break; 
        default: 
            break; 
    }
    emmc.appcmd = (command == EMMC_CMD_APP_CMD);
    return EMMC_R_OK;
}

void emmc_pio_setup(unsigned block_size, unsigned block_count) {
    emmc_write_register(EMMC_BLKSIZECNT, (block_count << 16) | (block_size & 0xFFFF));
}

int emmc_perform_pio(unsigned is_write, uint32_t *buffer, unsigned block_size, unsigned block_count) {
    unsigned idx = 0; 
    if (!is_write) {
        #if EMMC_DEBUG
            printk("waiting for data\n");
        #endif 
        for (unsigned b = 0; b < block_count; ++b) {
            uint32_t interrupt_reg = emmc_wait_for_interrupt(EMMC_INTERRUPT_READ_RDY, 3000); 
            // printk("data received, interrupt_reg = %x\n", interrupt_reg);
            if (interrupt_reg & EMMC_INTERRUPT_ERR) {
                printk("PIO error, status=%x, interrupt=%x\n", GET32(EMMC_STATUS), interrupt_reg); 
                return EMMC_R_ERR; 
            }
            for (int i = 0; i < block_size; ++i) {
                buffer[idx] = GET32(EMMC_DATA); 
                idx += 1; 
            }
        }
    } else {
        for (unsigned b = 0; b < block_count; ++b) {
            uint32_t interrupt_reg = emmc_wait_for_interrupt(EMMC_INTERRUPT_WRITE_RDY, 3000); 
            if (interrupt_reg & EMMC_INTERRUPT_ERR) {
                printk("PIO error, status=%x, interrupt=%x\n", GET32(EMMC_STATUS), interrupt_reg); 
                return EMMC_R_ERR; 
            }
            for (int i = 0; i < block_size; ++i) {
                PUT32(EMMC_DATA, buffer[idx]);
                idx += 1; 
            }
        }
    }
    return EMMC_R_OK;
}

void emmc_dma_setup(unsigned block_size, unsigned block_count) {
    emmc_write_register(EMMC_BLKSIZECNT, (block_count << 16) | (block_size & 0xFFFF));
}

int emmc_perform_dma(unsigned is_write, uint8_t *buffer, int length) {
    // if using DMA, then it MUST be using multiblock transmission, otherwise it might hang 
    if (is_write) {
        // DMA write 
        memcpy(emmc.dma_buffer, buffer, length);
        flush_all_caches(); dev_barrier();
        dma_start(EMMC_DMA_CHANNEL, EMMC_DMA_DEVICE, DMA_MEM2DEV, emmc.dma_buffer, (void*)(EMMC_DATA), length);
        // dma_start(EMMC_DMA_CHANNEL, EMMC_DMA_DEVICE, DMA_MEM2DEV, buffer, (void*)(EMMC_DATA), length);
    } else {
        // DMA read 
        dma_start(EMMC_DMA_CHANNEL, EMMC_DMA_DEVICE, DMA_DEV2MEM, (void*)EMMC_DATA, emmc.dma_buffer, length);
        // dma_start(EMMC_DMA_CHANNEL, EMMC_DMA_DEVICE, DMA_DEV2MEM, (void*)EMMC_DATA, buffer, length);
    }
    flush_all_caches(); dev_barrier();
    // DMA wait  
    if (dma_wait(EMMC_DMA_CHANNEL) == 0) {
        printk("EMMC DMA failed!\n");
        return EMMC_R_ERR;
    }
    uint32_t interrupt_reg = emmc_wait_for_data(EMMC_INTERRUPT_DATA_DONE, 3000);
    if ((interrupt_reg & EMMC_INTERRUPT_DATA_DONE) == 0) {
        printk("EMMC DMA transmission timeout! write=%d, interrupt_reg=%x, status=%x\n", 
            is_write, interrupt_reg, GET32(EMMC_STATUS)); 
        return EMMC_R_DATA_TIMEOUT; 
    }
    if (interrupt_reg & EMMC_INTERRUPT_ERR) {
        printk("EMMC DMA transmission failed! write=%d, interrupt_reg=%x, status=%x\n", 
            is_write, interrupt_reg, GET32(EMMC_STATUS)); 
        return EMMC_R_ERR; 
    }

    if (!is_write) {
        memcpy(buffer, emmc.dma_buffer, length);
        flush_all_caches(); dev_barrier();
    }
    return EMMC_R_OK;
}




int emmc_enable_card_interrupt(card_interrupt_handler_t handler) {
    emmc.card_handler = handler; 
    emmc_write_register(EMMC_IRPT_EN, GET32(EMMC_IRPT_EN) | EMMC_IRPT_MASK_CARD_INTERRUPT);  
    dev_barrier(); 
    return EMMC_R_OK; 
}
int emmc_disable_card_interrupt() {
    emmc_write_register(EMMC_IRPT_EN, GET32(EMMC_IRPT_EN) & (~EMMC_IRPT_MASK_CARD_INTERRUPT)); 
    dev_barrier(); 
    return EMMC_R_OK; 
}


void emmc_dump_regs(const char *msg) {
#if EMMC_DEBUG
    printk("-----------------------------------------\n");
    printk("%s dump regs:\n", msg); 
    uint32_t arg2 = GET32(EMMC_ARG2); 
    printk("arg2 = %x\n", arg2); 
    uint32_t blksizecnt = GET32(EMMC_BLKSIZECNT); 
    printk("blksizecnt = %x\n", blksizecnt); 
    uint32_t arg1 = GET32(EMMC_ARG1); 
    printk("arg1 = %x\n", arg1); 
    uint32_t cmdtm = GET32(EMMC_CMDTM); 
    printk("cmdtm = %x\n", cmdtm); 
    uint32_t resp0 = GET32(EMMC_RESP0); 
    printk("resp0 = %x\n", resp0); 
    uint32_t resp1 = GET32(EMMC_RESP1); 
    printk("resp1 = %x\n", resp1); 
    uint32_t resp2 = GET32(EMMC_RESP2); 
    printk("resp2 = %x\n", resp2); 
    uint32_t resp3 = GET32(EMMC_RESP3); 
    printk("resp3 = %x\n", resp3); 
    // uint32_t data = GET32(EMMC_DATA); 
    // printk("data = %x\n", data); 
    uint32_t status = GET32(EMMC_STATUS); 
    printk("status = %x\n", status); 
    uint32_t control0 = GET32(EMMC_CONTROL0); 
    printk("control0 = %x\n", control0); 
    uint32_t control1 = GET32(EMMC_CONTROL1); 
    printk("control1 = %x\n", control1); 
    uint32_t interrupt = GET32(EMMC_INTERRUPT); 
    printk("interrupt = %x\n", interrupt); 
    uint32_t irpt_mask = GET32(EMMC_IRPT_MASK); 
    printk("irpt_mask = %x\n", irpt_mask); 
    uint32_t irpt_en = GET32(EMMC_IRPT_EN); 
    printk("irpt_en = %x\n", irpt_en); 
    uint32_t control2 = GET32(EMMC_CONTROL2); 
    printk("control2 = %x\n", control2); 
    #if 0 
    uint32_t force_irpt = GET32(EMMC_FORCE_IRPT); 
    printk("force_irpt = %x\n", force_irpt); 
    uint32_t boot_timeout = GET32(EMMC_BOOT_TIMEOUT); 
    printk("boot_timeout = %x\n", boot_timeout); 
    uint32_t dbg_sel = GET32(EMMC_DBG_SEL); 
    printk("dbg_sel = %x\n", dbg_sel); 
    uint32_t exrdfifo_cfg = GET32(EMMC_EXRDFIFO_CFG); 
    printk("exrdfifo_cfg = %x\n", exrdfifo_cfg); 
    uint32_t exrdfifo_en = GET32(EMMC_EXRDFIFO_EN); 
    printk("exrdfifo_en = %x\n", exrdfifo_en); 
    uint32_t tune_step = GET32(EMMC_TUNE_STEP); 
    printk("tune_step = %x\n", tune_step); 
    uint32_t tune_steps_std = GET32(EMMC_TUNE_STEPS_STD); 
    printk("tune_step_std = %x\n", tune_steps_std); 
    uint32_t tune_steps_ddr = GET32(EMMC_TUNE_STEPS_DDR); 
    printk("tune_step_ddr = %x\n", tune_steps_ddr); 
    uint32_t spi_int_spt = GET32(EMMC_SPI_INT_SPT); 
    printk("spi_int_spt = %x\n", spi_int_spt); 
    #endif 
    uint32_t slotisr_ver = GET32(EMMC_SLOTISR_VER); 
    printk("slotisr_ver = %x\n", slotisr_ver); 
    printk("-----------------------------------------\n");
#endif  // ! EMMC_DEBUG 
}

