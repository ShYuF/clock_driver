#ifndef RTC_DRIVER_H
#define RTC_DRIVER_H

#include "utils.h"

// RTC芯片寄存器地址（基于DS1302芯片或兼容芯片）
#define RTC_BASE_ADDR    0x500                // RTC基地址
#define RTC_SECOND_REG   (RTC_BASE_ADDR + 0)  // 秒寄存器
#define RTC_MINUTE_REG   (RTC_BASE_ADDR + 1)  // 分钟寄存器
#define RTC_HOUR_REG     (RTC_BASE_ADDR + 2)  // 小时寄存器
#define RTC_CONTROL_REG  (RTC_BASE_ADDR + 7)  // 控制寄存器

// RTC控制位
#define RTC_CTRL_HALT    0x80 // 停止RTC位
#define RTC_CTRL_WP      0x40 // 写保护位

typedef struct {
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
} rtc_time_t;

int rtc_init(void);
int rtc_get_time(rtc_time_t *time);
int rtc_set_time(const rtc_time_t *time);
int rtc_close(void);

#endif