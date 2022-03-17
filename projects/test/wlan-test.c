#include "wlan/wlan-sdio.h" 
#include "wlan/wlan-backplane.h"
#include "wlan/wlan-load-firmware.h"
#include "wlan/wlan-ioctl.h"
#include "wlan/wlan.h"
// uint8_t config_data[] = 
// "manfid=0x2d0\0""prodid=0x0726\0""vendid=0x14e4\0""devid=0x43e2\0"
// "boardtype=0x0726\0""boardrev=0x1202\0""boardnum=22\0""macaddr=00:90:4c:c5:12:38\0"
// "sromrev=11\0""boardflags=0x00404201\0""boardflags3=0x08000000\0""xtalfreq=37400\0"
// "nocrc=1\0""ag0=255\0""aa2g=1\0""ccode=ALL\0""pa0itssit=0x20\0""extpagain2g=0\0"
// "pa2ga0=-168,7161,-820\0""AvVmid_c0=0x0,0xc8\0""cckpwroffset0=5\0""maxp2ga0=84\0"
// "txpwrbckof=6\0""cckbw202gpo=0\0""legofdmbw202gpo=0x66111111\0"
// "mcsbw202gpo=0x77711111\0""propbw202gpo=0xdd\0""ofdmdigfilttype=18\0"
// "ofdmdigfilttypebe=18\0""papdmode=1\0""papdvalidtest=1\0""pacalidx2g=32\0"
// "papdepsoffset=-36\0""papdendidx=61\0""il0macaddr=00:90:4c:c5:12:38\0"
// "wl0id=0x431b\0""deadman_to=0xffffffff\0""muxenab=0x1\0""spurconfig=0x3 \0"
// "btc_mode=1\0""btc_params8=0x4e20\0""btc_params1=0x7530\0""\0\0\0\0\xaa\x00\x55\xff";
// uint32_t config_len = sizeof(config_data) - 1;
// void load_test() {
//     uint8_t config_txt_buffer[4096];
//     uint32_t config_txt_size = 0; 
//     load_file_from_root_dir("CONFIG.TXT", config_txt_buffer, &config_txt_size); 
//     printk("------------------------ CONFIG.TXT -------------------------------\n");
//     for (unsigned i = 0; i < config_txt_size; ++i) {
//         printk("%c", config_txt_buffer[i]); 
//     }
//     printk("------------------------ CONFIG.TXT -------------------------------\n");
//     return; 
// }
void test_nvram_condense() {
    const uint8_t *nvram = NULL; 
    unsigned nvram_size = 0; 
    get_nvram(&nvram, &nvram_size); 
    uint8_t condensed[1024]; 
    uint32_t condensed_size = 0; 
    condense_nvram(nvram, nvram_size, condensed, &condensed_size); 
    dump_nvram(condensed, condensed_size); 
}
void others() {
    // test_nvram_condense();
    // load_test();
    // delay_ms(1000);
    // printk("initing emmc again!\n"); 
    // emmc_init();
    // printk("initing emmc done! setting ether gpio\n"); 
    // delay_ms(1000);
}
#define TEST_SCAN 0 
#define TEST_JOIN 0 
#define TEST_SEND 1 
#if TEST_SCAN
#include "test_ioctl_scan.h"
#endif 
// use iw dev wlan0 station dump  in RPi AP mode to see this device 
void run() {
    wlan_init(); 
    #if TEST_SCAN
    test_ioctl_scan();
    #elif TEST_JOIN
    // test_ioctl_join("RaspberryPi", "US", -1, SECURITY_WPA2, "raspberry"); 
    wlan_up(); 
    wlan_join("RaspberryPi", "US", -1, SECURITY_WPA2, "raspberry"); 
    int exclude_events[] = {
        44, 71, 72, 
        -1
    };
    ioctl_enable_all_evts((int*)exclude_events);
    #define WLC_E_TXFAIL 20  
    EVT_STR evts[] = {
        EVT(WLC_E_SET_SSID), 
        EVT(WLC_E_LINK), 
        EVT(WLC_E_AUTH), 
        EVT(WLC_E_DEAUTH_IND), 
        EVT(WLC_E_DISASSOC_IND), 
        EVT(WLC_E_PSK_SUP), 
        EVT(WLC_E_TXFAIL), 
        EVT(-1)
    };
    wlan_listen_to_events(NULL); 
    #elif TEST_SEND
    wlan_up(); 
    wlan_join("raspberrypi1", "US", -1, SECURITY_WPA2, "raspberry"); 
    // b8:27:eb:b0:a8:30
    // dc:a6:32:a1:94:fe
    uint8_t dest_mac[6] = 
    {0xdc, 0xa6, 0x32, 0xa1, 0x94, 0xfe};    // RPi 4 _1 dc:a6:32:a1:94:fe
    // {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};   // broadcast 
    // {0xb8, 0x27, 0xeb, 0xb0, 0xa8, 0x30};   // RPi 3
    uint8_t *test_data = "Hello World from Pi Zero~"; 
    wlan_test_send_frame((uint8_t*)dest_mac, test_data, strlen(test_data)); 
    #endif 

}
void notmain() {
    run(); 
    // // others(); 
    // set_ether_func_gpio(); 
    // printk("setting ether gpio done! initing sdio\n"); 
    // int ret = sdio_init(); 
    // printk("ret = %d\n", ret); 
    // uint8_t buffer[4];
    // // get the chip id 
    // uint32_t chip_id = get_chip_id(); 
    // printk("chip_id = %x\n", chip_id); // 0xA6 0xA9 0x41 0x15, or 0x1541a9a6 
    // // try scanning core 
    // // uint32_t r = backplane_read32(BACKPLANE_BASE_ADDR + 63*4);
    // // corescan(ctl, r); 
    // // initialize the backplane 
    // backplane_init(ctl); 
    // printk("chipcommon=%x, armcore=%x, armctl=%x, armregs=%x, socramctl=%x, socramregs=%x, socramrev=%x, sdregs=%x, sdiorev=%x, d11ctl=%x\n", 
    //     ctl->chipcommon, 
    //     ctl->armcore, 
    //     ctl->armctl, 
    //     ctl->armregs, 
    //     ctl->socramctl, 
    //     ctl->socramregs, 
    //     ctl->socramrev, 
    //     ctl->sdregs, 
    //     ctl->sdiorev, 
    //     ctl->d11ctl
    // );
    
    // // upload firmware 
    // printk("uploading firmware...\n"); 
    // // const unsigned char *firmware_buffer = NULL; 
    // // unsigned firmware_size = 0; 
    // // get_firmware(&firmware_buffer, &firmware_size); 
    // // upload_data(ctl->rambase, firmware_buffer, firmware_size); 
    // // printk("firmware uploaded...\n"); 
    // // delay_ms(3000);
    // // printk("verifying...\n");
    // // assert(verify_upload_data(ctl->rambase, firmware_buffer, firmware_size) == 0); 

    // assert(firmware_upload() == 0); 

    // printk("FIRMWARE Upload Done!!!\n");
    // // backplane_enable 
    // assert(backplane_enable(ctl) == 0); 
    // printk("wait for interrupt\n"); 
    // uint32_t intr = 0; 
    // emmc_wait_for_interrupt(EMMC_INTERRUPT_CARD, 3000); 
    // // intr = GET32(EMMC_INTERRUPT); 
    // // while ((intr & EMMC_INTERRUPT_CARD) == 0) {
    // //     printk("no interrupt\n"); 
    // //     delay_ms(500); 
    // //     intr = GET32(EMMC_INTERRUPT); 
    // // }
    // printk("got interrupt\n"); 
    // card_interrupt_handler(emmc.interrupt_val); 
    // printk("SUCCESS!!!\n"); 
    // delay_ms(500); 

    // // emmc_write_register(EMMC_INTERRUPT, EMMC_INTERRUPT_CARD); 
    // // intr = GET32(EMMC_INTERRUPT); 
    // // int cnt = 0; 
    // // while ((intr & EMMC_INTERRUPT_CARD) != 0) {
    // //     printk("strange: we've already cleared the card interrupt, there's another one? intr = %x\n", intr); 
    // //     card_interrupt_handler(intr); 
    // //     emmc_write_register(EMMC_INTERRUPT, intr); 
    // //     dev_barrier(); 
    // //     intr = GET32(EMMC_INTERRUPT); 
    // //     if ((cnt++) > 3) {
    // //         break; 
    // //     }
    // // }
    // printk("performing the rest of the initialization...\n"); 
    // backplane_init_enable_unknown_tailing_sequence(); 
    // uint8_t resp[256] = {0}; 
    // ////// MUST READ THIS!!!!!! 
    // sdio_perform_io(0, SDIO_FN2, BACKPLANE_32BIT_ADDR_BASE, resp, 64, 1); 
    // printk("SUCCESS!!!\n"); 
    // test_ioctl(); 
    // return; 
}

