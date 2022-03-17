#ifndef WLAN_H 
#define WLAN_H 

#include "wlan/wlan-ioctl.h"
#define CHECK(f, a, ...) {if (!f(a, __VA_ARGS__)) \
    printk("Error: %s(%s ...)\n", #f, #a);}
int wlan_init();
int wlan_shutdown(); 
int wlan_up(); 

// use 
// $ iw dev wlan0 station dump 
// in RPi AP mode to see the device 
int wlan_join(
    const char *ssid, 
    const char *country, 
    int country_rev, 
    int security, 
    const char *passphrase
); 

int wlan_listen_to_events(EVT_STR *events_to_listen);

/// just to test, not intended for use, and it doesn't work actually 
int wlan_test_send_frame(uint8_t *dest_mac, uint8_t *data, uint32_t len); 

#endif 

