#ifndef CLOCK_DRIVER_H
#define CLOCK_DRIVER_H

#include "rtc_driver.h"
#include "display_driver.h"
#include "interrupt_handler.h"
#include "keypad_driver.h"

typedef enum {
    CLOCK_MODE_NORMAL,          // 时钟模式
    CLOCK_MODE_SETTING,         // 设置模式
    CLOCK_MODE_STOPWATCH         // 秒表模式
} clock_mode_t;

int clock_driver_init(void);
int clock_start(void);
void clock_stop(void);
int clock_set_time(const rtc_time_t *time);
int clock_get_time(rtc_time_t *time);
void clock_stopwatch_start(void);
void clock_stopwatch_pause(void);
void clock_stopwatch_reset(void);
int clock_stopwatch_save_record(uint8_t record_id);
void clock_set_mode(clock_mode_t mode);
void clock_timer_callback(interrupt_type_t type, void *data);
void clock_keypad_callback(uint8_t key_code, key_event_t event);

#endif