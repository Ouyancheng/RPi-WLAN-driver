// ZeroWi bare-metal WiFi driver, see https://iosoft.blog/zerowi
// Raspberry Pi WiFi network scan
//
// Copyright (c) 2020 Jeremy P Bentham
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#ifndef TEST_IOCTL_SCAN_H 
#define TEST_IOCTL_SCAN_H
#define VERSION "0.76"
/// file instrumented by Yancheng Ou 
#include "wlan/wlan-ioctl.h"
#include "wlan/wlan.h"
#define SCAN_CHAN       1
EVT_STR escan_evts[]=ESCAN_EVTS;
#define SSID_MAXLEN         32
#define SCANTYPE_ACTIVE     0
#define SCANTYPE_PASSIVE    1
typedef struct {
    uint32_t version;
    uint16_t action,
             sync_id;
    uint32_t ssidlen;
    uint8_t  ssid[SSID_MAXLEN],
             bssid[6],
             bss_type,
             scan_type;
    uint32_t nprobes,
             active_time,
             passive_time,
             home_time;
    uint16_t nchans,
             nssids;
    uint8_t  chans[14][2],
             ssids[1][SSID_MAXLEN];
} SCAN_PARAMS;
// corresponding to wl_escan_params_t in whd_wlioctl.h 
SCAN_PARAMS scan_params = {
    .version=1, .action=1, .sync_id=0x1234, .ssidlen=0, .ssid={0}, 
    .bssid={0xff,0xff,0xff,0xff,0xff,0xff}, .bss_type=2,
    .scan_type=SCANTYPE_PASSIVE, .nprobes=~0, .active_time=~0,
    .passive_time=~0, .home_time=~0, 
#if SCAN_CHAN == 0
    .nchans=14, .nssids=0, 
    .chans={{1,0x2b},{2,0x2b},{3,0x2b},{4,0x2b},{5,0x2b},{6,0x2b},{7,0x2b},
      {8,0x2b},{9,0x2b},{10,0x2b},{11,0x2b},{12,0x2b},{13,0x2b},{14,0x2b}},
#else
    .nchans=1, .nssids=0, .chans={{SCAN_CHAN,0x2b}}, .ssids={{0}}
#endif
};
uint8_t eventbuff[1600];
typedef struct {
    uint8_t pad[10];
    whd_event_t event;
    wl_escan_result_t escan;
} __attribute__((packed)) escan_result;
#define SIZE_ sizeof(escan_result) + sizeof(whd_event_t) + sizeof(wl_escan_result_t)
// Display SSID, prefixed with length byte
static inline void disp_ssid(uint8_t *data) {
    int i=*data++;
    if (i == 0 || *data == 0) {
        printk("[hidden]");
    } else if (i <= SSID_MAXLEN) {
        while (i --> 0) {
            uart_putc(*data++);
        }
    } else {
        printk("[invalid length %u]", i);
        i = *(data-2); 
        printk("[trying %u]", i); 
        // data -= 2; 
        while (i --> 0) {
            uart_putc(*data++);
        }
    }       
}

static inline void test_ioctl_scan() {
    int ticks=0, ledon=0, n;
    uint32_t val=0;
    uint8_t resp[256] = {0}, eth[7]={0};
    IOCTL_EVENT_HDR ieh;
    escan_result *erp = (escan_result *)eventbuff;

    wlan_up(); 

    // why clear the interrupt? 
    assert(sdio_bak_write32(SB_INT_STATUS_REG, val));
    assert(sdio_cmd53_read(SDIO_FN2, SB_32BIT_WIN, (uint8_t *)resp, 64));

    assert(ioctl_enable_evts(escan_evts));
    printk("events set\n"); 
    
    assert(ioctl_set_data("escan", 0, &scan_params, sizeof(scan_params)));
    int cnt = 0; 
    printk("done setting escan parameters\n"); 
    while (1) {
        delay_ms(10); 
        n = ioctl_get_event(&ieh, eventbuff, sizeof(eventbuff));
        if (n >= sizeof(escan_result)) {
            printk("%u bytes\n", n);
            print_mac_addr((uint8_t *)&erp->event.whd_event.addr);
            printk(" %2u ", SWAP16(erp->escan.bss_info->chanspec));
            disp_ssid(&erp->escan.bss_info->SSID_len);
            printk("\n");
        } else {
            printk("%d.", n);
        }
        cnt += 1; 
        if (cnt > 100) {
            break; 
        }
    }
    return; 
}   



#endif 
