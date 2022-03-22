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
#define EMMC_BUFFER_MAX_SIZE 131072 
#define EMMC_BUFFER_ALIGNMENT 4 
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
void emmc_dump_regs(const char *msg); 
/// for debuging 
int emmc_test_clock_dividers(void *arg); 
uint32_t emmc_cmdtm_get_masks(uint32_t cmd); 

#define EMMC_CMDTM_UNSUPPORTED 0


#endif // ! EMMC_H






