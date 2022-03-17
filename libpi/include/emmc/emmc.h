/**
 * A raw EMMC driver for BCM2835, 
 * Supports: 
 * - DMA data transfer 
 * - PIO data transfer 
 * - SDIO commands 
 * - (Most of the) SD commands 
 */
/// set this to 1 to enable verbose mode (WARNING: VERY VERBOSE)
#define EMMC_DEBUG 0 
#define EMMC_USE_INTERRUPT 0 
/// NOTE: if you are using interrupt, remember in no circumstance can you enable card interrupt!!! 
/// because the WLAN card keeps triggering the card interrupt, though nothing is there!!!  
/*
Sources: 
    https://elixir.bootlin.com/linux/latest/source/drivers/mmc/core/sdio_io.c#L315
    https://elixir.bootlin.com/linux/latest/source/drivers/mmc/core/core.c#L217
    https://elixir.bootlin.com/linux/latest/source/drivers/mmc/core/sdio_ops.c#L114
    https://elixir.bootlin.com/linux/latest/source/drivers/mmc/core/sdio_io.c#L471
    https://elixir.bootlin.com/linux/latest/source/drivers/mmc/core/sdio_io.c#L485
    https://9p.io/sources/contrib/miller/9/bcm/ether4330.c
    https://9p.io/sources/contrib/miller/9/bcm/emmc.c
    https://forums.raspberrypi.com/viewtopic.php?t=94133
    https://github.com/moizumi99/RPiHaribote/blob/master/haribote/sdcard.c
    https://github.com/LdB-ECM/Raspberry-Pi/blob/master/GLES2_Model/SDCard.c
    https://elinux.org/RPi_BCM2835_GPIOs
    https://android.googlesource.com/kernel/common/+/bcmdhd-3.10/drivers/net/wireless/bcmdhd/bcmsdh.c 
    https://android.googlesource.com/kernel/common/+/bcmdhd-3.10/drivers/net/wireless/bcmdhd/bcmsdh_sdmmc.c
    https://github.com/torvalds/linux/blob/master/include/linux/mmc/sdio_func.h 
    https://github.com/torvalds/linux/blob/master/drivers/net/wireless/broadcom/brcm80211/brcmfmac/sdio.h
    https://github.com/torvalds/linux/blob/master/drivers/net/wireless/broadcom/brcm80211/brcmfmac/sdio.c
    https://github.com/torvalds/linux/tree/master/drivers/net/wireless/broadcom/brcm80211/brcmfmac
    https://github.com/torvalds/linux/blob/master/drivers/net/wireless/broadcom/brcm80211/brcmfmac/Makefile
    https://iosoft.blog/2020/03/27/zerowi-part5/ 
    https://forums.raspberrypi.com/viewtopic.php?t=279789 
    https://github.com/rsta2/circle/blob/master/include/circle/bcm2835.h 
    https://github.com/rsta2/circle/blob/master/lib/dmachannel.cpp 
    https://github.com/rsta2/circle/blob/master/lib/dma4channel.cpp 
    https://github.com/rsta2/circle/blob/master/include/circle/dmachannel.h 
    https://github.com/rsta2/circle/blob/master/include/circle/dmacommon.h 
    https://github.com/rsta2/circle/blob/master/include/circle/sysconfig.h 
    https://github.com/rsta2/circle/tree/master/include/circle 
    https://github.com/jncronin/rpi-boot/blob/master/emmc.c 
*/
#ifndef EMMC_H
#define EMMC_H
#if EMMC_USE_INTERRUPT 
#include "interrupts/interrupts.h"
#endif 
#include "emmc/emmc-regs.h"
#include "emmc/emmc-control.h"
#include "emmc/emmc-cmds.h" 
#include "dma/dma.h"

#define EMMC_R_OK 0 
#define EMMC_R_ERR -1 
#define EMMC_R_CMD_INHIBIT -2 
#define EMMC_R_CMD_TIMEOUT -3 
#define EMMC_R_CMD_CRC_ERR -4 
#define EMMC_R_CMD_END_ERR -5 
#define EMMC_R_CMD_BAD_ERR -6 
#define EMMC_R_DATA_INHIBIT -7 
#define EMMC_R_DATA_TIMEOUT -8 
#define EMMC_R_DATA_CRC_ERR -9 
#define EMMC_R_DATA_END_ERR -10 
#define EMMC_R_ACMD_ERR -11 
typedef int(*card_interrupt_handler_t)(uint32_t); 
/// the state of the emmc controller 
struct EMMCState {
    uint32_t interrupt_val;  // holds the interrupt register value after the interrupt
    uint32_t use_fastclock;  // whether to use the fast clock, 0 = 400KHz, 1 = 25MHz, 2 = 50MHz 
    uint32_t extclock;  // external clock, the emmc controller clock rate 
    uint32_t appcmd;    // is appcmd already sent? 
    card_interrupt_handler_t card_handler; // the interrupt handler for card 
    uint8_t *dma_buffer;  // the dma buffer 
    struct MBoxMallocState dma_buffer_handle;  // to manage the dma buffer 
};
/// the state of the emmc controller 
extern struct EMMCState emmc; 

/// initializes the emmc controller 
int emmc_init(void);
/// the interrupt handler for emmc controller 
void emmc_interrupt_handler(unsigned pc, void *args); 
/// writes to emmc register 
void emmc_write_register(unsigned reg, uint32_t val); 
/// wait for data to complete, with the dedicated interrupt mask, timeout is not handled (TODO)
int emmc_wait_for_data(uint32_t interrupt_mask, uint32_t timeout); 
/// wait for command to complete, timeout is not handled (TODO)
int emmc_wait_for_cmd(uint32_t timeout); 
/// wait for interrupt, with the dedicated interrupt mask, timeout is not handled (TODO)
int emmc_wait_for_interrupt(uint32_t interrupt_mask, uint32_t timeout); 
/// sends a command, ** NOTE: response must have size >= 4 ** 
int emmc_send_command(uint32_t command, uint32_t argument, uint32_t *response); 
/// NOTE: NOT TESTED!!! the linux driver does not use DMA either!!! 
void emmc_pio_setup(unsigned block_size, unsigned block_count); 
/// NOTE: NOT TESTED!!! the linux driver does not use DMA either!!! 
int emmc_perform_pio(unsigned is_write, uint32_t *buffer, unsigned block_size, unsigned block_count);
/// set up the block size and block count of the emmc DMA IO 
void emmc_dma_setup(unsigned block_size, unsigned block_count); 
/// performs data transmission using DMA 
/// the datasheet somehow says we need to use multiblock on DMA, but actually we can use single block 
int emmc_perform_dma(unsigned is_write, uint8_t *buffer, int length); 
/// enables card interrupt and set its handler 
int emmc_enable_card_interrupt(card_interrupt_handler_t handler); 
/// disables the card interrupt 
int emmc_disable_card_interrupt();
/// sets the frequency to EMMC_SD_FREQUENCY 
int emmc_set_sd_freq(); 
/// sets the frequency to EMMC_INIT_FREQUENCY 
int emmc_set_init_freq();
/// sets the frequency to EMMC_SD_HIGH_FREQUENCY 
int emmc_set_sdhc_freq(); 
/// dumps the emmc controller state for debugging 
static inline void emmc_dump_regs(const char *msg) {
    return;
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
}
/// for debuging 
int emmc_test_clock_dividers(void *arg); 
//  CMD0:  GO_IDLE_STATE       = 0,  // resp = no? 
//  CMD3:  SEND_RELATIVE_ADDR  = 3,  // resp = r6 
//  CMD5:  IO_SEND_OP_COND     = 5,  // resp = r4 
//  CMD7:  SELECT_CARD         = 7,  // resp = r1b 
//  CMD11: VOLTAGE_SWITCH      = 11, // resp = ?
//  CMD52: IO_RW_DIRECT        = 52, // resp = r5 
//  CMD53: IO_RW_EXTENDED      = 53, // resp = r5 
#ifndef emmc_cmdtm_masks_macro
#define emmc_cmdtm_masks_macro
#define EMMC_CMDTM_UNSUPPORTED 0
static const uint32_t emmc_cmdtm_masks[64] = {
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

#endif // ! EMMC_H






