#ifndef EMMC_REGS_H
#define EMMC_REGS_H 



/**
 * The EMMC registers, see the bcm2835 datasheet, chapter 5 
 */
#define EMMC_REG_BASE (0x20000000 + 0x300000)
/// ACMD23 Argument
#define EMMC_ARG2 (EMMC_REG_BASE + 0x00)
/// Block Size and Count
#define EMMC_BLKSIZECNT (EMMC_REG_BASE + 0x04)
/// Argument
#define EMMC_ARG1 (EMMC_REG_BASE + 0x08)
/// Command and Transfer Mode
#define EMMC_CMDTM (EMMC_REG_BASE + 0x0C)
/// Response bits 31 : 0
#define EMMC_RESP0 (EMMC_REG_BASE + 0x10)
/// Response bits 63 : 32
#define EMMC_RESP1 (EMMC_REG_BASE + 0x14)
/// Response bits 95 : 64 
#define EMMC_RESP2 (EMMC_REG_BASE + 0x18)
/// Response bits 127 : 96
#define EMMC_RESP3 (EMMC_REG_BASE + 0x1C)
/// Data
#define EMMC_DATA (EMMC_REG_BASE + 0x20)
/// Status 
#define EMMC_STATUS (EMMC_REG_BASE + 0x24)
/// Host Configuration bits
#define EMMC_CONTROL0 (EMMC_REG_BASE + 0x28) 
/// Host Configuration bits
#define EMMC_CONTROL1 (EMMC_REG_BASE + 0x2C) 
/// Interrupt Flags 
#define EMMC_INTERRUPT (EMMC_REG_BASE + 0x30) 
/// Interrupt Flag Enable 
#define EMMC_IRPT_MASK (EMMC_REG_BASE + 0x34)
/// Interrupt Generation Enable 
#define EMMC_IRPT_EN (EMMC_REG_BASE + 0x38) 
/// Host Configuration bits 
#define EMMC_CONTROL2 (EMMC_REG_BASE + 0x3C) 
/// Force Interrupt Event
#define EMMC_FORCE_IRPT (EMMC_REG_BASE + 0x50) 
/// Timeout in boot mode 
#define EMMC_BOOT_TIMEOUT (EMMC_REG_BASE + 0x70) 
/// Debug Bus Configuration 
#define EMMC_DBG_SEL (EMMC_REG_BASE + 0x74) 
/// Extension FIFO Configuration 
#define EMMC_EXRDFIFO_CFG (EMMC_REG_BASE + 0x80) 
/// Extension FIFO Enable 
#define EMMC_EXRDFIFO_EN (EMMC_REG_BASE + 0x84) 
/// Delay per card clock tuning step 
#define EMMC_TUNE_STEP (EMMC_REG_BASE + 0x88) 
/// Card clock tuning steps for SDR 
#define EMMC_TUNE_STEPS_STD (EMMC_REG_BASE + 0x8C) 
/// Card clock tuning steps for DDR 
#define EMMC_TUNE_STEPS_DDR (EMMC_REG_BASE + 0x90) 
/// SPI Interrupt Support 
#define EMMC_SPI_INT_SPT (EMMC_REG_BASE + 0xF0) 
/// Slot Interrupt Status and Version 
#define EMMC_SLOTISR_VER (EMMC_REG_BASE + 0xFC) 




#endif // ! EMMC_REGS_H 


