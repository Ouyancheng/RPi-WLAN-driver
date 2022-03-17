#ifndef __MBOX_H__
#define __MBOX_H__
#include "rpi.h"
//
// rpi mailbox interface.
//  https://github.com/raspberrypi/firmware/wiki/Mailbox-property-interface
//
//  a more reasonable but unofficial writeup:
//  http://magicsmoke.co.za/?p=284
//
// General format [in 32-bit words]:
//   word 0: total size of message [in bytes] 
//   word 1: 0 --- always when sending
//   word 2: tag value
//   word 3: nbytes in reply
//   word 4: nbytes of request 
//        [... nbytes of space.  size=max(nbytes reply, nbytes request)]
//   last word: 0   end tag
//
//   so: value of word 0 = 6*4 + max(response size, request size)

#include "rpi.h"

enum { OneMB = 1024 * 1024 };

/***********************************************************************
 * mailbox interface
 */

#define MAILBOX_FULL   (1<<31)
#define MAILBOX_EMPTY  (1<<30)
#define MAILBOX_START  0x2000B880
#define GPU_MEM_OFFSET    0x40000000

// document states: only using 8 right now.
#define MBOX_CH  8

/*
    REGISTER	ADDRESS
    Read	        0x2000B880
    Poll	        0x2000B890
    Sender	        0x2000B894
    Status	        0x2000B898
    Configuration	0x2000B89C
    Write	        0x2000B8A0
 */
#define MBOX_READ   0x2000B880
#define MBOX_STATUS 0x2000B898
#define MBOX_WRITE  0x2000B8A0
extern volatile uint32_t *u;
// need to pass in the pointer as a GPU address?
static inline uint32_t uncached(volatile void *cp) { 
    // not sure this is needed: we have no data caching
    // since no VM
	return (unsigned)cp | GPU_MEM_OFFSET; 	
}

void 
mbox_write(unsigned channel, volatile void *data);

unsigned 
mbox_read(unsigned channel);

uint32_t 
mbox_send(unsigned channel, volatile void *data);

uint32_t rpi_get_revision(void);
uint32_t rpi_get_model(void);
uint32_t rpi_clock_curhz_get(uint32_t clock);
uint32_t rpi_clock_hz_set(uint32_t clock, uint32_t hz);
uint32_t rpi_clock_maxhz_get(uint32_t clock);
uint32_t rpi_get_memsize(void);
uint64_t rpi_get_serialnum(void);

uint32_t rpi_get_clock_rate_hz(uint32_t clock_id);

#endif
