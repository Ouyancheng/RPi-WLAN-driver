#include "wlan/wlan.h"
#include "wlan/whd/whd_events.h"
#include "wlan/wlan-backplane.h"
#include "wlan/wlan-ioctl.h"
#include "wlan/wlan-load-firmware.h"
#include <stdint.h>
uint8_t eventbuffer[2048];
#define DISPLAY_FRAME 1 
#define PRINT_DATA_AS_STRING 1
uint8_t mac_addr[7] = {0};
int wlan_init() {
    set_ether_func_gpio(); 
    int ret = sdio_init(); 
    assert(ret == 0); 
    uint8_t buffer[4];
    uint8_t resp[256] = {0}; 
    uint32_t chip_id = get_chip_id(); 
    printk("chip_id = %x\n", chip_id); // 0xA6 0xA9 0x41 0x15, or 0x1541a9a6 

    backplane_init(ctl); 
    printk("chipcommon=%x, armcore=%x, armctl=%x, armregs=%x, socramctl=%x, socramregs=%x, socramrev=%x, sdregs=%x, sdiorev=%x, d11ctl=%x\n", 
        ctl->chipcommon, 
        ctl->armcore, 
        ctl->armctl, 
        ctl->armregs, 
        ctl->socramctl, 
        ctl->socramregs, 
        ctl->socramrev, 
        ctl->sdregs, 
        ctl->sdiorev, 
        ctl->d11ctl
    );
    printk("uploading firmware...\n"); 
    assert(firmware_upload() == 0); 
    printk("FIRMWARE Upload Done!!!\n");
    assert(backplane_enable(ctl) == 0); 
    printk("wait for interrupt\n"); 
    uint32_t intr = 0; 
    emmc_wait_for_interrupt(EMMC_INTERRUPT_CARD, 3000); 
    printk("got interrupt\n"); 
    card_interrupt_handler(emmc.interrupt_val); 
    delay_ms(500); 

    printk("performing the rest of the initialization...\n"); 
    backplane_init_enable_unknown_tailing_sequence(); 
    ////// MUST READ THIS!!!!!! 
    sdio_perform_io(0, SDIO_FN2, BACKPLANE_32BIT_ADDR_BASE, resp, 64, 1); 
    printk("SUCCESS!!!\n"); 
    return 0; 
}

int wlan_shutdown() {
    // shutdown: wlcmdint(ctlr, 3, 0);        /* DOWN */ 
    // and then reset sdio 
    int ret = ioctl_wr_int32(3, 100, 0);
    if (ret == 0) {
        return -1; 
    } 
    return sdio_reset(); 
}
#define DEFAULT_WAIT_MSEC 10
int wlan_up() {
    int n = 0; 
    uint8_t resp[128] = {0}; 
    n = ioctl_get_data("cur_etheraddr", 0, mac_addr, 6);
    printk("MAC address ");
    if (n) {
        print_mac_addr(mac_addr);
    } else {
        printk("unavailable");
        return -1; 
    }
    n = ioctl_get_data("ver", 0, resp, sizeof(resp));
    printk("\nFirmware %s\n", (n ? (char *)resp : "not responding"));
    if (!n) {
        return -2; 
    }


    /// wlsetint(ctlr, "assoc_listen", 10);
    ioctl_set_uint32("assoc_listen", DEFAULT_WAIT_MSEC, 10); 
    /// wlcmdint(ctlr, 0x56, 0);        /* powersave off */
    ioctl_wr_int32(0x56, DEFAULT_WAIT_MSEC, 0); 
    /** Are we doing tx glom mode? If so we need to use the glom header for the tx packets!!! */
    /// wlsetint(ctlr, "bus:txglom", 0);
    ioctl_set_uint32("bus:txglom", DEFAULT_WAIT_MSEC, 0); 
    /// wlsetint(ctlr, "bcn_timeout", 10);
    ioctl_set_uint32("bcn_timeout", DEFAULT_WAIT_MSEC, 10); 
    /// wlsetint(ctlr, "assoc_retry_max", 3);
    ioctl_set_uint32("assoc_retry_max", DEFAULT_WAIT_MSEC, 3); 
    /// wlcmdint(ctlr, 0xb9, 0x28); /* SET_SCAN_CHANNEL_TIME */
    ioctl_wr_int32(0xb9, DEFAULT_WAIT_MSEC, 0x28); 
    /// wlcmdint(ctlr, 0xbb, 0x28); /* SET_SCAN_UNASSOC_TIME */
    ioctl_wr_int32(0xbb, DEFAULT_WAIT_MSEC, 0x28); 
    /// wlcmdint(ctlr, 0x102, 0x82); /* SET_SCAN_PASSIVE_TIME */
    ioctl_wr_int32(0x102, DEFAULT_WAIT_MSEC, 0x82); 

    if (!ioctl_wr_int32(WLC_UP, 200, 0)) {
        printk("WiFi CPU not running\n");
        return -3; 
    }
    printk("CPU up!\n"); 

    /// wlsetint(ctlr, "roam_off", 1);
    ioctl_set_uint32("roam_off", DEFAULT_WAIT_MSEC, 1); 
    /// wlcmdint(ctlr, 0x14, 1);    /* SET_INFRA 1 */
    // ioctl_wr_int32(0x14, DEFAULT_WAIT_MSEC, 1); 
    /// wlcmdint(ctlr, 10, 0);      /* SET_PROMISC */
    ioctl_wr_int32(10, DEFAULT_WAIT_MSEC, 0); 
    /// wlcmdint(ctlr, 2, 1);       /* UP */ again??? 
    ioctl_wr_int32(WLC_UP, DEFAULT_WAIT_MSEC, 1); 

    return 0; 
}
void disp_bytes(uint8_t *data, int len); 
void disp_block(uint8_t *data, int len);
wlc_ssid_t ssid_struct; 
wl_country_t country_struct; 
wsec_pmk_t pmk_struct; 
int wlan_join(
    const char *ssid, 
    const char *country, 
    int country_rev, 
    int security, 
    const char *passphrase
) {
    // SSID 
    unsigned ssid_len = strlen(ssid); 
    if (ssid_len >= 32) {
        printk("SSID too long!\n"); 
        return -1; 
    }
    memset(&ssid_struct, 0, sizeof(ssid_struct)); 
    ssid_struct.SSID_len = ssid_len; 
    memcpy((void*)ssid_struct.SSID, ssid, ssid_len); 
    // country 
    memset(&country_struct, 0, sizeof(country_struct)); 
    country_struct.rev = country_rev; 
    strcpy((char*)country_struct.ccode, country); 
    strcpy((char*)country_struct.country_abbrev, country);
    // passphrase 
    memset(&pmk_struct, 0, sizeof(pmk_struct)); 
    if (security) {
        pmk_struct.key_len = strlen(passphrase); 
        pmk_struct.flags = WSEC_PASSPHRASE; 
        strcpy((char*)pmk_struct.key, passphrase); 
    }

    if (!ioctl_set_data("country", 1000, &country_struct, sizeof(country_struct))) {
        printk("Can't set country\n");
        return -2; 
    }

    EVT_STR join_evts[] = JOIN_EVTS; 
    EVT_STR no_evts[] = NO_EVTS; 
    ioctl_enable_evts(no_evts);
    CHECK(ioctl_wr_int32, WLC_SET_INFRA, 50, 1);
    CHECK(ioctl_wr_int32, WLC_SET_AUTH, 0, 0);
    if (security) {
        CHECK(ioctl_wr_int32, WLC_SET_WSEC, 0, security==2 ? 6 : 2);
        CHECK(ioctl_set_intx2, "bsscfg:sup_wpa", 0, 0, 1);
        CHECK(ioctl_set_intx2, "bsscfg:sup_wpa2_eapver", 0, 0, -1);
        CHECK(ioctl_set_intx2, "bsscfg:sup_wpa_tmo", 0, 0, 2500);
        CHECK(ioctl_wr_data, WLC_SET_WSEC_PMK, 0, &pmk_struct, sizeof(pmk_struct));
        CHECK(ioctl_wr_int32, WLC_SET_WPA_AUTH, 0, security==2 ? 0x80 : 4);
    } else {
        CHECK(ioctl_wr_int32, WLC_SET_WSEC, 0, 0);
        CHECK(ioctl_wr_int32, WLC_SET_WPA_AUTH, 0, 0);
    }

    ioctl_enable_evts(join_evts); 

    CHECK(ioctl_wr_data, WLC_SET_SSID, 100, &ssid_struct, sizeof(ssid_struct)); 
    IOCTL_EVENT_HDR ieh;
    ETH_EVENT_FRAME *eep = (ETH_EVENT_FRAME *)eventbuffer;
    int n = 0; 
    // int cnt = 1; 
    while (1) {
        delay_ms(20); 
        if ((n = ioctl_get_event(&ieh, eventbuffer, sizeof(eventbuffer))) > 0) {
            printk("--------------------------------------------------------\n"); 
            disp_fields(&ieh, ioctl_event_hdr_fields, n);
            printk("\n");
            disp_bytes((uint8_t *)&ieh, sizeof(ieh));
            printk("\n");
            disp_fields(&eep->eth_hdr, eth_hdr_fields, sizeof(eep->eth_hdr));
            uint16_t ether_frame_type = SWAP16(eep->eth_hdr.ethertype); 

            switch (ether_frame_type) {
            case 0x886c: {
                /// NOTE: 0x886C seems to be a private value 
                /// it's not documented here: https://www.iana.org/assignments/ieee-802-numbers/ieee-802-numbers.xhtml#ieee-802-numbers-1 
                printk("IOCTL event frame received\n");
                disp_fields(&eep->event.hdr, event_hdr_fields, sizeof(eep->event.hdr));
                printk("\n");
                disp_fields(&eep->event.msg, event_msg_fields, sizeof(eep->event.msg));
                uint32_t event_type = SWAP32(eep->event.msg.event_type); 
                uint32_t event_status = SWAP32(eep->event.msg.status); 
                printk("%s %s", ioctl_evt_str(event_type),
                        ioctl_evt_status_str(event_status));
                if (event_type == WLC_E_SET_SSID) {
                    if (event_status == 0) {
                        printk("join success!!!\n"); 
                        return 0; 
                    } else {
                        printk("join failed!\n"); 
                        return -1; 
                    }
                }
            } break;
            case 0x800: {
                /**
                 * For a Data Frame the structure is: 
                 * ETH_EVENT_FRAME:
                 *   pad: 10 bytes (something we don't care about)
                 *   whd_event_ether_header_t: 14 bytes 
                 *     (this is the event header 
                 *       (part of, the full header should also include the previous 10 bytes, or 8 bytes? or else? )) 
                 *     dest_mac_addr: 6 bytes 
                 *     src_mac_addr: 6 bytes 
                 *     frame type: 2 bytes 
                 *   data: the rest of the bytes 
                 *   ... 
                 *   ... 
                 */
            
                printk("ETH_P_IP / IPv4 data frame received, data: \n"); 
                int datalen = n - sizeof(whd_event_ether_header_t) - 10; 
                for (int i = 0; i < datalen; ++i) {
                #if PRINT_DATA_AS_STRING
                    uart_putc(eep->data[i]); 
                #else
                    printk("%x, ", eep->data[i]);
                #endif 
                }
                printk("\n--------------------------------------------------------"); 
            } break; 
            default:
                /// refer to https://en.wikipedia.org/wiki/EtherType for more values 
                printk("unrecognized frame type\n"); 
                break;
            }
            printk("\n");
            disp_block(eventbuffer, n);
            printk("--------------------------------------------------------\n");
        }
    }
}

int wlan_listen_to_events(EVT_STR *events_to_listen) {
    if (events_to_listen != NULL) {
        ioctl_enable_evts(events_to_listen);
    }
    IOCTL_EVENT_HDR ieh;
    ETH_EVENT_FRAME *eep = (ETH_EVENT_FRAME *)eventbuffer;
    int n = 0; 
    while (1) {
        delay_ms(20); 
        if ((n = ioctl_get_event(&ieh, eventbuffer, sizeof(eventbuffer))) > 0) {
            printk("--------------------------------------------------------\n"); 
            disp_fields(&ieh, ioctl_event_hdr_fields, n);
            printk("\n");
            disp_bytes((uint8_t *)&ieh, sizeof(ieh));
            printk("\n");
            disp_fields(&eep->eth_hdr, eth_hdr_fields, sizeof(eep->eth_hdr));
            uint16_t ether_frame_type = SWAP16(eep->eth_hdr.ethertype); 
            switch (ether_frame_type) {
            case 0x886c: {
                /// NOTE: 0x886C seems to be a private value 
                /// it's not documented here: https://www.iana.org/assignments/ieee-802-numbers/ieee-802-numbers.xhtml#ieee-802-numbers-1 
                printk("IOCTL event frame received\n");
                disp_fields(&eep->event.hdr, event_hdr_fields, sizeof(eep->event.hdr));
                printk("\n");
                disp_fields(&eep->event.msg, event_msg_fields, sizeof(eep->event.msg));
                uint32_t event_type = SWAP32(eep->event.msg.event_type); 
                uint32_t event_status = SWAP32(eep->event.msg.status); 
                printk("%s %s", ioctl_evt_str(event_type),
                        ioctl_evt_status_str(event_status));
                if (event_type == WLC_E_SET_SSID) {
                    if (event_status == 0) {
                        printk("\njoin success!!!\n"); 
                        return 0; 
                    } else {
                        printk("\njoin failed!\n"); 
                        return -1; 
                    }
                }
            } break;
            case 0x800: {
                /**
                 * For a Data Frame the structure is: 
                 * ETH_EVENT_FRAME:
                 *   pad: 10 bytes (something we don't care about)
                 *   whd_event_ether_header_t: 14 bytes 
                 *     (this is the event header 
                 *       (part of, the full header should also include the previous 10 bytes, or 8 bytes? or else? )) 
                 *     dest_mac_addr: 6 bytes 
                 *     src_mac_addr: 6 bytes 
                 *     frame type: 2 bytes 
                 *   data: the rest of the bytes 
                 *   ... 
                 *   ... 
                 */
                printk("ETH_P_IP / IPv4 data frame received, data: \n"); 
                int datalen = n - sizeof(whd_event_ether_header_t) - 10; 
                for (int i = 0; i < datalen; ++i) {
                #if PRINT_DATA_AS_STRING
                    uart_putc(eep->data[i]); 
                #else
                    printk("%x, ", eep->data[i]);
                #endif 
                }
                printk("\n----------------------------------"); 
            } break; 
            default:
                /// refer to https://en.wikipedia.org/wiki/EtherType for more values 
                printk("unrecognized frame type\n"); 
                break;
            }
            printk("\n");
            disp_block(eventbuffer, n);
            printk("----------------------------------\n");
        }
    }
}

#define DISP_BLOCKLEN       32
void disp_block(uint8_t *data, int len) {
    int i=0, n;
    while (i < len) {
        if (i > 0)
            printk("\n");
        n = MIN(len-i, DISP_BLOCKLEN);
        disp_bytes(&data[i], n);
        i += n;
    }
}

void disp_bytes(uint8_t *data, int len) {
    while (len--) {
        printk("%x ", *data++);
    }
}

// typedef uint8_t uchar; 
uint8_t txbuffer[2048]; 
typedef unsigned char uchar; 
typedef struct Block
{
    struct Block *next;
    uchar *buf;
    uchar *lim;
    uchar *wp;
    uchar *rp;
    uchar data[0];
#define BLEN(b)		((b)->wp - (b)->rp)
}
Block;
Block *allocb(uint32_t size) {
    static const uint32_t maxhdrsize = 64; 
    size += sizeof(IOCTL_EVENT_HDR) + maxhdrsize; 
    Block *b = (Block*)txbuffer; 
    b->buf = b->data; 
    b->next = 0; 
    b->lim = b->buf + size; 
    b->wp = b->buf + maxhdrsize; 
    b->rp = b->wp; 
    return b; 
}
Block *padblock(Block *b, int size) {
    assert (size > 0);
    assert (b != 0);
    assert (b->rp - b->buf >= size);
    b->rp -= size;
    return b;
}
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
typedef struct Sdpcm Sdpcm; 
_Static_assert(sizeof(Sdpcm) == 12, "Sdpcm size != 12?"); 
#define FLIPENDIAN(x) ((((x) & 0xFF00) >> 8) | (((x) & 0x00FF) << 8))
void txstart(uint8_t *buffer, uint32_t size) {
    Block *b = allocb(size); 
    memcpy(b->wp, buffer, size);
    b->wp += size; 
    int len; 
    int off; 
    off = ((uintptr_t)b->rp & 3) + sizeof(Sdpcm); 
    b = padblock(b, off + 4); 
    len = BLEN(b); 
    Sdpcm *p;
    p = (Sdpcm*)b->rp;
    memset(p, 0, off); 
    serialize_int16(p->len, len); 
    serialize_int16(p->lenck, ~len); 
    p->chanflg = 2; 
    p->seq = txseq; 
    p->doffset = off; 
    serialize_int32(b->rp + off, 0x20); /* BDC header */
    printk("\n----------\n" 
            "b->rp = \n"); 
    for (int i = 0; i < len; ++i) {
        printk("%x ", b->rp[i]); 
    }
    printk("\n----------\n"); 
    int n = sdio_cmd53_write(SDIO_FN2, BACKPLANE_32BIT_ADDR_BASE, b->rp, ROUNDUP(len,4)); 
    ioctl_wait(IOCTL_WAIT_USEC);
    int wait_msec = 100; 
    uint32_t val=0;
    IOCTL_MSG rxmsg;
    IOCTL_MSG *rsp = &rxmsg;
    while (wait_msec>=0 && n==0) {
        // Wait for response to be available
        wait_msec -= IOCTL_POLL_MSEC;
        sdio_bak_read32(SB_INT_STATUS_REG, &val);
        // If response is waiting..
        if (val & 0xFF) {
            // ..request response
            sdio_bak_write32(SB_INT_STATUS_REG, val);
            // Fetch response
            n = sdio_cmd53_read(SDIO_FN2, SB_32BIT_WIN, (void *)rsp, len);
            // // Discard response if not matching request
            // if ((rsp->cmd.flags>>16) != ioctl_reqid) n = 0;
            // // Exit if error response
            // if (n && (rsp->cmd.flags & 1)) { n = 0; break; }
        }
        // If no response, wait
        else { 
            delay_us(IOCTL_POLL_MSEC * 1000);
            printk("noresponse!\n"); 
        }
    }
    if (n == 0) {
        printk("Frame failed to send!\n"); 
        return; 
    }
    printk("Frame written to WLAN memory\n"); 
    txseq += 1; 
}
#define ETH_P_IP 0x800
uint8_t framebuf[128]; 
int send_raw_ip_packet(uint8_t *src_mac, uint8_t *dst_mac, const char *ippacket, uint32_t len) {
    whd_event_ether_header_t *petherheader = (whd_event_ether_header_t*)framebuf;
    for (int i = 0; i < 6; ++i) {
        petherheader->source_address.octet[i] = src_mac[i];
    }
    for (int i = 0; i < 6; ++i) {
        petherheader->destination_address.octet[i] = dst_mac[i];
    }
    petherheader->ethertype = FLIPENDIAN(ETH_P_IP); 
    
    memcpy((uint8_t*)framebuf + sizeof(whd_event_ether_header_t), ippacket, len); 
    
    len += sizeof(whd_event_ether_header_t); 
    txstart(framebuf, len); 

    return 0; 
}

int wlan_test_send_frame(uint8_t *dest_mac, uint8_t *framedata, uint32_t len) {
    int n = 0; 
    send_raw_ip_packet(mac_addr, dest_mac, framedata, len);
    delay_ms(5000); 
    for (int i = 0; i < 10; ++i) {
        send_raw_ip_packet(mac_addr, dest_mac, framedata, len); 
        delay_ms(5000); 
    }
#if 0 
    uint32_t frame_sz = 
        sizeof(IOCTL_EVENT_HDR) 
        + 10  // padding? 
        + sizeof(whd_event_ether_header_t)  // ethernet header 
        + len; 
    // round to a multiple of 4, the +1 might not be necessary 
    memset((void*)txbuffer, 0, ROUNDUP(frame_sz, 4));  
    IOCTL_EVENT_HDR *txhdrp = (IOCTL_EVENT_HDR*)txbuffer; 
    uint8_t *datap = 
        (uint8_t*)txbuffer 
        + sizeof(IOCTL_EVENT_HDR) 
        + 10 
        + sizeof(whd_event_ether_header_t); 
    whd_event_ether_header_t *ethp = (whd_event_ether_header_t*)(
        (uint8_t*)txbuffer 
        + sizeof(IOCTL_EVENT_HDR)
        + 10
    ); 
    // uint32_t framesize = frame_sz - sizeof(IOCTL_EVENT_HDR); 
    txhdrp->len = frame_sz; 
    txhdrp->notlen = ~frame_sz; 
    txhdrp->seq = txseq; 
    txhdrp->chan = 2; 
    txhdrp->nextlen = 0; 
    txhdrp->hdrlen = sizeof(IOCTL_EVENT_HDR) + 4; 
    _Static_assert(sizeof(IOCTL_EVENT_HDR) + 2 == 0xe, "hdrlen should be 0xe!"); 
    txhdrp->flow = 0x00; // ? 
    txhdrp->credit = 0x00; // ? 0x23 
    txhdrp->reserved[0] = 0x0; // ? 
    txhdrp->reserved[1] = 0; 
    uint8_t *padp = (uint8_t*)txbuffer+sizeof(IOCTL_EVENT_HDR); 
    uint8_t padding[10] = {
        0x0, 
        0x0,  // 
        0x20, // 0x20, 
        0x00, // 0x3, the header is up here? 
        0x0, 
        0x00,// 0x1, 
        0x0, 
        0x00, // 0x1, 
        0x00, // 0xca, 
        0x0
    };
    memcpy(padp, padding, 10); 
    ethp->destination_address.octet[0] = dest_mac[0];
    ethp->destination_address.octet[1] = dest_mac[1];
    ethp->destination_address.octet[2] = dest_mac[2];
    ethp->destination_address.octet[3] = dest_mac[3];
    ethp->destination_address.octet[4] = dest_mac[4];
    ethp->destination_address.octet[5] = dest_mac[5];

    ethp->source_address.octet[0] = mac_addr[0]; 
    ethp->source_address.octet[1] = mac_addr[1]; 
    ethp->source_address.octet[2] = mac_addr[2]; 
    ethp->source_address.octet[3] = mac_addr[3]; 
    ethp->source_address.octet[4] = mac_addr[4]; 
    ethp->source_address.octet[5] = mac_addr[5]; 

    ethp->ethertype = ETH_P_IP; 

    memcpy(datap, framedata, len); 


    
    n = sdio_cmd53_write(
        SDIO_FN2, 
        BACKPLANE_32BIT_ADDR_BASE, 
        (uint8_t*)txbuffer, 
        ROUNDUP(frame_sz, 4)
    ); 
    
    ioctl_wait(IOCTL_WAIT_USEC);
    int wait_msec = 100; 
    uint32_t val=0;
    IOCTL_MSG rxmsg;
    IOCTL_MSG *rsp = &rxmsg;
    while (wait_msec>=0 && n==0)
    {
        // Wait for response to be available
        wait_msec -= IOCTL_POLL_MSEC;
        sdio_bak_read32(SB_INT_STATUS_REG, &val);
        // If response is waiting..
        if (val & 0xFF)
        {
            // ..request response
            sdio_bak_write32(SB_INT_STATUS_REG, val);
            // Fetch response
            n = sdio_cmd53_read(SDIO_FN2, SB_32BIT_WIN, (void *)rsp, frame_sz);
            // Discard response if not matching request
            // if ((rsp->cmd.flags>>16) != ioctl_reqid)
            //     n = 0;
            // // Exit if error response
            // if (n && (rsp->cmd.flags & 1))
            // {
            //     n = 0;
            //     break;
            // }
        }
        // If no response, wait
        else { 
            delay_us(IOCTL_POLL_MSEC * 1000);
            printk("noresponse!\n"); 
        }
    }
    if (n == 0) {
        printk("Frame failed to send!\n"); 
        return -1; 
    }
    printk("Frame written to WLAN memory\n"); 
    txseq++; 

#endif 




#define WLC_E_TXFAIL 20 
    int exclude_events[] = {
        44, 71, 72, 
        -1
    };
    ioctl_enable_all_evts((int*)exclude_events); 
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
    // ioctl_enable_evts(evts); 

    IOCTL_EVENT_HDR ieh;
    ETH_EVENT_FRAME *eep = (ETH_EVENT_FRAME *)eventbuffer;
    
    while (1) {
        delay_ms(20); 
        if ((n = ioctl_get_event(&ieh, eventbuffer, sizeof(eventbuffer))) > 0) {
            printk("--------------------------------------------------------\n"); 
            disp_fields(&ieh, ioctl_event_hdr_fields, n);
            printk("\n");
            disp_bytes((uint8_t *)&ieh, sizeof(ieh));
            printk("\n");
            disp_fields(&eep->eth_hdr, eth_hdr_fields, sizeof(eep->eth_hdr));
            uint16_t ether_frame_type = SWAP16(eep->eth_hdr.ethertype); 
            switch (ether_frame_type) {
            case 0x886c: {
                /// NOTE: 0x886C seems to be a private value 
                /// it's not documented here: https://www.iana.org/assignments/ieee-802-numbers/ieee-802-numbers.xhtml#ieee-802-numbers-1 
                printk("IOCTL event frame received\n");
                disp_fields(&eep->event.hdr, event_hdr_fields, sizeof(eep->event.hdr));
                printk("\n");
                disp_fields(&eep->event.msg, event_msg_fields, sizeof(eep->event.msg));
                uint32_t event_type = SWAP32(eep->event.msg.event_type); 
                uint32_t event_status = SWAP32(eep->event.msg.status); 
                printk("%s %s", ioctl_evt_str(event_type),
                        ioctl_evt_status_str(event_status));
            } break;
            case 0x800: {
                /**
                 * For a Data Frame the structure is: 
                 * ETH_EVENT_FRAME:
                 *   pad: 10 bytes (something we don't care about)
                 *   whd_event_ether_header_t: 14 bytes 
                 *     (this is the event header 
                 *       (part of, the full header should also include the previous 10 bytes, or 8 bytes? or else? )) 
                 *     dest_mac_addr: 6 bytes 
                 *     src_mac_addr: 6 bytes 
                 *     frame type: 2 bytes 
                 *   data: the rest of the bytes 
                 *   ... 
                 *   ... 
                 */
                printk("ETH_P_IP / IPv4 data frame received, data: \n"); 
                int datalen = n - sizeof(whd_event_ether_header_t) - 10; 
                for (int i = 0; i < datalen; ++i) {
                #if PRINT_DATA_AS_STRING
                    uart_putc(eep->data[i]); 
                #else
                    printk("%x, ", eep->data[i]);
                #endif 
                }
                printk("\n----------------------------------"); 
            } break; 
            default:
                /// refer to https://en.wikipedia.org/wiki/EtherType for more values 
                printk("unrecognized frame type\n"); 
                break;
            }
            printk("\n");
            disp_block(eventbuffer, n);
            printk("\n----------------------------------\n");
        }
    }
}


