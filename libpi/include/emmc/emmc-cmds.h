#ifndef EMMC_CMDS_H 
#define EMMC_CMDS_H 
#include <stdint.h>
/**
 * Check out https://www.sdcard.org/downloads/pls/ for the SD commands
 */
enum EMMC_CMD_LIST {
    EMMC_CMD_GO_IDLE_STATE = 0,  // SDIO Mandatory 
    // there's no CMD1 
    EMMC_CMD_ALL_SEND_CID = 2, // not supported by SDIO
    EMMC_CMD_SEND_RELATIVE_ADDR = 3,  // SDIO Mandatory 
    EMMC_CMD_SET_DSR = 4, // not supported by SDIO 
    EMMC_CMD_IO_SEND_OP_COND = 5,  // SDIO only 
    EMMC_CMD_SWITCH_FUNC = 6, // doesn't seem to support SDIO? 
    EMMC_CMD_SELECT_CARD = 7,  // SDIO Mandatory 
    EMMC_CMD_SEND_IF_COND = 8, // optional 
    EMMC_CMD_SEND_CSD = 9,  // not supported by SDIO 
    EMMC_CMD_SEND_CID = 10, // not supported by SDIO
    EMMC_CMD_VOLTAGE_SWITCH = 11,  // optional 
    EMMC_CMD_STOP_TRANSMISSION = 12,  // doesn't seem to support SDIO? 
    EMMC_CMD_SEND_STATUS = 13, 
    // no CMD 14 
    EMMC_CMD_GO_INACTIVE_STATE = 15,  // SDIO Mandatory 
    EMMC_CMD_SET_BLOCKLEN = 16, 
    EMMC_CMD_READ_SINGLE_BLOCK = 17, 
    EMMC_CMD_READ_MULTIPLE_BLOCK = 18, 
    EMMC_CMD_SEND_TUNING_BLOCK = 19,  // optional 
    // no CMD 20, 21, 22
    EMMC_CMD_SET_BLOCK_COUNT = 23, 
    EMMC_CMD_WRITE_BLOCK = 24, 
    EMMC_CMD_WRITE_MULTIPLE_BLOCK = 25, 
    // no CMD 26 
    EMMC_CMD_PROGRAM_CSD = 27,  // not supported by SDIO 
    EMMC_CMD_SET_WRITE_PROT = 28, 
    EMMC_CMD_CLR_WRITE_PROT = 29, 
    EMMC_CMD_SEND_WRITE_PROT = 30, 
    // no CMD 31 
    EMMC_CMD_ERASE_WR_BLK_START = 32, 
    EMMC_CMD_ERASE_WR_BLK_END = 33, 
    // no CMD 34, 35, 36, 37 
    EMMC_CMD_ERASE = 38, 
    // no CMD 39, 40,  
    EMMC_CMD_LOCK_UNLOCK = 42, 
    // no CMD 43, 44, 45, 46, 47, 48, 49, 50,  
    EMMC_CMD_IO_RW_DIRECT = 52,  // SDIO Only 
    EMMC_CMD_IO_RW_EXTENDED = 53,  // SDIO Only 
    // no CMD 54 
    EMMC_CMD_APP_CMD = 55, 
    EMMC_CMD_GEN_CMD = 56, 

    /// the card select command 
    /// just an alias 
    EMMC_CMD_MMC_SELECT = 7,  
    /// CMD6: switch func, ACMD6: set bus width 
    EMMC_ACMD_SET_BUS_WIDTH = 6, 
    EMMC_ACMD_SEND_SCR = 51, 
    EMMC_ACMD_SD_APP_OP_COND = 41
};
#define EMMC_CMD_RCA_MASK 0xffff0000
#define EMMC_CMD_ERRORS_MASK 0xfff9c004
/// please match the EMMC_CMD_LIST to SDIO_CMD_LIST 
/**
 * Check out https://www.sdcard.org/downloads/pls/ for the SDIO commands
 */
enum SDIO_CMD_LIST {
    /// NOTE: this is NOT go idle state, 
    /// sending CMD0 to SDIO card will toggle switching SD and SPI modes 
    SDIO_CMD_GO_IDLE_STATE = 0,  // SDIO Mandatory 
    // there's no CMD1 
    SDIO_UNSUPPORTED_ALL_SEND_CID = 2, // not supported by SDIO
    SDIO_CMD_SEND_RELATIVE_ADDR = 3,  // SDIO Mandatory 
    SDIO_UNSUPPORTED_SET_DSR = 4, // not supported by SDIO 
    SDIO_CMD_IO_SEND_OP_COND = 5,  // SDIO Mandatory 
    SDIO_UNKNOWN_SWITCH_FUNC = 6, // doesn't seem to support SDIO? 
    SDIO_CMD_SELECT_CARD = 7,  // SDIO Mandatory 
    SDIO_CMD_SEND_IF_COND = 8, // optional 
    SDIO_UNSUPPORTED_SEND_CSD = 9,  // not supported by SDIO 
    SDIO_UNSUPPORTED_SEND_CID = 10, // not supported by SDIO
    SDIO_CMD_VOLTAGE_SWITCH = 11,  // optional 
    SDIO_UNKNOWN_STOP_TRANSMISSION = 12,  // doesn't seem to support SDIO? 
    SDIO_UNKNOWN_SEND_STATUS = 13, 
    // no CMD 14 
    SDIO_CMD_GO_INACTIVE_STATE = 15,  // SDIO Mandatory 
    SDIO_UNKNOWN_SET_BLOCKLEN = 16, 
    SDIO_UNKNOWN_READ_SINGLE_BLOCK = 17, 
    SDIO_UNKNOWN_READ_MULTIPLE_BLOCK = 18, 
    SDIO_CMD_SEND_TUNING_BLOCK = 19,  // optional 
    // no CMD 20, 21, 22
    SDIO_UNKNOWN_SET_BLOCK_COUNT = 23, 
    SDIO_UNKNOWN_WRITE_BLOCK = 24, 
    SDIO_UNKNOWN_WRITE_MULTIPLE_BLOCK = 25, 
    // no CMD 26 
    SDIO_UNSUPPORTED_PROGRAM_CSD = 27,  // not supported by SDIO 
    SDIO_UNKNOWN_SET_WRITE_PROT = 28, 
    SDIO_UNKNOWN_CLR_WRITE_PROT = 29, 
    SDIO_UNKNOWN_SEND_WRITE_PROT = 30, 
    // no CMD 31 
    SDIO_UNKNOWN_ERASE_WR_BLK_START = 32, 
    SDIO_UNKNOWN_ERASE_WR_BLK_END = 33, 
    // no CMD 34, 35, 36, 37 
    SDIO_UNKNOWN_ERASE = 38, 
    // no CMD 39, 40, 41 
    SDIO_UNKNOWN_LOCK_UNLOCK = 42, 
    // no CMD 43, 44, 45, 46, 47, 48, 49, 50, 51 
    SDIO_CMD_IO_RW_DIRECT = 52,  // SDIO Mandatory 
    SDIO_CMD_IO_RW_EXTENDED = 53,  // SDIO Mandatory 
    // no CMD 54 
    SDIO_UNKNOWN_APP_CMD = 55, 
    SDIO_UNKNOWN_GEN_CMD = 56, 

};

//  GO_IDLE_STATE       = 0,  // resp = no
//  SEND_RELATIVE_ADDR  = 3,  // resp = r6 
//  IO_SEND_OP_COND     = 5,  // resp = r4 
//  SELECT_CARD         = 7,  // resp = r1b 
//  VOLTAGE_SWITCH      = 11, // resp = ?
//  IO_RW_DIRECT        = 52, // resp = r5 
//  IO_RW_EXTENDED      = 53, // resp = r5 
/*
	Indexshift		= 24,
	Suspend			= 1<<22,
	Resume			= 2<<22,
	Abort			= 3<<22,
	Isdata			= 1<<21,
	Ixchken			= 1<<20,
	Crcchken		= 1<<19,
	Respmask		= 3<<16,
	Respnone		= 0<<16,
	Resp136			= 1<<16,
	Resp48			= 2<<16,
	Resp48busy		= 3<<16,
	Multiblock		= 1<<5,
	Host2card		= 0<<4,
	Card2host		= 1<<4,
	Autocmd12		= 1<<2,
	Autocmd23		= 2<<2,
	Blkcnten		= 1<<1,
*/

enum EMMC_CMDTM_REG_MASKS {
    EMMC_CMDTM_SUSPEND      = (1 << 22), 
    EMMC_CMDTM_RESUME       = (2 << 22), 
    EMMC_CMDTM_ABORT        = (3 << 22), 
    EMMC_CMDTM_ISDATA       = (1 << 21), 
    EMMC_CMDTM_IXCHK_EN     = (1 << 20), 
    EMMC_CMDTM_CRCCHK_EN    = (1 << 19), 
    EMMC_CMDTM_RESP_MASK    = (3 << 16), 
    EMMC_CMDTM_RESP_NONE    = (0 << 16), 
    EMMC_CMDTM_RESP_136     = (1 << 16), 
    EMMC_CMDTM_RESP_48      = (2 << 16), 
    EMMC_CMDTM_RESP_48BUSY  = (3 << 16), 
    EMMC_CMDTM_MULTI_BLOCK  = (1 << 5), 
    EMMC_CMDTM_HOST_TO_CARD = (0 << 4), 
    EMMC_CMDTM_CARD_TO_HOST = (1 << 4), 
    EMMC_CMDTM_AUTO_CMD12   = (1 << 2), // sends CMD12 after completion
    EMMC_CMDTM_AUTO_CMD23   = (2 << 2), // sends CMD23 after completion 
    EMMC_CMDTM_BLKCNT_EN    = (1 << 1), 
    EMMC_CMDTM_CMD_INDEX_SHIFT = 24,  // the field corresponding the index 

};
#define ACMD41_VOLTAGE      0x00ff8000
#define ACMD41_CMD_COMPLETE 0x80000000
#define ACMD41_CMD_CCS      0x40000000
#define ACMD41_ARG_HC       0x51ff8000

#define CMD52_WRITE_DATA_MASK (0xFF) 
#define CMD52_RW_FLAG_SHIFT 31
#define CMD52_FUNC_NUM_SHIFT 28 
#define CMD52_REG_ADDR_SHIFT 9 
#define CMD53_RW_FLAG_SHIFT 31 
#define CMD53_FUNC_NUM_SHIFT 28 
#define CMD53_REG_ADDR_SHIFT 9 
/**
 * CMD53 structure: 
 * S - 1 bit 
 * D - 1 bit 
 * Command index - 6 bits (0b110100)
 * Argument begin: Arg bits 31 - 0 
 *   R(0)/W(1) Flag - 1 bit (31)
 *   Function Number - 3 bits (30, 29, 28), func 0 to select the common io area 
 *   Block Mode - 1 bit (27), block mode 1 to read in block basis rather than byte basis 
 *   Op Code - 1 bit (26), 0 = multibyte RW to fixed address, 1 = multibyte RW to incrementing address 
 *   Register Address - 17 bits (25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9)
 *   Byte/Block Count - 9 bits (8, 7, 6, 5, 4, 3, 2, 1, 0)
 * Argument end. 
 * CRC7 - 7 bits 
 * E - 1 bit
 */
static inline uint32_t cmd53_args_assemble(
    uint32_t iswrite, 
    uint32_t func_num, 
    uint32_t block_mode, 
    uint32_t op_code, 
    uint32_t register_addr, 
    uint32_t byte_or_block_count
) {
    return (
        ((iswrite & 1) << 31) | 
        ((func_num & 0b111) << 28) | 
        ((block_mode & 1) << 27) | 
        ((op_code & 1) << 26) | 
        ((register_addr & 0x1FFFF) << 9) | 
        (byte_or_block_count & 0x1FF)
    );
}

/**
 * CMD52 structure: 
 * S - 1 bit 
 * D - 1 bit 
 * Command index - 6 bits (0b110100)
 * Argument begin: Arg bits 31 - 0 
 *   R(0)/W(1) Flag - 1 bit (31)
 *   Function Number - 3 bits (30, 29, 28), func 0 to select the common io area 
 *   RAW/Read After Write Flag - 1 bit (27)
 *   Stuff - 1 bit (26)
 *   Register Address - 17 bits (25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9)
 *   Stuff - 1 bit (8)
 *   Write Data or Stuff - 8 bits (7, 6, 5, 4, 3, 2, 1, 0)
 * Argument end. 
 * CRC7 - 7 bits 
 * E - 1 bit
 */
static inline uint32_t cmd52_args_assemble(
    uint32_t iswrite, 
    uint32_t func_num, 
    uint32_t is_read_after_write, 
    uint32_t register_addr, 
    uint32_t write_data
) {
    return (
        ((iswrite & 1) << 31) | 
        ((func_num & 0b111) << 28) | 
        ((is_read_after_write & 1) << 27) | 
        ((register_addr & 0x1FFFF) << 9) | 
        (write_data & 0xFF)
    );
}



#endif // ! EMMC_CMDS_H 















