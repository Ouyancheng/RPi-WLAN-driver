#include "dma/dma.h"
#include "rpi.h"

#define SZ 123

void sync() {
    flush_all_caches();
    dev_barrier();
    // all things below won't work!!! 
    // use mbox_malloc to allocate the buffer instead! 
    // dsb();
    // flush_all_caches();
    // dev_barrier();
    // arm_mmu_reset();
    // arm_bp_disable();
    // arm_icache_disable();
    // arm_dcache_disable();
    // arm_smp_disable();
    // arm_icache_invalidate();
    // arm_dcache_invalidate();
    // arm_dcache_clean();
    // arm_dcache_clean_invalidate();
    // arm_cache_invalidate();
    // arm_bp_invalidate();
    // arm_tlb_invalidate();
    // arm_data_memorybarrier();
    // arm_instruction_syncbarrier();
    // arm_drain_write_buffer();
    // dev_barrier();
    // dmb();
    // dsb();
}
uint32_t dest1[SZ], dest2[SZ]; 
void notmain() {
    kmalloc_init();
    dma_init();

    for (int j = 1; j < 2; ++j) {
        uint32_t *ptr = 0; // kmalloc_aligned(SZ * sizeof(uint32_t), 4);
        uint32_t *ptr2 = 0; // kmalloc_aligned(SZ * sizeof(uint32_t), 4);
        uint32_t ptr1_handle = 0;
        uint32_t ptr2_handle = 0; 
        uint32_t ptr1_size_allocated = 0; 
        uint32_t ptr2_size_allocated = 0; 
        uint32_t ptr1_bus_addr = 0;
        uint32_t ptr2_bus_addr = 0;
        // ptr = (uint32_t*)mbox_malloc(SZ * sizeof(uint32_t), 4, MEM_FLAG_DIRECT, &ptr1_handle, &ptr1_size_allocated, &ptr1_bus_addr);
        // ptr2 = (uint32_t*)mbox_malloc(SZ * sizeof(uint32_t), 4, MEM_FLAG_DIRECT, &ptr2_handle, &ptr2_size_allocated, &ptr2_bus_addr); 
        ptr = kmalloc_aligned(SZ * sizeof(uint32_t), 4);
        ptr2 = kmalloc_aligned(SZ * sizeof(uint32_t), 4);

        uint32_t arr_handle = 0;
        uint32_t arr2_handle = 0; 
        uint32_t arr_size_allocated = 0; 
        uint32_t arr2_size_allocated = 0; 
        uint32_t *arr;
        uint32_t *arr2;
        uint32_t arr1_bus_addr = 0;
        uint32_t arr2_bus_addr = 0;
        // arr = (uint32_t*)mbox_malloc(SZ * sizeof(uint32_t), 4, MEM_FLAG_DIRECT, &arr_handle, &arr_size_allocated, &arr1_bus_addr);
        // arr2 = (uint32_t*)mbox_malloc(SZ * sizeof(uint32_t), 4, MEM_FLAG_DIRECT, &arr2_handle, &arr2_size_allocated, &arr2_bus_addr); 
        arr = (uint32_t*)kmalloc_aligned(SZ * sizeof(uint32_t), 4);
        arr2 = (uint32_t*)kmalloc_aligned(SZ * sizeof(uint32_t), 4); 

        

        printk("ptr1_size_allocated=%d, ptr2_size_allocated=%d\n", ptr1_size_allocated, ptr2_size_allocated); 
        printk("ptr=%p arr=%p ptr2=%p arr2=%p\n", ptr, arr, ptr2, arr2);
        printk("ptr_bus=%p arr_bus=%p ptr2_bus=%p arr2_bus=%p\n", ptr1_bus_addr, arr1_bus_addr, ptr2_bus_addr, arr2_bus_addr);

        // memset(arr, 0, SZ * sizeof(uint32_t));
        for (unsigned i = 0; i < SZ; ++i) {
            arr[i] = (i + j);
            arr2[i] = 2 * i + j;
        }
        
        sync();
        

        memset((void*)ptr, 0xFF, SZ*sizeof(uint32_t));
        memset((void*)ptr2, 0xFF, SZ*sizeof(uint32_t));
        printk("pre-DMA: ptr = ");
        for (unsigned i = 0; i < SZ; ++i) {
            uint32_t t = ptr[i];
            printk("%d ", t);
            t = ptr2[i];
            printk("%d ", t);
        }
        printk("\n");
        printk("pre-DMA: arr = ");
        for (unsigned i = 0; i < SZ; ++i) {
            uint32_t t = arr[i];
            printk("%d ", t);
            t = arr2[i];
            printk("%d ", t);
        }
        printk("\n");
        sync();
        #define CH1 0
        #define CH2 2
        dma_start(CH1, 0, DMA_MEM2MEM, arr, (void*)ptr, SZ * sizeof(uint32_t));
        if (!dma_wait(CH1)) {
            printk("DMA wait failed!\n");
        }
        sync();

        memcpy(dest1, ptr, SZ*sizeof(uint32_t)); 
        
        printk("DMA result: ptr = ");
        for (unsigned i = 0; i < SZ; ++i) {
            printk("%d ", ptr[i]);
        }
        printk("\n");


        dma_start(CH2, 0, DMA_MEM2MEM, arr2, (void*)ptr2, SZ * sizeof(uint32_t));
        if (!dma_wait(CH2)) {
            printk("DMA wait failed!\n");
        }
        sync();
        memcpy(dest2, ptr2, SZ*sizeof(uint32_t)); 
        printk("DMA result: ptr2 = ");
        for (unsigned i = 0; i < SZ; ++i) {
            printk("%d ", dest2[i]);
        }
        printk("\n");

        // mbox_unlock_release(ptr1_handle); 
        // mbox_unlock_release(ptr2_handle); 
        // mbox_unlock_release(arr_handle); 
        // mbox_unlock_release(arr2_handle); 
    }
    return;
}












