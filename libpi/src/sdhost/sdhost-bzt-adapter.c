#include "sdhost/sdhost-bzt-adapter.h"
#include "libc/assert.h"
#include "sdhost/sdhost.h"
static uint32_t sd_relative_addr; 
static uint32_t scr_buffer[8];
static uint32_t sd_scr[2]; 
static uint32_t resp[4];
static int cmd(uint32_t cmd, uint32_t arg, uint32_t *resp) {
    // printk("CMD%d, arg=%x, sending\n", cmd, arg);
    resp[0] = resp[1] = resp[2] = resp[3] = 0x0;
    uint32_t success = sdhost_send_command(cmd, arg, resp);
    // printk("CMD%d, arg=%x: %s, resp=%x\n", cmd, arg, (success==0) ? "success" : "failed", resp[0]);
    int cnt = 0; 
    while (success != 0) {
        cnt += 1; 
        resp[0] = resp[1] = resp[2] = resp[3] = 0x0;
        printk("prev command failed, retrying... cnt=%d\n", cnt);
        delay_ms(10);
        success = sdhost_send_command(cmd, arg, resp);
    }
    return resp[0];
}
int sdhost_sd_init() {
    memset(scr_buffer, 0xFF, 8 * sizeof(uint32_t)); 
    #if SDHOST_USE_DMA 
    dma_init(); 
    #endif 
    sdhost_init(SD_SET_SDCARD_GPIO);
    cmd(EMMC_CMD_GO_IDLE_STATE, 0, resp); 
    cmd(EMMC_CMD_SEND_IF_COND, 0x000001AA, resp); 
    // for some reason, ACMD41 will timeout occasionally... 
    int cnt=6; int r=0; 
    while(!(r & ACMD41_CMD_COMPLETE) && cnt--) {
        delay_cycles(400);
        cmd(EMMC_CMD_APP_CMD, 0, resp); 
        cmd(EMMC_ACMD_SD_APP_OP_COND, ACMD41_ARG_HC, resp);
        r = resp[0];
        printk("SDHOST SD: CMD_SEND_OP_COND returned resp0=%x ", r);
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
    // uint32_t ccs = EMMC_SCR_SUPP_CCS; 
    cmd(EMMC_CMD_ALL_SEND_CID, 0, resp); 
    printk("resp0=%x, resp1=%x, resp2=%x, resp3=%x\n", resp[0], resp[1], resp[2], resp[3]); 
    cmd(EMMC_CMD_SEND_RELATIVE_ADDR, 0, resp); 
    sd_relative_addr = resp[0] & EMMC_CMD_RCA_MASK; 
    printk("sd relative addr = %x\n", sd_relative_addr); 
    cmd(EMMC_CMD_SELECT_CARD, sd_relative_addr, resp);  // this is a busy command  
    sdhost_io_setup(8, 1); 
    cmd(EMMC_CMD_APP_CMD, sd_relative_addr, resp); 
    cmd(EMMC_ACMD_SEND_SCR, 0, resp); 
    sdhost_perform_io(0, (uint8_t*)scr_buffer, 2*sizeof(uint32_t)); 
    printk("scr buffer: ");
    for (unsigned i = 0; i < 2; ++i) {
        printk("%x ", scr_buffer[i]);
    }
    printk("\n");
    printk("sd rca = %x\n", sd_relative_addr); 
    if (scr_buffer[0] & EMMC_SCR_SD_BUS_WIDTH_4) {
        cmd(EMMC_CMD_APP_CMD, sd_relative_addr, resp); 
        cmd(EMMC_ACMD_SET_BUS_WIDTH, sd_relative_addr|2, resp); 
    }

    printk("SDHOST: supports ");
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
int sdhost_sd_readblock(unsigned int lba, unsigned char *buffer, unsigned int num) {
    printk("sdhost_sd_readblock(lba=%x, buf=%p, num=%u)\n", lba, buffer, num); 
    if (num < 1) {
        num = 1; 
    }
    // if (num > 1) {
    //     cmd(EMMC_CMD_SET_BLOCK_COUNT, num, resp);
    // }
    // sdhost_io_setup(512, 1); 
    // uint32_t *buf = (uint32_t *)buffer;
    // for (unsigned i = 0; i < num; ++i) {
    //     cmd(EMMC_CMD_READ_SINGLE_BLOCK, lba+i, resp); 
    //     int ret = sdhost_perform_io(0, (uint8_t*)buf, 512); 
    //     if (ret != 0) {
    //         return 0; 
    //     }
    //     buf += 128;
    // }
    // return num * 512; 
    if (!(sd_scr[0] & EMMC_SCR_SUPP_CCS)) {
        panic("let's simplify the problem by assuming that the card supports multiblock transfer...\n"); 
    }
    uint32_t blocks_remaining =  num; 
    uint32_t bcount = 0; 
    const uint32_t max_block = 1; // SDHOST_MAX_IO_SIZE / 512; 
    while (blocks_remaining > 0) {
        if (blocks_remaining > max_block) {
            bcount = max_block; 
        } else {
            bcount = blocks_remaining; 
        }
        // printk("blocks_remaining=%d, bcount=%d\n", blocks_remaining, bcount); 
        if (bcount > 1) {
            cmd(EMMC_CMD_SET_BLOCK_COUNT, bcount, resp);
        }
        sdhost_io_setup(512, bcount); 
        cmd(bcount == 1 ? EMMC_CMD_READ_SINGLE_BLOCK : EMMC_CMD_READ_MULTIPLE_BLOCK, (uint32_t)lba, resp);
        int ret = sdhost_perform_io(0, buffer, bcount*512); 
        if (ret != 0) {
            return 0; 
        }
        buffer += (bcount*512);
        blocks_remaining -= bcount; 
        lba += bcount; 
    }
    // if (num > 1) {
    //     cmd(EMMC_CMD_SET_BLOCK_COUNT, num, resp);
    // }
    // sdhost_io_setup(512, num); 
    // cmd(num == 1 ? EMMC_CMD_READ_SINGLE_BLOCK : EMMC_CMD_READ_MULTIPLE_BLOCK, (uint32_t)lba, resp);
    // int ret = sdhost_perform_io(0, buffer, num*512); 
    // if (ret != 0) {
    //     return 0; 
    // }
    return num*512; 
#if 0
    printk("sdhci readblock\n"); 
    int c = 0;
    if (num < 1) { 
        num = 1;
    }
    unsigned int *buf = (unsigned int *)buffer;
    if (sd_scr[0] & EMMC_SCR_SUPP_CCS) {
        if (num > 1 && (sd_scr[0] & EMMC_SCR_SUPP_SET_BLKCNT)) {
            cmd(EMMC_CMD_SET_BLOCK_COUNT, num, resp);
        }
        // PUT32(EMMC_BLKSIZECNT, (num << 16) | 512); 
        sdhost_io_setup(512, num); 
        cmd(num == 1 ? EMMC_CMD_READ_SINGLE_BLOCK : EMMC_CMD_READ_MULTIPLE_BLOCK, (uint32_t)lba, resp);
    } else {
        // PUT32(EMMC_BLKSIZECNT, (1 << 16) | 512);
        sdhost_io_setup(512, 1); 
    }
    // emmc_perform_dma(0, buffer, num*512); 
    while (c < num) {
        if (!(sd_scr[0] & EMMC_SCR_SUPP_CCS)) {
            cmd(EMMC_CMD_READ_SINGLE_BLOCK, (lba+c)*512, resp);
        }
        uint32_t r = sdhost_wait_for_interrupt(SDHOST_SDHSTS_BUSY_IRPT, 3000); 
        if (r == 0) {
            printk("sd_readblock: data timeout!\n"); 
            return 0; 
        }
        if (r & SDHOST_SDHSTS_ERROR_MASK) {
            printk("sd_readblock: error waiting data\n");
            return 0; 
        }
        for (unsigned d = 0; d < 128; d++) {
            buf[d] = GET32(SDHOST_BASE+SDHOST_SDDATA); 
        }
        // uint32_t r = emmc_wait_for_data(EMMC_INTERRUPT_READ_RDY, 3000); 
        // if (r & EMMC_INTERRUPT_ERR) {
        //     printk("sd_readblock: error waiting data\n");
        //     return 0;
        // }
        // if (!(r & EMMC_INTERRUPT_READ_RDY)) {
        //     printk("sd_readblock: data timeout!\n"); 
        //     return 0; 
        // }
        // for (unsigned d = 0; d < 128; d++) {
        //     buf[d] = GET32(EMMC_DATA);
        // } 
        c++; 
        buf += 128;
    }
    if (num > 1 && !(sd_scr[0] & EMMC_SCR_SUPP_SET_BLKCNT) && (sd_scr[0] & EMMC_SCR_SUPP_CCS)) {
        cmd(EMMC_CMD_STOP_TRANSMISSION, 0, resp);
    }
    return (c != num) ? 0 : (num * 512);
#endif 
}
int sdhost_sd_writeblock(unsigned char *buffer, unsigned int lba, unsigned int num) {
    panic("sdhost_sd_writeblock(buffer=%p, lba=%x, num=%d): currently not-yet supported!!!\n", buffer, lba, num); 
#if 0 
    if (num < 1) {
        num = 1; 
    }
    if (!(sd_scr[0] & EMMC_SCR_SUPP_CCS)) {
        panic("let's simplify the problem by assuming that the card supports multiblock transfer...\n"); 
    }
    if (num > 1) {
        cmd(EMMC_CMD_SET_BLOCK_COUNT, num, resp);
    }
    sdhost_io_setup(512, num); 
    // PUT32(EMMC_BLKSIZECNT, (num << 16) | 512); 
    cmd(num == 1 ? EMMC_CMD_WRITE_BLOCK : EMMC_CMD_WRITE_MULTIPLE_BLOCK, lba, resp);
    int ret = sdhost_perform_io(1, buffer, num*512); 
    if (ret != 0) {
        return 0; 
    }
    return num*512; 
#endif 
    return 0; 
}




