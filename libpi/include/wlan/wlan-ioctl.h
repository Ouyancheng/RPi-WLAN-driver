// ZeroWi bare-metal WiFi driver, see https://iosoft.blog/zerowi
// IOCTL interface definitions
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

/// File instrumented by Yancheng Ou 
#ifndef WLAN_IOCTL_H 
#define WLAN_IOCTL_H 
#include "rpi.h"
#include "wlan/wlan-sdio.h"
#include "wlan/wlan-backplane.h"
#include "wlan/whd/whd.h"
#include "wlan/whd/whd_events.h"
#include "wlan/whd/whd_types.h"
#include "wlan/whd/whd_wlioctl.h"
#ifndef SD_FUNC_RAD
#define SD_FUNC_RAD SDIO_FN2 
#endif 
#ifndef SD_FUNC_BAK
#define SD_FUNC_BAK SDIO_FN1 
#endif 
static inline void print_mac_addr(uint8_t *data) {
    for (unsigned i = 0; i < 6; ++i) {
        if (i > 0) {
            printk(":");
        }
        printk("%02x", data[i]); 
    }
}
#define SWAP16(x) ((x&0xff)<<8 | (x&0xff00)>>8)
#define SWAP32(x) ((x&0xff)<<24 | (x&0xff00)<<8 | (x&0xff0000)>>8 | (x&0xff000000)>>24)

#define IOCTL_WAIT_USEC     2000
#define IOCTL_MAX_BLKLEN    256

// Event structures
typedef struct {
    whd_event_eth_hdr_t   hdr;
    struct whd_event_msg  msg;
    uint8_t data[1];  // placeholder 
} ETH_EVENT;
_Static_assert(sizeof(whd_event_eth_hdr_t) == 10, "whd_event_eth_hdr_t size is not 10!"); 
_Static_assert(sizeof(struct whd_event_msg) == 48, "whd_event_msg size is not 48!"); 
typedef struct {
    uint8_t pad[10];
    whd_event_ether_header_t eth_hdr;
    union {
        ETH_EVENT event;
        uint8_t data[1];
    };
} ETH_EVENT_FRAME;
_Static_assert(sizeof(ETH_EVENT_FRAME) - 10 - 1 == sizeof(whd_event_t), "sizeof the event frames mismatch!");
typedef struct {
    uint8_t  seq,       // sdpcm_sw_header
             chan,
             nextlen,
             hdrlen,
             flow,
             credit,
             reserved[2];
    uint32_t cmd;       // cdc_header
    uint16_t outlen,
             inlen;
    uint32_t flags,
             status;
    uint8_t data[IOCTL_MAX_BLKLEN];
} IOCTL_CMD;
_Static_assert(sizeof(IOCTL_CMD) == 280, "IOCTL_CMD is something other than 280? (but it could be OK)"); 
typedef struct {
    uint16_t len;
    uint8_t  reserved1,
             flags,
             reserved2[2],
             pad[2];
} IOCTL_GLOM_HDR;
_Static_assert(sizeof(IOCTL_GLOM_HDR) == 8, "ioctl glom header size is not 8!"); 

typedef struct {
    IOCTL_GLOM_HDR glom_hdr;
    IOCTL_CMD  cmd;
} IOCTL_GLOM_CMD;

typedef struct
{
    uint16_t len,           // sdpcm_header.frametag
             notlen;
    union 
    {
        IOCTL_CMD cmd;
        IOCTL_GLOM_CMD glom_cmd;
    };
} IOCTL_MSG;

typedef struct {
    uint16_t len,       // sdpcm_header.frametag
             notlen;
    uint8_t  seq,       // sdpcm_sw_header
             chan,
             nextlen,
             hdrlen,
             flow,
             credit,
             reserved[2];
} IOCTL_EVENT_HDR;
_Static_assert(sizeof(IOCTL_EVENT_HDR) == 12, "event header size is not 12!"); 
/// NOTE: compare IOCTL_EVENT_HDR with this: 
/*
struct Sdpcm {
    uchar   len[2];
    uchar   lenck[2];
    uchar   seq;
    uchar   chanflg;
    uchar   nextlen;
    uchar   doffset;
    uchar   fcmask;
    uchar   window;
    uchar   version;
    uchar   pad;
};
*/
#define SSID_MAXLEN         32

#define EVENT_SET_SSID      0
#define EVENT_JOIN          1
#define EVENT_AUTH          3
#define EVENT_LINK          16
#define EVENT_MAX           208
#define SET_EVENT(msk, e)   msk[e/8] |= 1 << (e & 7)

typedef struct {
    int num;
    char *str;
} EVT_STR;
#define EVT(e)      {e, #e}

#define NO_EVTS     {EVT(-1)}
#define ESCAN_EVTS  {EVT(WLC_E_ESCAN_RESULT), EVT(-1)}
#define JOIN_EVTS   {EVT(WLC_E_SET_SSID), EVT(WLC_E_LINK), EVT(WLC_E_AUTH), \
        EVT(WLC_E_DEAUTH_IND), EVT(WLC_E_DISASSOC_IND), EVT(WLC_E_PSK_SUP), EVT(-1)}

extern char ioctl_event_hdr_fields[];
extern char eth_hdr_fields[]; 
extern char event_hdr_fields[];
extern char event_msg_fields[];
extern int txglom;
extern uint16_t ioctl_reqid;
extern uint8_t txseq;

int ioctl_get_event(IOCTL_EVENT_HDR *hp, uint8_t *data, int maxlen);
int ioctl_enable_evts(EVT_STR *evtp);
char *ioctl_evt_str(int event);
char *ioctl_evt_status_str(int status);
int ioctl_get_data(char *name, int wait_msec, uint8_t *data, int dlen);
int ioctl_set_uint32(char *name, int wait_msec, uint32_t val);
int ioctl_set_intx2(char *name, int wait_msec, int val1, int val2);
int ioctl_set_data(char *name, int wait_msec, void *data, int len);
int ioctl_wr_int32(int cmd, int wait_msec, int val);
int ioctl_wr_data(int cmd, int wait_msec, void *data, int len);
int ioctl_cmd(int cmd, char *name, int wait_msec, int wr, void *data, int dlen);
int ioctl_wait(int usec);
int ioctl_ready(void);
void disp_fields(void *data, char *fields, int maxlen);

// EOF
// int ioctl_enable_all_evts();
int ioctl_enable_all_evts(int *exclude_events);  

int sdio_cmd53_read(int func, int addr, uint8_t *dp, int nbytes);

int sdio_cmd53_write(int func, int addr, uint8_t *dp, int nbytes);
void sdio_bak_window(uint32_t addr);
int sdio_bak_write32(uint32_t addr, uint32_t val);
// Read a 32-bit value via the backplane window
int sdio_bak_read32(uint32_t addr, uint32_t *valp);

#define BUS_IOEN_REG            0x002   // SDIOD_CCCR_IOEN          I/O enable
#define BUS_IORDY_REG           0x003   // SDIOD_CCCR_IORDY         Ready indication
#define BUS_INTEN_REG           0x004   // SDIOD_CCCR_INTEN
#define BUS_INTPEND_REG         0x005   // SDIOD_CCCR_INTPEND
#define BUS_BI_CTRL_REG         0x007   // SDIOD_CCCR_BICTRL        Bus interface control
#define BUS_SPEED_CTRL_REG      0x013   // SDIOD_CCCR_SPEED_CONTROL Bus speed control  
#define BUS_BRCM_CARDCAP        0x0f0   // SDIOD_CCCR_BRCM_CARDCAP
#define BUS_BAK_BLKSIZE_REG     0x110   // SDIOD_CCCR_F1BLKSIZE_0   Backplane blocksize 
#define BUS_RAD_BLKSIZE_REG     0x210   // SDIOD_CCCR_F2BLKSIZE_0   WiFi radio blocksize

// Backplane config registers
#define BAK_WIN_ADDR_REG        0x1000a // SDIO_BACKPLANE_ADDRESS_LOW Window addr 
#define BAK_CHIP_CLOCK_CSR_REG  0x1000e // SDIO_CHIP_CLOCK_CSR      Chip clock ctrl 
#define BAK_PULLUP_REG          0x1000f // SDIO_PULL_UP             Pullups
#define BAK_WAKEUP_REG          0x1001e // SDIO_WAKEUP_CTRL

// Silicon backplane
#define BAK_BASE_ADDR           0x18000000              // CHIPCOMMON_BASE_ADDRESS
                                                        //
#define MAC_BASE_ADDR           (BAK_BASE_ADDR+0x1000)  // DOT11MAC_BASE_ADDRESS
#define MAC_BASE_WRAP           (MAC_BASE_ADDR+0x100000)
#define MAC_IOCTRL_REG          (MAC_BASE_WRAP+0x408)   // +AI_IOCTRL_OFFSET
#define MAC_RESETCTRL_REG       (MAC_BASE_WRAP+0x800)   // +AI_RESETCTRL_OFFSET
#define MAC_RESETSTATUS_REG     (MAC_BASE_WRAP+0x804)   // +AI_RESETSTATUS_OFFSET

#define SB_BASE_ADDR            (BAK_BASE_ADDR+0x2000)  // SDIO_BASE_ADDRESS
#define SB_INT_STATUS_REG       (SB_BASE_ADDR +0x20)    // SDIO_INT_STATUS
#define SB_INT_HOST_MASK_REG    (SB_BASE_ADDR +0x24)    // SDIO_INT_HOST_MASK
#define SB_FUNC_INT_MASK_REG    (SB_BASE_ADDR +0x34)    // SDIO_FUNCTION_INT_MASK
#define SB_TO_SB_MBOX_REG       (SB_BASE_ADDR +0x40)    // SDIO_TO_SB_MAILBOX
#define SB_TO_SB_MBOX_DATA_REG  (SB_BASE_ADDR +0x48)    // SDIO_TO_SB_MAILBOX_DATA
#define SB_TO_HOST_MBOX_DATA_REG (SB_BASE_ADDR+0x4C)    // SDIO_TO_HOST_MAILBOX_DATA

#define ARM_BASE_ADDR           (BAK_BASE_ADDR+0x3000)  // WLAN_ARMCM3_BASE_ADDRESS
#define ARM_BASE_WRAP           (ARM_BASE_ADDR+0x100000)
#define ARM_IOCTRL_REG          (ARM_BASE_WRAP+0x408)   // +AI_IOCTRL_OFFSET
#define ARM_RESETCTRL_REG       (ARM_BASE_WRAP+0x800)   // +AI_RESETCTRL_OFFSET
#define ARM_RESETSTATUS_REG     (ARM_BASE_WRAP+0x804)   // +AI_RESETSTATUS_OFFSET

#define SRAM_BASE_ADDR          (BAK_BASE_ADDR+0x4000)  // SOCSRAM_BASE_ADDRESS
#define SRAM_BANKX_IDX_REG      (SRAM_BASE_ADDR+0x10)   // SOCSRAM_BANKX_INDEX
#define SRAM_UNKNOWN_REG        (SRAM_BASE_ADDR+0x40)   // ??
#define SRAM_BANKX_PDA_REG      (SRAM_BASE_ADDR+0x44)   // SOCSRAM_BANKX_PDA
#define SRAM_BASE_WRAP          (SRAM_BASE_ADDR+0x100000)
#define SRAM_IOCTRL_REG         (SRAM_BASE_WRAP+0x408)  // +AI_IOCTRL_OFFSET
#define SRAM_RESETCTRL_REG      (SRAM_BASE_WRAP+0x800)  // +AI_RESETCTRL_OFFSET
#define SRAM_RESETSTATUS_REG    (SRAM_BASE_WRAP+0x804)  // +AI_RESETSTATUS_OFFSET

// Save-restore
#define SR_CONTROL1             (BAK_BASE_ADDR+0x508)   // CHIPCOMMON_SR_CONTROL1

// Backplane window
#define SB_32BIT_WIN    0x8000
#define SB_ADDR_MASK    0x7fff
#define SB_WIN_MASK     (~SB_ADDR_MASK)
#define IOCTL_POLL_MSEC     2
#ifndef MIN 
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif 
#ifndef MAX 
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif 

#define SECURITY_WPA2 2 
#define SECURITY_WPA_TKIP 1 
#define SECURITY_NONE 0 
const char *ioctl_get_event_name(uint32_t event); 
#endif 

