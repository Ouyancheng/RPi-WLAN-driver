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
}

