#include "wlan/wlan-backplane.h" 
struct WLANDeviceInfo device; 
struct WLANDeviceInfo *ctl = &device; 
#define CACHELINESZ 64 
uint8_t backplane_rw_buffer[CACHELINESZ]; 
uint32_t backplane_read32(uint32_t addr) {
    // uint8_t cbuf[2*CACHELINESZ];
    uint8_t *p = (uint8_t*)backplane_rw_buffer;
    // p = (uint8_t*)ROUND(((uintptr_t)cbuf), CACHELINESZ);
    memset(p, 0, 4);
    sdio_perform_io(0, SDIO_FN_BACKPLANE, addr|BACKPLANE_32BIT_ADDR_BASE, p, 4, 1); 
    // printk("backplane_read32 %x: %x %x %x %x\n", addr, p[0], p[1], p[2], p[3]);
    return deserialize_int32(p); 
}

int backplane_write32(uint32_t addr, uint32_t data) {
    // uint8_t cbuf[2*CACHELINESZ];
    uint8_t *p = (uint8_t*)backplane_rw_buffer;;
    // p = (uint8_t*)ROUND(((uintptr_t)cbuf), CACHELINESZ);
    serialize_int32(p, data);
    // printk("backplane_write32 %x: %x %x %x %x\n", addr, p[0], p[1], p[2], p[3]);
    sdio_perform_io(1, SDIO_FN_BACKPLANE, addr|BACKPLANE_32BIT_ADDR_BASE, p, 4, 1); 
    return 0; 
}

void set_backplane_window(uint32_t addr) {
    addr &= (~BACKPLANE_WINDOW_MASK); 
    backplane_write(BACKPLANE_WINDOW_ADDR_REG  , addr >> 8);
    backplane_write(BACKPLANE_WINDOW_ADDR_REG+1, addr >> 16);
    backplane_write(BACKPLANE_WINDOW_ADDR_REG+2, addr >> 24);
}
int backplane_io_internal(int is_write, uint32_t addr, uint8_t *buf, uint32_t len) {
    if (len >= 4) {
        // handle 4-aligned 
        uint32_t n = (len & (~0b11));
        sdio_perform_io(
            is_write, 
            SDIO_FN_BACKPLANE, 
            addr|BACKPLANE_32BIT_ADDR_BASE, 
            buf, 
            n, 
            1
        ); 
        addr += n;
        buf += n;
        len -= n;
    }
    while (len > 0) {
        // handle the rest 
        if (is_write) {
            sdio_write(
                SDIO_FN_BACKPLANE, 
                addr|BACKPLANE_32BIT_ADDR_BASE, 
                *buf
            ); 
        } else {
            (*buf) = (uint8_t)sdio_read(
                SDIO_FN_BACKPLANE, 
                addr|BACKPLANE_32BIT_ADDR_BASE
            ); 
        }
        addr += 1;
        buf += 1;
        len -= 1;
    }
    return 0; 
}
int backplane_perform_io(int is_write, uint32_t addr, uint8_t *buf, uint32_t len) {
    uint32_t n = ROUNDUP(addr, BACKPLANE_WINDOW_SIZE) - addr;
    if (n == 0) n = BACKPLANE_WINDOW_SIZE;
    while (len > 0) {
        if (n > len) n = len;
        set_backplane_window(addr);
        backplane_io_internal(is_write, (addr & BACKPLANE_WINDOW_MASK), buf, n); 
        addr += n;
        buf += n;
        len -= n;
        n = BACKPLANE_WINDOW_SIZE;
    }
    return 0; 
}


/// Plan 9, details not-yet clear 
static void corescan(struct WLANDeviceInfo *ctl, uint32_t r) {
    uint8_t buf[512];
    int i, coreid, corerev;
    uint32_t addr;
    backplane_perform_io(0, r, buf, 512);
    coreid = 0;
    corerev = 0;
    for(i = 0; i < 512; i += 4){
        switch(buf[i]&0xF){
        case 0xF:	/* end */
            return;
        case 0x1:	/* core info */
            if((buf[i+4]&0xF) != 0x1)
                break;
            coreid = (buf[i+1] | buf[i+2]<<8) & 0xFFF;
            i += 4;
            corerev = buf[i+3];
            break;
        case 0x05:	/* address */
            addr = buf[i+1]<<8 | buf[i+2]<<16 | buf[i+3]<<24;
            addr &= ~0xFFF;
            printk("core %x %s %p\n", coreid, buf[i]&0xC0? "ctl" : "mem", addr);
            switch(coreid){
            case 0x800:
                if((buf[i] & 0xC0) == 0)
                    ctl->chipcommon = addr;
                break;
            case ARMcm3:
            case ARM7tdmi:
            case ARMcr4:
                ctl->armcore = coreid;
                if(buf[i] & 0xC0){
                    if(ctl->armctl == 0)
                        ctl->armctl = addr;
                }else{
                    if(ctl->armregs == 0)
                        ctl->armregs = addr;
                }
                break;
            case 0x80E:
                if(buf[i] & 0xC0)
                    ctl->socramctl = addr;
                else if(ctl->socramregs == 0)
                    ctl->socramregs = addr;
                ctl->socramrev = corerev;
                break;
            case 0x829:
                if((buf[i] & 0xC0) == 0)
                    ctl->sdregs = addr;
                ctl->sdiorev = corerev;
                break;
            case 0x812:
                if(buf[i] & 0xC0)
                    ctl->d11ctl = addr;
                break;
            }
        }
    }
    assert(ctl->armcore == ARMcm3); 
}

/// from plan 9, why are we reading things back after write? To ensure completion??? 
void backplane_disable(uint32_t regs, uint32_t pre, uint32_t ioctl) {
	set_backplane_window(regs);
	if((backplane_read32(regs + BACKPLANE_RESET_CTRL) & 1) != 0){
		backplane_write32(regs + BACKPLANE_IO_CTRL, 3|ioctl);
		backplane_read32(regs + BACKPLANE_IO_CTRL);
		return;
	}
	backplane_write32(regs + BACKPLANE_IO_CTRL, 3|pre);
	backplane_read32(regs + BACKPLANE_IO_CTRL);
	backplane_write32(regs + BACKPLANE_RESET_CTRL, 1);
	delay_us(10);
	while((backplane_read32(regs + BACKPLANE_RESET_CTRL) & 1) == 0) {} 
	backplane_write32(regs + BACKPLANE_IO_CTRL, 3|ioctl);
	backplane_read32(regs + BACKPLANE_IO_CTRL);
}
void backplane_reset(uint32_t regs, int pre, int ioctl) {
    backplane_disable(regs, pre, ioctl);
    set_backplane_window(regs);
    printk("backplane_reset %x %x %x ->", 
        regs, 
        backplane_read32(regs+BACKPLANE_IO_CTRL), 
        backplane_read32(regs+BACKPLANE_RESET_CTRL));
    while((backplane_read32(regs + BACKPLANE_RESET_CTRL) & 1) != 0){
        backplane_write32(regs + BACKPLANE_RESET_CTRL, 0);
        delay_ms(40);
    }
    backplane_write32(regs + BACKPLANE_IO_CTRL, 1|ioctl);
    backplane_read(regs + BACKPLANE_IO_CTRL);
    printk("%x %x\n", 
        backplane_read32(regs+BACKPLANE_IO_CTRL), 
        backplane_read32(regs+BACKPLANE_RESET_CTRL));
}
/// Plan 9, why are we writing data to the backplane if we are just scaning ram??? 
static void ramscan(struct WLANDeviceInfo *ctl) {
    assert(ctl->armcore == ARMcm3); 
    backplane_reset(ctl->socramctl, 0, 0);
    uint32_t socramregs = ctl->socramregs;
    set_backplane_window(socramregs);
    uint32_t ramcoreinfo = backplane_read32(socramregs + SOCRAM_COREINFO);
    uint32_t banks = (ramcoreinfo >> 4) & 0xF;
    printk("socramrev %d coreinfo %x banks=%u\n", 
        ctl->socramrev, ramcoreinfo, banks);
    uint32_t size = 0; 
    for(unsigned i = 0; i < banks; i++){
        backplane_write32(socramregs + SOCRAM_BANKIDX, i);
        uint32_t bankinfo = backplane_read32(socramregs + SOCRAM_BANKINFO);
        printk("bank %u reg %x size %u\n", 
            i, 
            bankinfo, 
            8192 * ((bankinfo & 0x3F) + 1)
        );
        size += 8192 * ((bankinfo & 0x3F) + 1);
    }
    ctl->socramsize = size;
    ctl->rambase = 0;
    backplane_write(socramregs + SOCRAM_BANKIDX, 3);
    backplane_write(socramregs + SOCRAM_BANKPDA, 0);
    return; 
}

void backplane_init(struct WLANDeviceInfo *ctl) {
    uint32_t r;
    int chipid;
    set_backplane_window(BACKPLANE_BASE_ADDR);
    r = backplane_read32(BACKPLANE_32BIT_ADDR_BASE); 
    chipid = get_chip_id();
    printk("backplane_init: chip %d rev %d type %d\n", 
        chipid, 
        (r>>16)&0xF, 
        (r>>28)&0xF);
    assert(chipid == 43430); 
    // ??? 
    r = backplane_read32(BACKPLANE_BASE_ADDR + 63*4);
    corescan(ctl, r);
    assert(ctl->armcore == ARMcm3); 
    backplane_disable(ctl->armctl, 0, 0);
    delay_ms(100); 
    backplane_reset(ctl->d11ctl, 8|4, 4);
    delay_ms(100); 
    ramscan(ctl);
    printk("ARM %x D11 %x SOCRAM %x,%x %u bytes @ %x\n", 
        ctl->armctl, 
        ctl->d11ctl, 
        ctl->socramctl, ctl->socramregs, 
        ctl->socramsize, ctl->rambase);
    // set the active low-power clock!!! 
    backplane_write(BACKPLANE_CLK_CSR, 0);
    delay_ms(10);
    printk("chipclk: %x\n", backplane_read(BACKPLANE_CLK_CSR));
    const uint32_t nohwreq = 0x20;  // ??? 
    const uint32_t request_ALP = 0x08; // active low-power clock!! 
    // is it called power-save mode? 
    backplane_write(BACKPLANE_CLK_CSR, 
        nohwreq | request_ALP); 
    const uint32_t ht_clk_available = 0x80; // high throughput clock!!! 
    const uint32_t ALP_available = 0x40; 
    while (
        (backplane_read(BACKPLANE_CLK_CSR) & (ht_clk_available|ALP_available)) 
        == 0
    ) {
        delay_us(10);
    }
    // const uint32_t force_enable_ht = 0x02; 
    // force enable low-power clock 
    const uint32_t force_enable_ALP = 0x01; 
    backplane_write(BACKPLANE_CLK_CSR, nohwreq | force_enable_ALP);
    delay_us(65);
    // the active low-power clock should be set right now!!! 
    printk("chipclk: %x\n", backplane_read(BACKPLANE_CLK_CSR));
    backplane_write(BACKPLANE_PULLUP, 0);
    set_backplane_window(BACKPLANE_BASE_ADDR);
    backplane_write32(BACKPLANE_BASE_ADDR + BACKPLANE_GPIO_PULLUP, 0);
    backplane_write32(BACKPLANE_BASE_ADDR + BACKPLANE_GPIO_PULLDOWN, 0);
    return; 
}
#if 0
// Clkcsr = 0x1000e,
// ForceALP = 0x01,/* active low-power clock */
// ForceHT = 0x02,/* high throughput clock */
// ForceILP = 0x04,/* idle low-power clock */
// ReqALP = 0x08,
// ReqHT = 0x10,
// Nohwreq = 0x20,
// ALPavail = 0x40,
// HTavail = 0x80,
#endif 
int backplane_enable(struct WLANDeviceInfo *ctl) {
    printk("enabling high throughput clock...\n");
    set_backplane_window(BACKPLANE_BASE_ADDR); 
    backplane_write(BACKPLANE_CLK_CSR, 0);
    delay_ms(100);
    // use the high throughput clock 
    const uint32_t request_ht = 0x10; 
    // this is to wake up the WLAN 
    /// now we try to wake up the card, 
    /// sometimes it couldn't wake up at first attempt... 
    const uint32_t ht_clk_available = 0x80; 
    int wake_up_attempts = 0; 
    int retries = 0;
    while (1) {
        backplane_write(BACKPLANE_CLK_CSR, request_ht);
        dev_barrier(); 
        delay_ms(1000); 
        printk("after requesting ht, csr = %x\n", backplane_read(BACKPLANE_CLK_CSR)); 
        int is_active = 0; 
        retries = 0; 
        is_active = (backplane_read(BACKPLANE_CLK_CSR) & ht_clk_available); 
        while (is_active == 0) {
            retries += 1; 
            if (retries > 300) {
                printk("cannot enable HT clock after a while, csr = %x\n", 
                    backplane_read(BACKPLANE_CLK_CSR)); 
                // return -1; 
                break; 
            }
            delay_ms(100);
            is_active = (backplane_read(BACKPLANE_CLK_CSR) & ht_clk_available); 
        }
        if (is_active) {
            break; 
        } else {
            wake_up_attempts += 1; 
        }
    }
    
    printk("High throughput clock enabled, retries=%d\n", wake_up_attempts); 
    const uint32_t force_use_ht = 0x02; 
    backplane_write(BACKPLANE_CLK_CSR, backplane_read(BACKPLANE_CLK_CSR) | force_use_ht);
    delay_ms(10);
    printk("chipclk: %x\n", backplane_read(BACKPLANE_CLK_CSR));
    // high throughput clock should be enabled 
    printk("setting backplane window = %x\n", ctl->sdregs); 
    set_backplane_window(ctl->sdregs);
    printk("backplane window set\n"); 
    /* protocol version */ // ??? 
    // SDCORE_BACKPLANE_MBOX_DATA = 0x48 = 72 
    backplane_write32(ctl->sdregs + SDCORE_BACKPLANE_MBOX_DATA, 4 << 16);  // 0x40000 or 262144 
    printk("SDCORE_BACKPLANE_MBOX_DATA set\n"); 
    // is it masking in or masking out? 
    // 0x24 = 36 
    backplane_write32(ctl->sdregs + SDCORE_INT_MASK, SDCORE_INT_FRAME_INT | SDCORE_INT_MBOX_INT | SDCORE_INT_FCC_CHANGE);
    printk("SDCORE_INT_MASK set\n"); 
    // enable IO for the WLAN plane 
    sdio_set(SDIO_FN0, SDIO_CCCR_IO_ENABLE, 1 << SDIO_FN_WLAN);
    retries = 0; 
    while (!(sdio_read(SDIO_FN0, SDIO_CCCR_IO_READY) & (1 << SDIO_FN_WLAN))) {
        retries += 1; 
        if (retries > 10) {
            printk("cannot enable WLAN function for 43430! ioready = %x\n", 
                sdio_read(SDIO_FN0, SDIO_CCCR_IO_READY)); 
            return -2; 
        }
        printk("retrying in 100ms...\n"); 
        delay_ms(100); 
    }
    printk("WLAN IO enabled!!!\n"); 
    /// WARNING: DO NOT enable card interrupt!!! 
    /// the card keeps generating interrupt even if nothing's there!!!!!! 
    // emmc_enable_card_interrupt(card_interrupt_handler); 
    // enable interrupts 
    sdio_write(SDIO_FN0, SDIO_CCCR_INT_ENABLE, 
        (1 << SDIO_FN_BACKPLANE) | (1 << SDIO_FN_WLAN) | 1);
    printk("BACKPLANE + WLAN + SD interrupt enabled!!!\n"); 
    return 0; 
}
#define DEBUG_CARD_INTERRRUPT 1 
int card_interrupt_handler(uint32_t sdio_int_reg_val) {
    #if DEBUG_CARD_INTERRUPT
        printk("received card interrupt, emmc_int_reg = %x\n", sdio_int_reg_val); 
    #endif 
    set_backplane_window(ctl->sdregs); 
    #if DEBUG_CARD_INTERRUPT
        printk("set backplane window = %x\n", ctl->sdregs); 
    #endif 
    int intr = sdio_read(SDIO_FN0, SDIO_CCCR_INT_PENDING); 
    #if DEBUG_CARD_INTERRUPT
        printk("sdio_cccr_int_pending = %x\n", intr); 
    #endif 
    // assert(intr != 0); 
    if (intr == 0) { 
        // printk("spruious card interrupt\n"); 
        return -1; 
    }
    uint32_t intstat = backplane_read32(ctl->sdregs+SDCORE_INT_STATUS); 
    #if DEBUG_CARD_INTERRUPT
        printk("intstatus = %x\n", intstat); 
    #endif 
    // clear the interrupt 
    backplane_write32(ctl->sdregs+SDCORE_INT_STATUS, intstat); 
    #if DEBUG_CARD_INTERRUPT
        printk("interrupt bit cleared\n"); 
    #endif 
    if (intstat & SDCORE_INT_MBOX_INT) {
        uint32_t mbx = backplane_read32(ctl->sdregs + SDCORE_HOST_MBOX_DATA); 
        #if DEBUG_CARD_INTERRUPT
            printk("mbx = %x\n", mbx);
        #endif  
        backplane_write32(ctl->sdregs + SDCORE_BACKPLANE_MBOX, 2); 
        #if DEBUG_CARD_INTERRUPT
            printk("mbx acked\n"); 
        #endif 
        if (mbx & 0x8) {
            printk("WLAN Firmware ready!\n"); 
        }
    }
    if (intstat & SDCORE_INT_FRAME_INT) {
        return 0; 
    }
    return 0; 
}

void backplane_init_enable_unknown_tailing_sequence() {
    uint8_t buffer[64];
    uint32_t unknown_addr = 0x68000 | 0x7ffc; 
    set_backplane_window(unknown_addr); 
    uint32_t n = backplane_read32(unknown_addr); 
    set_backplane_window(0x38000); 
    backplane_perform_io(0, 0x70d4, buffer, 64); 
    emmc_wait_for_interrupt(EMMC_INTERRUPT_CARD, 3000); 
    card_interrupt_handler(emmc.interrupt_val); 
    // const uint32_t backplane_32bit_window = 0x8000; 
    sdio_perform_io(0, SDIO_FN_WLAN, BACKPLANE_32BIT_ADDR_BASE, buffer, 64, 1); 
}
