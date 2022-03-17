#ifndef WLAN_BACKPLANE_H 
#define WLAN_BACKPLANE_H 
#include "wlan/wlan-sdio.h"

#define ROUNDUP(val, num)	(((val)+(num)-1) / (num) * (num))
#define ROUND(val, num)		ROUNDUP((val), (num))

#define BACKPLANE_BASE_ADDR 0x18000000 
#define BACKPLANE_32BIT_ADDR_BASE 0x8000
#define BACKPLANE_RESET_CTRL 0x800
#define BACKPLANE_IO_CTRL 0x408 
// the registers for the soc ram core 
#define SOCRAM_COREINFO 0x0 
#define SOCRAM_BANKIDX 0x10 
#define SOCRAM_BANKINFO 0x40 
// ??? 
#define SOCRAM_BANKPDA 0x44 
// the backplane registers alone? 
#define BACKPLANE_CLK_CSR 0x1000e 
#define BACKPLANE_PULLUP 0x1000f 
#define BACKPLANE_WINDOW_SIZE 0x8000 
#define BACKPLANE_WINDOW_MASK (BACKPLANE_WINDOW_SIZE-1)
#define BACKPLANE_WINDOW_ADDR_REG 0x1000a
// backplane registers offsets, please add the BASE_ADDR offset 
#define BACKPLANE_GPIO_PULLUP 0x58 
#define BACKPLANE_GPIO_PULLDOWN 0x5C 
#define BACKPLANE_CHIP_CTRL_ADDR 0x650 
#define BACKPLANE_CHIP_CTRL_DATA 0x654 
// SDIO core registers offsets, please add the sdregs offset!!! 
#define SDCORE_INT_MASK 0x24 
// the interrupt bits in SDIO core interrupt register 
#define SDCORE_INT_FRAME_INT (1<<6)
#define SDCORE_INT_MBOX_INT (1<<7)
#define SDCORE_INT_FCC_CHANGE (1<<5)
#define SDCORE_INT_STATUS (0x20) 
// the mailbox? 
#define SDCORE_BACKPLANE_MBOX (0x40) 
#define SDCORE_HOST_MBOX_DATA (0x4c)
#define SDCORE_BACKPLANE_MBOX_DATA (0x48) 
/// plan 9 
struct WLANDeviceInfo {
    /// the backplane base address BACKPLANE_BASE_ADDR 0x18000000 
    uint32_t chipcommon;
    /// arm core tag 
    uint32_t armcore; 
    /// the arm core control register base 
    uint32_t armctl;
    /// the arm core register base 
    uint32_t armregs;
    /// the d11 core control register 
    uint32_t d11ctl;
    /// the ram core register base 
    uint32_t socramregs;
    /// the ram core control register base 
    uint32_t socramctl;
    /// the SDIO core register base 
    uint32_t sdregs;
    /// the SDIO revision 
    int sdiorev;
    /// the ram core revision 
    int socramrev;
    /// the ram size 
    uint32_t socramsize;
    /// the starting address of the ram 
    uint32_t rambase;
    uint16_t reqid;
    uint8_t fcmask;
    uint8_t txwindow;
    uint8_t txseq;
    uint8_t rxseq;
};
/// the device 
extern struct WLANDeviceInfo device; 
/// literally &device 
extern struct WLANDeviceInfo *ctl; 
// Cortex M3 
#define ARMcm3 0x82A 
#define ARM7tdmi 0x825 
#define ARMcr4 0x83E 
// serializes int16 in little endian... 
static inline uint8_t *serialize_int16(uint8_t *p, uint16_t v) {
    p[0] = (v & 0xFF);
    p[1] = ((v >> 8) & 0xFF);
    return p + 2;
}
// serializes int32 in little endian... 
static inline uint8_t *serialize_int32(uint8_t *p, uint32_t v) {
    p[0] = (v & 0xFF);
    p[1] = ((v >> 8) & 0xFF);
    p[2] = ((v >> 16) & 0xFF);
    p[3] = ((v >> 24) & 0xFF);
    return p + 4;
}
static inline uint16_t deserialize_int16(uint8_t *p) { 
    return (uint16_t)(p[0]) | (((uint16_t)(p[1])) << 8);
}
static inline uint32_t deserialize_int32(uint8_t *p) {
    return (uint32_t)(p[0]) | (((uint32_t)(p[1]))<<8) | (((uint32_t)(p[2]))<<16) | (((uint32_t)(p[3]))<<24);
}
/// a PUT8 on the backplane memory 
static inline int backplane_write(uint32_t addr, uint32_t val) {
    return sdio_write(SDIO_FN_BACKPLANE, addr, val);
}
/// a GET8 on the backplane memory 
static inline int backplane_read(uint32_t addr) {
    return sdio_read(SDIO_FN_BACKPLANE, addr);
}
/// a GET32 on the backplane (the arm core) memory 
uint32_t backplane_read32(uint32_t addr); 
/// a PUT32 on the backplane (the arm core) memory 
int backplane_write32(uint32_t addr, uint32_t data); 
/// align the backplane window for performing IO on the target address 
void set_backplane_window(uint32_t addr); 


static inline uint32_t get_chip_id() {
    set_backplane_window(BACKPLANE_BASE_ADDR); 
    uint32_t r = backplane_read32(BACKPLANE_32BIT_ADDR_BASE); 
    uint32_t chip_id = (r & 0xFFFF); 
    uint32_t chip_rev = ((r >> 16) & 0xF); 
    if (chip_rev > 1) {
        printk(
            "warning: chip rev is %d, but we choose to support rev 1 (BCM43430), for rev = 2, please modify to upload firmware for BCM43436!\n", 
            chip_rev
        ); 
    }
    return chip_id; 
}
int backplane_perform_io(int is_write, uint32_t addr, uint8_t *buf, uint32_t len); 

/// from plan 9, implementation details not clear 
void backplane_disable(uint32_t regs, uint32_t pre, uint32_t ioctl);
/// from plan 9, implementation details not clear 
void backplane_reset(uint32_t regs, int pre, int ioctl);  
void backplane_init(struct WLANDeviceInfo *ctl); 
int backplane_enable(struct WLANDeviceInfo *ctl); 
int card_interrupt_handler(uint32_t sdio_int_reg_val); 
void backplane_init_enable_unknown_tailing_sequence(); 
#endif 

