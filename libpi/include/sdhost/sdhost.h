#ifndef SDHOST_H 
#define SDHOST_H 
#include "rpi.h"
#include "emmc/emmc-cmds.h"

#define SDHOST_BASE (0x20000000 + 0x202000)
/// external frequency = 250MHz
#define SDHOST_EXTERNAL_FREQ 250000000 
/// init frequency = 400kHz 
#define SDHOST_INIT_FREQ 400000 
/// SD frequency = 25MHz 
#define SDHOST_SD_FREQ 25000000 
/// SDHC frequency = 50MHz 
#define SDHOST_SDHC_FREQ 50000000 
/// limit FIFO usage due to silicon bug -- linux driver 
#define SDHOST_FIFO_READ_THRESHOLD    4
#define SDHOST_FIFO_WRITE_THRESHOLD    4

/// Linux driver 
#define SDHOST_SDCMD  0x00 /* Command to SD card              - 16 R/W */
#define SDHOST_SDARG  0x04 /* Argument to SD card             - 32 R/W */
#define SDHOST_SDTOUT 0x08 /* Start value for timeout counter - 32 R/W */
#define SDHOST_SDCDIV 0x0c /* Start value for clock divider   - 11 R/W */
#define SDHOST_SDRSP0 0x10 /* SD card response (31:0)         - 32 R   */
#define SDHOST_SDRSP1 0x14 /* SD card response (63:32)        - 32 R   */
#define SDHOST_SDRSP2 0x18 /* SD card response (95:64)        - 32 R   */
#define SDHOST_SDRSP3 0x1c /* SD card response (127:96)       - 32 R   */
#define SDHOST_SDHSTS 0x20 /* SD host status                  - 11 R/W */
#define SDHOST_SDVDD  0x30 /* SD card power control           -  1 R/W */
#define SDHOST_SDEDM  0x34 /* Emergency Debug Mode            - 13 R/W */
#define SDHOST_SDHCFG 0x38 /* Host configuration              -  2 R/W */
#define SDHOST_SDHBCT 0x3c /* Host byte count (debug)         - 32 R/W */
#define SDHOST_SDDATA 0x40 /* Data to/from SD card            - 32 R/W */
#define SDHOST_SDHBLC 0x50 /* Host block count (SDIO/SDHC)    -  9 R/W */

/// Linux driver 
#define SDHOST_SDCMD_NEW_FLAG            0x8000
#define SDHOST_SDCMD_FAIL_FLAG           0x4000
#define SDHOST_SDCMD_BUSYWAIT            0x800
#define SDHOST_SDCMD_NO_RESPONSE         0x400
/// NOTE: the 48-bit response is (0 << 9) 
#define SDHOST_SDCMD_NORMAL_RESPONSE     0x000 
#define SDHOST_SDCMD_LONG_RESPONSE       0x200
#define SDHOST_SDCMD_RESPONSE_MASK       (7<<9)
/// write_cmd = host to card, read_cmd = card to host 
#define SDHOST_SDCMD_WRITE_CMD           0x80
#define SDHOST_SDCMD_READ_CMD            0x40
#define SDHOST_SDCMD_CMD_MASK            0x3f

#define SDHOST_SDCDIV_MAX_CDIV           0x7ff

#define SDHOST_SDHSTS_BUSY_IRPT          0x400
#define SDHOST_SDHSTS_BLOCK_IRPT         0x200
#define SDHOST_SDHSTS_SDIO_IRPT          0x100
#define SDHOST_SDHSTS_REW_TIME_OUT       0x80
#define SDHOST_SDHSTS_CMD_TIME_OUT       0x40
#define SDHOST_SDHSTS_CRC16_ERROR        0x20
#define SDHOST_SDHSTS_CRC7_ERROR         0x10
#define SDHOST_SDHSTS_FIFO_ERROR         0x08
/* Reserved */
/* Reserved */
#define SDHOST_SDHSTS_DATA_FLAG          0x01
#define SDHOST_MAX_IO_SIZE 131072  
/// read_multiblock sometimes doesn't work, figuring out why... 
/// so we need to fall back to single block, but using DMA in single-block mode is painfully slow!!! 
#define SDHOST_USE_DMA 0 

#define SDHOST_SDHSTS_TRANSFER_ERROR_MASK    (SDHOST_SDHSTS_CRC7_ERROR | \
    SDHOST_SDHSTS_CRC16_ERROR | \
    SDHOST_SDHSTS_REW_TIME_OUT | \
    SDHOST_SDHSTS_FIFO_ERROR)

#define SDHOST_SDHSTS_ERROR_MASK        (SDHOST_SDHSTS_CMD_TIME_OUT | \
    SDHOST_SDHSTS_TRANSFER_ERROR_MASK)


#ifndef BIT 
#define BIT(x) (1 << (x))
#endif  // ! BIT 

#define SDHOST_SDHCFG_BUSY_IRPT_EN    BIT(10)
#define SDHOST_SDHCFG_BLOCK_IRPT_EN   BIT(8)
#define SDHOST_SDHCFG_SDIO_IRPT_EN    BIT(5)
#define SDHOST_SDHCFG_DATA_IRPT_EN    BIT(4)
#define SDHOST_SDHCFG_SLOW_CARD       BIT(3)
#define SDHOST_SDHCFG_WIDE_EXT_BUS    BIT(2)
#define SDHOST_SDHCFG_WIDE_INT_BUS    BIT(1)
#define SDHOST_SDHCFG_REL_CMD_LINE    BIT(0)

#define SDHOST_SDVDD_POWER_OFF        0
#define SDHOST_SDVDD_POWER_ON         1

#define SDHOST_SDEDM_FORCE_DATA_MODE  BIT(19)
#define SDHOST_SDEDM_CLOCK_PULSE      BIT(20)
#define SDHOST_SDEDM_BYPASS           BIT(21)

#define SDHOST_SDEDM_WRITE_THRESHOLD_SHIFT    9
#define SDHOST_SDEDM_READ_THRESHOLD_SHIFT     14
#define SDHOST_SDEDM_THRESHOLD_MASK           0x1f

#define SDHOST_SDEDM_FSM_MASK         0xf
#define SDHOST_SDEDM_FSM_IDENTMODE    0x0
#define SDHOST_SDEDM_FSM_DATAMODE     0x1
#define SDHOST_SDEDM_FSM_READDATA     0x2
#define SDHOST_SDEDM_FSM_WRITEDATA    0x3
#define SDHOST_SDEDM_FSM_READWAIT     0x4
#define SDHOST_SDEDM_FSM_READCRC      0x5
#define SDHOST_SDEDM_FSM_WRITECRC     0x6
#define SDHOST_SDEDM_FSM_WRITEWAIT1   0x7
#define SDHOST_SDEDM_FSM_POWERDOWN    0x8
#define SDHOST_SDEDM_FSM_POWERUP      0x9
#define SDHOST_SDEDM_FSM_WRITESTART1  0xa
#define SDHOST_SDEDM_FSM_WRITESTART2  0xb
#define SDHOST_SDEDM_FSM_GENPULSES    0xc
#define SDHOST_SDEDM_FSM_WRITEWAIT2   0xd
#define SDHOST_SDEDM_FSM_STARTPOWDOWN 0xf

#define SDHOST_SDDATA_FIFO_WORDS      16

// #define SDHOST_FIFO_READ_THRESHOLD     4
// #define SDHOST_FIFO_WRITE_THRESHOLD    4
#define SDHOST_SDDATA_FIFO_PIO_BURST  8

#define SDHOST_PIO_THRESHOLD          1  /* Maximum block count for PIO (0 = always DMA) */

#define SDHOST_DMA_DEVICE 13   /// the DMA DREQ number for the SDHOST 
#define SDHOST_DMA_CHANNEL 2   /// different than EMMC_DMA_CHANNEL(4)
struct SDHostStatus {
    uint32_t block_count; // block or byte count 
    uint32_t done;        // is the command done? 
    uint32_t extclock;    // external clock 
    uint32_t appcmd;      // are we in appcmd state? 
    uint32_t fast_clock;  // what's the current clock state (0=init, 1=sd, 2=sdhc)
};

extern struct SDHostStatus sdhost_status; 

uint32_t sdhost_get_sdcmd_mask(uint32_t cmd); 
/** 
 * The SDCDIV register has 11 bits, and holds (div - 2).  But
 * in data mode the max is 50MHz wihout a minimum, and only
 * the bottom 3 bits are used. Since the switch over is
 * automatic (unless we have marked the card as slow...),
 * chosen values have to make sense in both modes.  Ident mode
 * must be 100-400KHz, so can range check the requested
 * clock. CMD15 must be used to return to data mode, so this
 * can be monitored.
 *
 * clock 250MHz -> 0->125MHz, 1->83.3MHz, 2->62.5MHz, 3->50.0MHz
 *                 4->41.7MHz, 5->35.7MHz, 6->31.3MHz, 7->27.8MHz
 *
 *         623->400KHz/27.8MHz
 *         reset value (507)->491159/50MHz
 *
 * BUT, the 3-bit clock divisor in data mode is too small if
 * the core clock is higher than 250MHz, so instead use the
 * SLOW_CARD configuration bit to force the use of the ident
 * clock divisor at all times.
 * -- Linux driver 
 */
void sdhost_set_init_freq(); 
void sdhost_set_sd_freq(); 
void sdhost_set_sdhc_freq(); 

int sdhost_init(int set_gpio); 
int sdhost_reset(); 
int sdhost_send_command(uint32_t command, uint32_t argument, uint32_t *response); 
uint32_t sdhost_wait_for_interrupt(uint32_t interrupt, uint32_t timeout); 
int sdhost_io_setup(uint32_t block_size, uint32_t block_count); 
int sdhost_perform_io(unsigned is_write, uint8_t *buffer, uint32_t buffer_size); 
#endif  // SDHOST_H 

