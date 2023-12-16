#ifndef _RTC_HANDLER_H
#define _RTC_HANDLER_H

#include "lib.h"
#include "x86_desc.h"
#include "types.h"
#include "PCB.h"
#include "terminal.h"


#define RTC_SIZE   NUM_TERMINAL

#define RTC_WAIT    1
#define RTC_TICK    0

#define REGA        0x0A
#define REGB        0x0B
#define REGC        0x0C
#define REG_NMI     0x80
#define CMOS_PORT   0x71
#define RTC_PORT    0x70
#define IRQ2        0x02
#define IRQ8        0x08

#define RATE_MAX    6

#define RTC_R1      1  
#define RTC_R2      2
#define RTC_R4      4
#define RTC_R8      8
#define RTC_R16     16
#define RTC_R32     32
#define RTC_R64     64
#define RTC_R128    128
#define RTC_R256    256
#define RTC_R512    512

#define HZ_1024     1024
#define HZ_512      512
#define HZ_256      256
#define HZ_128      128
#define HZ_64       64
#define HZ_32       32
#define HZ_16       16
#define HZ_8        8
#define HZ_4        4
#define HZ_2        2


void init_rtc(void);
void rtc_handler(void);

int32_t rtc_read(int32_t fd, void* buf, int32_t nbytes);
int32_t rtc_write(int32_t fd, const void* buf, int32_t nbytes);
int32_t rtc_open(const uint8_t* filename);
int32_t rtc_close(int32_t fd);


// RTC client struct
typedef struct rtc_client {
    int client;
    volatile int32_t rate;
    volatile int32_t count;
    volatile uint8_t flags;
} rtcc_t;

#endif /* _RTC_HANDLER_H */
