#include "dma/dma.h" 
volatile struct DMAInfo channels[DMA_NUM_CHANNELS];
static volatile struct DMAControlBlock *cbs = 0; 
// static uint32_t cbs_handle = 0;
// static uint32_t cbs_allocated_size = 0; 
// static uint32_t cbs_bus_addr = 0; 
static struct MBoxMallocState cbs_malloc_state; 
#if DMA_USE_INTERRUPT
void dma_interrupt_handler(unsigned pc, void *dma_info) {
    volatile struct DMAInfo *info = (volatile struct DMAInfo *)dma_info;
    #if DEBUG_DMA 
    printk("dma_interrupt_handler: channel%d dma done!\n", info->channel);
    #endif 
    // clear the interrupt status bit of CS register 
    // note: don't clear the end bit, because dma_wait needs it! 
    info->reg_file[DMA_CS_IDX] = DMA_CS_INT;
    info->dma_done = 1;
    #if DEBUG_DMA 
    printk("DMA channel%d done CS=%x, Debug=%x\n", info->channel, info->reg_file[DMA_CS_IDX], info->reg_file[DMA_DEBUG_IDX]);
    #endif 
    // PUT32(DMA_REG_ENABLE, GET32(DMA_REG_ENABLE) & ~(1 << info->channel)); 
    disable_interrupt(bcm2835_dma_get_irq(info->channel));
    return; 
}
#endif 

void dma_init() {
    if (!cbs) {
        // kmalloc_init();
        #if DMA_USE_INTERRUPT
        interrupt_init();
        system_enable_interrupts();
        #endif 
        cbs = (volatile struct DMAControlBlock *)mbox_malloc(
            sizeof(volatile struct DMAControlBlock) * DMA_NUM_CHANNELS, 
            32, 
            MEM_FLAG_COHERENT, 
            &cbs_malloc_state
        ); 
        for (unsigned i = 0; i < DMA_NUM_CHANNELS; ++i) {
            channels[i].channel = i;
            channels[i].reg_file = (volatile uint32_t*)(DMA_CH0_REGS + i * DMA_CH_REG_OFFSET); 
            channels[i].dma_done = 0;
            channels[i].cb = &(cbs[i]);
        }
        flush_all_caches(); 
        dev_barrier(); 
    }
}

int dma_start(
    uint32_t channel, 
    uint32_t device, 
    uint32_t direction, 
    void *src, 
    void *dest, 
    uint32_t len
) {
    if (channel >= DMA_NUM_CHANNELS) {
        printk("invalid channel %d, DMA_NUM_CHANNELS=%d\n", channel, DMA_NUM_CHANNELS); 
        return 0; 
    }
    if (channel == 1 || channel == 3) {
        printk("warning: channel 1 or channel 3 may have some timeout issue!\n"); 
    }
    volatile struct DMAInfo *info = &(channels[channel]);
    if (!info->reg_file) {
        printk("DMA uninitialized, please call dma_init first!\n");
        return 0;
    }
    #if DMA_USE_INTERRUPT
    enable_interrupt(bcm2835_dma_get_irq(channel), dma_interrupt_handler, (void*)info);
    #endif 
    // should be enabled by default... 
    PUT32(DMA_REG_ENABLE, GET32(DMA_REG_ENABLE) | (1 << channel));
    dev_barrier();
    info->reg_file[DMA_CS_IDX] = DMA_CS_RESET;
    dev_barrier();
    delay_ms(1);
    // cannot be read, that bit will self clear 
    // while (info->reg_file[DMA_CS_IDX] & DMA_CS_RESET) {} 
    volatile struct DMAControlBlock *cb = info->cb;
    uint32_t ti = 0;
    flush_all_caches();
    dev_barrier();
    switch (direction) {
        case DMA_DEV2MEM:
            ti = (DMA_SRC_DREQ | DMA_DST_INC);
            cb->src_addr = ARM2BUS_PERI_ADDR(src);
            // cb->dst_addr = ((uint32_t)dest);
            cb->dst_addr = RPI1_DMA_ARM2BUS_MEM_ADDR(dest);
            break;
        case DMA_MEM2DEV:
            ti = (DMA_SRC_INC | DMA_DST_DREQ);
            cb->src_addr = RPI1_DMA_ARM2BUS_MEM_ADDR((uint32_t)src);
            // cb->src_addr = ((uint32_t)src);
            cb->dst_addr = ARM2BUS_PERI_ADDR((uint32_t)dest);
            break;
        case DMA_MEM2MEM:
            ti = (DMA_SRC_INC | DMA_DST_INC);
            cb->src_addr = RPI1_DMA_ARM2BUS_MEM_ADDR((uint32_t)src);
            cb->dst_addr = RPI1_DMA_ARM2BUS_MEM_ADDR((uint32_t)dest);
            // cb->src_addr = ((uint32_t)src);
            // cb->dst_addr = ((uint32_t)dest);
            break;
        case DMA_DEV2DEV: 
            ti = (DMA_SRC_DREQ | DMA_DST_DREQ); 
            cb->src_addr = ARM2BUS_PERI_ADDR(src); 
            cb->dst_addr = ARM2BUS_PERI_ADDR(dest); 
            break; 
        default:
            printk("unknown dma direction\n");
            return 0;
    }
    #if DMA_USE_INTERRUPT
    cb->transfer_info = ti | (device << 16) | DMA_TI_INT_EN;
    #else
    cb->transfer_info = ti | (device << 16); 
    #endif 
    cb->transfer_len = len;
    cb->mode_stride = 0;
    cb->next_cb_addr = 0;
    // dsb();
    flush_all_caches();
    dev_barrier();
    // reset CS register 
    info->reg_file[DMA_CS_IDX] = 0;
    delay_ms(1);
    dev_barrier();
    info->reg_file[DMA_CBLK_ADDR_IDX] = RPI1_DMA_ARM2BUS_MEM_ADDR(cb);
    // info->reg_file[DMA_CBLK_ADDR_IDX] = (uint32_t)(cb);
    // clear the interrupt status of the DMA engine... 
    PUT32(DMA_REG_INT_STATUS, GET32(DMA_REG_INT_STATUS) & (~(1 << channel)));
    // clear the interrupt bit of CS register also the end bit 
    info->reg_file[DMA_CS_IDX] = (DMA_CS_INT | DMA_CS_END);
    delay_ms(1);
    dev_barrier();
    // dsb();
    flush_all_caches();
    dev_barrier();
    #if DEBUG_DMA 
    printk("DMA channel%d pre CS=%x, Debug=%x\n", channel, info->reg_file[DMA_CS_IDX], info->reg_file[DMA_DEBUG_IDX]);
    #endif 
    info->reg_file[DMA_CS_IDX] = DMA_CS_ACTIVE;
    dev_barrier();
    return 1; 
}

int dma_wait(uint32_t channel) {
    volatile struct DMAInfo *info = &(channels[channel]);
    uint32_t i = 0;
#if DMA_USE_INTERRUPT
/// if we have threads, we could context switch to do other things 
    while (!(info->dma_done)) {
        // pass
        i += 1;
        if (i > 1048576) {
            printk("timeout!\n");
            break;
        }
    }
    info->dma_done = 0;
#else 
    uint32_t csval = info->reg_file[DMA_CS_IDX]; 
    while ((csval & (DMA_CS_END | DMA_CS_ERR)) == 0) {
        i += 1; 
        if (i > 1048576) {
            printk("timeout!\n"); 
            break; 
        }
        csval = info->reg_file[DMA_CS_IDX]; 
    }
#endif
    uint32_t cs = info->reg_file[DMA_CS_IDX];
    if ((cs & (DMA_CS_ACTIVE | DMA_CS_END | DMA_CS_ERR)) != DMA_CS_END) {
        printk("DMA channel %d transmission failed, CS=%x, Debug=%x\n", channel, cs, info->reg_file[DMA_DEBUG_IDX]);
        info->reg_file[DMA_CS_IDX] = DMA_CS_RESET;
        dev_barrier();
        delay_ms(1); 
        // clear the debug register, only the last 3 bits could be cleared by writing a 1. 
        info->reg_file[DMA_DEBUG_IDX] = (0b111);
        dev_barrier(); 
        return 0;
    }
    info->reg_file[DMA_CS_IDX] = (DMA_CS_INT | DMA_CS_END);
    return 1; 
} 


uint32_t mbox_alloc_mem(uint32_t size, uint32_t alignment, uint32_t flags, uint32_t *real_size) {
    // static volatile uint32_t *u = 0; 
    // if (!u) {
    //     u = kmalloc_aligned(9*4, 16); 
    // } // Make `size` a multiple of PAGE_SIZE
    size = ((size + PAGE_SIZE - 1) / PAGE_SIZE) * PAGE_SIZE;
    (*real_size) = size; 
    memset((void*)u, 0, 9*4);
    u[0] = 9*4;
    u[1] = 0;
    u[2] = 0x0003000c;
    u[3] = 12;
    u[4] = 12;
    u[5] = size;
    u[6] = alignment; 
    u[7] = flags;
    u[8] = 0;
    mbox_send(MBOX_CH, u);
    return u[5];
}
uint32_t mbox_lock_mem(uint32_t handle) {
    // static volatile unsigned *u = 0; 
    // if (!u) {
    //     u = kmalloc_aligned(7*4, 16); 
    // }
    memset((void*)u, 0, 7*4);
    u[0] = 7*4;
    u[1] = 0;
    u[2] = 0x0003000d;
    u[3] = 4;
    u[4] = 4;
    u[5] = handle;
    u[6] = 0;
    mbox_send(MBOX_CH, u);
    return u[5];
}
uint32_t mbox_unlock_mem(uint32_t handle) {
    // static volatile unsigned *u = 0; 
    // if (!u) {
    //     u = kmalloc_aligned(7*4, 16); 
    // }
    memset((void*)u, 0, 7*4);
    u[0] = 7*4;
    u[1] = 0;
    u[2] = 0x0003000e;
    u[3] = 4;
    u[4] = 4;
    u[5] = handle;
    u[6] = 0;
    mbox_send(MBOX_CH, u);
    return u[5];
}
uint32_t mbox_release_mem(uint32_t handle) {
    // static volatile unsigned *u = 0; 
    // if (!u) {
    //     u = kmalloc_aligned(7*4, 16); 
    // }
    memset((void*)u, 0, 7*4);
    u[0] = 7*4;
    u[1] = 0;
    u[2] = 0x0003000f;
    u[3] = 4;
    u[4] = 4;
    u[5] = handle;
    u[6] = 0;
    mbox_send(MBOX_CH, u);
    return u[5];
}

void *mbox_malloc(uint32_t size, uint32_t alignment, uint32_t flags, struct MBoxMallocState *state) {
    uint32_t real_size = 0; 
    uint32_t handle = mbox_alloc_mem(size, alignment, flags, &real_size);
    state->size_allocated = real_size; 
    state->handle = handle; 
    uint32_t bus_addr =  mbox_lock_mem(handle); 
    state->bus_addr = bus_addr;
    state->arm_addr = BUS2ARM_MEM_ADDR(bus_addr); 
    state->requested_size = size; 
    state->requested_alignment = alignment; 
    state->requested_flags = flags; 
    return (void*)state->arm_addr;
    // return (void*)bus_addr;
}
uint32_t mbox_unlock_release(uint32_t handle) {
    uint32_t status = mbox_unlock_mem(handle);
    if (status != 0) {
        printk("VC memory unlock failed, status=%d\n", status); 
        return status;
    }
    status = mbox_release_mem(handle); 
    if (status != 0) {
        printk("VC memory release failed, status=%d\n", status); 
    }
    return status; 
}









