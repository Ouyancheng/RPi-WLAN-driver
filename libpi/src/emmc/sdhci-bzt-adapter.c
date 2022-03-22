/**
 * An adapter for the emmc driver to be compatible to the bzt-sd library interface 
 */ 
#include "emmc/sdhci-bzt-adapter.h" 
static uint32_t sd_relative_addr; 
static uint32_t scr_buffer[8];
static uint32_t sd_scr[2]; 
static uint32_t resp[4];
/**
 * This function is adapted from bzt's sdcard driver, 
 * it basically sets the pins for the sdcard, but these should be the default config... 
 */
void sdhci_sd_set_sdcard_gpio() {
    // GPIO_CD
    uint32_t r = GET32(GPIO_FSEL4); r&=~(7<<(7*3)); PUT32(GPIO_FSEL4, r); 
    PUT32(GPIO_PUD, 2); delay_us(150); 
    PUT32(GPIO_PUDCLK1, (1<<15)); delay_us(150); 
    PUT32(GPIO_PUD, 0); PUT32(GPIO_PUDCLK1, 0); delay_us(150); 
    r = GET32(GPIO_HEN1); r|=1<<15; PUT32(GPIO_HEN1, r); 
    // GPIO_CLK, GPIO_CMD
    r = GET32(GPIO_FSEL4); r|=(7<<(8*3))|(7<<(9*3)); PUT32(GPIO_FSEL4, r); 
    PUT32(GPIO_PUD, 2); delay_us(150); 
    PUT32(GPIO_PUDCLK1, (1<<16)|(1<<17)); delay_us(150); 
    PUT32(GPIO_PUD, 0); PUT32(GPIO_PUDCLK1, 0);  delay_us(150);
    // GPIO_DAT0, GPIO_DAT1, GPIO_DAT2, GPIO_DAT3 
    r = GET32(GPIO_FSEL5); r|=(7<<(0*3)) | (7<<(1*3)) | (7<<(2*3)) | (7<<(3*3)); PUT32(GPIO_FSEL5, r); 
    PUT32(GPIO_PUD, 2); delay_us(150); 
    PUT32(GPIO_PUDCLK1, (1<<18) | (1<<19) | (1<<20) | (1<<21)); delay_us(150); 
    PUT32(GPIO_PUD, 0); PUT32(GPIO_PUDCLK1, 0);  delay_us(150);
}
static int cmd(uint32_t cmd, uint32_t arg, uint32_t *resp) {
    resp[0] = resp[1] = resp[2] = resp[3] = 0x0;
    uint32_t success = emmc_send_command(cmd, arg, resp);
    // printk("CMD%d, arg=%x: %s, resp=%x\n", cmd, arg, (success==EMMC_R_OK) ? "success" : "failed", resp[0]);
    return resp[0];
}
int sdhci_sd_init() {
    memset(scr_buffer, 0xFF, 8 * sizeof(uint32_t)); 
#if SD_SET_SDCARD_GPIO 
    sdhci_sd_set_sdcard_gpio();
#endif 
    dma_init();
    printk("dma init-ed\n");

    emmc_dump_regs("Before init/enable"); 

    emmc_init();

    emmc_dump_regs("After init/enable"); 

    cmd(EMMC_CMD_GO_IDLE_STATE, 0, resp); 

    emmc_dump_regs("After CMD0");

    cmd(EMMC_CMD_SEND_IF_COND, 0x000001AA, resp); 

    emmc_dump_regs("After if cond"); 

    int cnt=6; int r=0; 
    while(!(r & ACMD41_CMD_COMPLETE) && cnt--) {
        delay_cycles(400);
        cmd(EMMC_CMD_APP_CMD, 0, resp); 
        cmd(EMMC_ACMD_SD_APP_OP_COND, ACMD41_ARG_HC, resp);
        r = resp[0];
        printk("EMMC: CMD_SEND_OP_COND returned resp0=%x ", r);
        if(r & ACMD41_CMD_COMPLETE)
            printk("COMPLETE ");
        if(r & ACMD41_VOLTAGE)
            printk("VOLTAGE ");
        if(r & ACMD41_CMD_CCS)
            printk("CCS ");
        printk("resp0=%x\n", r);
    }
    uint32_t ccs = 0; 
    // printk("complete? = %x\n", r & ACMD41_CMD_COMPLETE); 
    if (r & ACMD41_CMD_CCS) {
        ccs = EMMC_SCR_SUPP_CCS; 
    }

    emmc_dump_regs("After ACMD41");

    cmd(EMMC_CMD_ALL_SEND_CID, 0, resp); 
    printk("resp0=%x, resp1=%x, resp2=%x, resp3=%x\n", resp[0], resp[1], resp[2], resp[3]); 

    emmc_dump_regs("After sending CID");

    cmd(EMMC_CMD_SEND_RELATIVE_ADDR, 0, resp); 
    sd_relative_addr = resp[0] & EMMC_CMD_RCA_MASK; 
    printk("sd relative addr = %x\n", sd_relative_addr); 
    
    emmc_dump_regs("After getting RCA");

    // emmc_set_sd_freq(); 
    
    emmc_dump_regs("Before setting SD freq");

    // printk("setting sd freq = %x\n", emmc_set_sd_freq());
    cmd(EMMC_CMD_SELECT_CARD, sd_relative_addr, resp);  // this is a busy command  
    
    emmc_dump_regs("After select card"); 

    // emmc_write_register(EMMC_BLKSIZECNT, (1 << 16) | 8); // NOTE: this MUST come before any command issued for send SCR!!! 
    emmc_dma_setup(8, 1); 
    cmd(EMMC_CMD_APP_CMD, sd_relative_addr, resp);  // send rca as argument 

    emmc_dump_regs("After app cmd"); 
    
    printk("acmd_send_scr_cmdtm = %x\n", emmc_cmdtm_get_masks(EMMC_ACMD_SEND_SCR) | (EMMC_ACMD_SEND_SCR << EMMC_CMDTM_CMD_INDEX_SHIFT));
    cmd(EMMC_ACMD_SEND_SCR, 0, resp); 
    
    emmc_dump_regs("After send SCR");    
    
    // emmc_perform_pio(0, scr_buffer, 2); 
    emmc_perform_dma(0, (uint8_t*)scr_buffer, 2*sizeof(uint32_t)); 
    printk("scr buffer: ");
    for (unsigned i = 0; i < 2; ++i) {
        printk("%x ", scr_buffer[i]);
    }
    printk("\n");

    printk("sd rca = %x\n", sd_relative_addr); 
    if (scr_buffer[0] & EMMC_SCR_SD_BUS_WIDTH_4) {
        cmd(EMMC_CMD_APP_CMD, sd_relative_addr, resp); 
        cmd(EMMC_ACMD_SET_BUS_WIDTH, sd_relative_addr|2, resp); 
        PUT32(EMMC_CONTROL0, GET32(EMMC_CONTROL0) | EMMC_CTRL0_HCTL_DWIDTH4); 
    }

    printk("EMMC: supports ");
    if(scr_buffer[0] & EMMC_SCR_SUPP_SET_BLKCNT)
        printk("SET_BLKCNT ");
    if(ccs)
        printk("CCS ");
    printk("\n");

    scr_buffer[0] &= ~EMMC_SCR_SUPP_CCS; 
    scr_buffer[0] |= ccs; 

    for (unsigned i = 0; i < 2; ++i) {
        sd_scr[i] = scr_buffer[i]; 
    }

    printk("Init success!!!!!!\n"); 
    return SD_OK;
}


int sdhci_sd_readblock(unsigned int lba, unsigned char *buffer, unsigned int num) {
    // printk("sdhci_sd_readblock(lba=%x, buf=%p, num=%u)\n", lba, buffer, num); 
#if SDHCI_USE_DMA 
    if (num < 1) {
        num = 1; 
    }
    if (GET32(EMMC_STATUS) & EMMC_STATUS_DATA_INHIBIT) {
        printk("sd_readblock: Data inhibit!\n"); 
        return 0; 
    }
    if (!(sd_scr[0] & EMMC_SCR_SUPP_CCS)) {
        panic("let's simplify the problem by assuming that the card supports multiblock transfer...\n"); 
    }
    
    uint32_t blocks_remaining =  num; 
    uint32_t bcount = 0; 
    const uint32_t max_block = EMMC_BUFFER_MAX_SIZE / 512; 
    while (blocks_remaining > 0) {
        if (blocks_remaining > max_block) {
            bcount = max_block; 
        } else {
            bcount = blocks_remaining; 
        }
        if (bcount > 1) {
            cmd(EMMC_CMD_SET_BLOCK_COUNT, bcount, resp);
        }
        emmc_dma_setup(512, bcount); 
        cmd(bcount == 1 ? EMMC_CMD_READ_SINGLE_BLOCK : EMMC_CMD_READ_MULTIPLE_BLOCK, (uint32_t)lba, resp);
        int ret = emmc_perform_dma(0, buffer, bcount*512); 
        if (ret != EMMC_R_OK) {
            return 0; 
        }
        buffer += (bcount*512);
        blocks_remaining -= bcount; 
        lba += bcount; 
    }
    return num*512; 
#else 
    printk("sdhci readblock\n"); 
    int c = 0;
    if (num < 1) { 
        num = 1;
    }
    if (GET32(EMMC_STATUS) & EMMC_STATUS_DATA_INHIBIT) {
        printk("sd_readblock: Data inhibit!\n"); 
        return 0; 
    }
    unsigned int *buf = (unsigned int *)buffer;
    if (sd_scr[0] & EMMC_SCR_SUPP_CCS) {
        if (num > 1 && (sd_scr[0] & EMMC_SCR_SUPP_SET_BLKCNT)) {
            cmd(EMMC_CMD_SET_BLOCK_COUNT, num, resp);
        }
        PUT32(EMMC_BLKSIZECNT, (num << 16) | 512); 
        cmd(num == 1 ? EMMC_CMD_READ_SINGLE_BLOCK : EMMC_CMD_READ_MULTIPLE_BLOCK, (uint32_t)lba, resp);
    } else {
        PUT32(EMMC_BLKSIZECNT, (1 << 16) | 512);
    }
    // emmc_perform_dma(0, buffer, num*512); 
    while (c < num) {
        if (!(sd_scr[0] & EMMC_SCR_SUPP_CCS)) {
            cmd(EMMC_CMD_READ_SINGLE_BLOCK, (lba+c)*512, resp);
        }
        uint32_t r = emmc_wait_for_data(EMMC_INTERRUPT_READ_RDY, 3000); 
        if (r & EMMC_INTERRUPT_ERR) {
            printk("sd_readblock: error waiting data\n");
            return 0;
        }
        if (!(r & EMMC_INTERRUPT_READ_RDY)) {
            printk("sd_readblock: data timeout!\n"); 
            return 0; 
        }
        for (unsigned d = 0; d < 128; d++) {
            buf[d] = GET32(EMMC_DATA);
        } 
        c++; 
        buf += 128;
    }
    if (num > 1 && !(sd_scr[0] & EMMC_SCR_SUPP_SET_BLKCNT) && (sd_scr[0] & EMMC_SCR_SUPP_CCS)) {
        cmd(EMMC_CMD_STOP_TRANSMISSION, 0, resp);
    }
    return (c != num) ? 0 : (num * 512);
#endif 
}

int sdhci_sd_writeblock(unsigned char *buffer, unsigned int lba, unsigned int num) {
#if SDHCI_USE_DMA 
    if (num < 1) {
        num = 1; 
    }
    if (GET32(EMMC_STATUS) & EMMC_STATUS_DATA_INHIBIT) {
        printk("sd_writeblock: Data inhibit!\n"); 
        return 0; 
    }
    if (!(sd_scr[0] & EMMC_SCR_SUPP_CCS)) {
        panic("let's simplify the problem by assuming that the card supports multiblock transfer...\n"); 
    }
    
    uint32_t blocks_remaining =  num; 
    uint32_t bcount = 0; 
    const uint32_t max_block = EMMC_BUFFER_MAX_SIZE / 512; 
    while (blocks_remaining > 0) {
        if (blocks_remaining > max_block) {
            bcount = max_block; 
        } else {
            bcount = blocks_remaining; 
        }
        if (bcount > 1) {
            cmd(EMMC_CMD_SET_BLOCK_COUNT, bcount, resp);
        }
        emmc_dma_setup(512, bcount); 
        cmd(bcount == 1 ? EMMC_CMD_WRITE_BLOCK : EMMC_CMD_WRITE_MULTIPLE_BLOCK, (uint32_t)lba, resp);
        int ret = emmc_perform_dma(1, buffer, bcount*512); 
        if (ret != EMMC_R_OK) {
            return 0; 
        }
        buffer += (bcount*512);
        blocks_remaining -= bcount; 
        lba += bcount; 
    }
    return num*512; 
    // if (GET32(EMMC_STATUS) & EMMC_STATUS_DATA_INHIBIT) {
    //     printk("sd_writeblock: Data inhibit!\n"); 
    //     return 0; 
    // }
    // if (!(sd_scr[0] & EMMC_SCR_SUPP_CCS)) {
    //     panic("let's simplify the problem by assuming that the card supports multiblock transfer...\n"); 
    // }
    // if (num > 1) {
    //     cmd(EMMC_CMD_SET_BLOCK_COUNT, num, resp);
    // }
    // emmc_dma_setup(512, num); 
    // // PUT32(EMMC_BLKSIZECNT, (num << 16) | 512); 
    // cmd(num == 1 ? EMMC_CMD_WRITE_BLOCK : EMMC_CMD_WRITE_MULTIPLE_BLOCK, lba, resp);
    // int ret = emmc_perform_dma(1, buffer, num*512); 
    // if (ret != EMMC_R_OK) {
    //     return 0; 
    // }
    // return num*512; 
#else  
    int c = 0;
    if (num < 1) {
        num = 1;
    }
    if (GET32(EMMC_STATUS) & EMMC_STATUS_DATA_INHIBIT) {
        printk("sd_writeblock: Data inhibit!\n"); 
        return 0;
    }
    unsigned int *buf = (unsigned int *)buffer;
    if (sd_scr[0] & EMMC_SCR_SUPP_CCS) {
        if (num > 1 && (sd_scr[0] & EMMC_SCR_SUPP_CCS)) {
            cmd(EMMC_CMD_SET_BLOCK_COUNT, num, resp);
        }
        PUT32(EMMC_BLKSIZECNT, (num << 16) | 512); 
        cmd(num == 1 ? EMMC_CMD_WRITE_BLOCK : EMMC_CMD_WRITE_MULTIPLE_BLOCK, lba, resp);
    } else {
        PUT32(EMMC_BLKSIZECNT, (1 << 16) | 512); 
    }
    while (c < num) {
        if(!(sd_scr[0] & EMMC_SCR_SUPP_CCS)) {
            cmd(EMMC_CMD_WRITE_BLOCK, (lba+c)*512, resp);
        }
        uint32_t r = emmc_wait_for_data(EMMC_INTERRUPT_WRITE_RDY, 3000); 
        if ((r & EMMC_INTERRUPT_WRITE_RDY) == 0) {
            printk("sd_writeblock: timeout waiting for write ready!\n"); 
            return 0; 
        }
        if (r & EMMC_INTERRUPT_ERR) {
            printk("sd_writeblock: error waiting for write ready!\n"); 
            return 0; 
        }
        for(unsigned d = 0; d < 128; d++) {
            PUT32(EMMC_DATA, buf[d]); 
        }
        c++; 
        buf += 128;
    }
    uint32_t interrupt_reg = emmc_wait_for_data(EMMC_INTERRUPT_DATA_DONE, 3000); 
    if (interrupt_reg & EMMC_INTERRUPT_ERR) {
        printk("sd_writeblock: error waiting for data done\n"); 
        return 0; 
    }
    if (!(interrupt_reg & EMMC_INTERRUPT_DATA_DONE)) {
        printk("sd_writeblock: timeout waiting for data done\n"); 
        return 0; 
    }
    if (num > 1 && !(sd_scr[0] & EMMC_SCR_SUPP_SET_BLKCNT) && (sd_scr[0] & EMMC_SCR_SUPP_CCS)) {
        cmd(EMMC_CMD_STOP_TRANSMISSION, 0, resp);
    }
    return (c != num) ? 0 : (num * 512);
#endif 
}





