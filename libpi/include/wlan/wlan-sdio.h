#ifndef WLAN_SDIO_H 
#define WLAN_SDIO_H 
#include "emmc/emmc.h" 

struct SDIOState {
    uint32_t rca;  // relative card address 
    // NOTE: SDIO card does not have SCR registers! 
    uint32_t ocr;  // operation condition register 
};
extern struct SDIOState sdio_state; 
#define SDIO_FN2 2 
#define SDIO_FN1 1
#define SDIO_FN0 0
#define SDIO_FN_BACKPLANE SDIO_FN1 
#define SDIO_FN_WLAN SDIO_FN2 
#define SDIO_FN_SDIO SDIO_FN0 
#define SDIO_BACKPLANE_BASE 0x100 
#define SDIO_WLAN_BASE 0x200 
#define SDIO_BACKPLANE_BLKSIZE 0x110 
#define SDIO_WLAN_BLKSIZE 0x210 
#define SDIO_R_OK EMMC_R_OK 
#define SDIO_R_ERR EMMC_R_ERR

// /* SEND_OP_COND args */
// Hcs	= 1<<30,	/* host supports SDHC & SDXC */
// V3_3	= 3<<20,	/* 3.2-3.4 volts */
// V2_8	= 3<<15,	/* 2.7-2.9 volts */
// V2_0	= 1<<8,		/* 2.0-2.1 volts */
// S18R	= 1<<24,	/* switch to 1.8V request */

#define SDIO_OP_HCS (1 << 30) 
#define SDIO_OP_V3_3 (3 << 20) 
#define SDIO_OP_V2_8 (3 << 15) 
#define SDIO_OP_V2_0 (1 << 8) 
#define SDIO_OP_SWITCH_V1_8_REQ (1 << 24) 
// /* CCCR */
// Ioenable	= 0x02,
// Ioready		= 0x03,
// Intenable	= 0x04,
// Intpend		= 0x05,
// Ioabort		= 0x06,
// Busifc		= 0x07,
// Capability	= 0x08,
// Blksize		= 0x10,
// Highspeed	= 0x13,
#define SDIO_CCCR_IO_ENABLE 0x02 
#define SDIO_CCCR_IO_READY 0x03 
#define SDIO_CCCR_INT_ENABLE 0x04 
#define SDIO_CCCR_INT_PENDING 0x05 
#define SDIO_CCCR_IO_ABORT 0x06 
#define SDIO_CCCR_BUSIFC 0x07 
#define SDIO_CCCR_CAPABILITY 0x08 
#define SDIO_CCCR_BLOCK_SIZE 0x10 
#define SDIO_CCCR_HIGH_SPEED 0x13 
/// NOTE: CCCR = card common control register 
#define SDIO_WRITE_RETRY 10 

int sdio_cmd(uint32_t cmd, uint32_t arg);

int sdio_read(uint32_t fn, uint32_t addr);

int sdio_write(uint32_t fn, uint32_t addr, uint32_t data);

int sdio_set(uint32_t fn, uint32_t addr, uint32_t bits);

int sdio_perform_io(uint32_t is_write, uint32_t fn, uint32_t addr, uint8_t *buffer, uint32_t num_bytes, uint32_t addr_increment);

int set_ether_func_gpio();

int sdio_init(void);

int sdio_reset(void); 
int sdio_abort(unsigned fn); 
#endif // ! WLAN_SDIO_H 


