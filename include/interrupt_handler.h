#ifndef INTERRUPT_HANDLER_H
#define INTERRUPT_HANDLER_H

#include "utils.h"

// 中断控制寄存器地址定义
#define INT_CTRL_BASE      0x400               // 中断控制器基地址
#define INT_CTRL_MASK      (INT_CTRL_BASE + 0) // 中断屏蔽寄存器
#define INT_CTRL_STATUS    (INT_CTRL_BASE + 1) // 中断状态寄存器
#define INT_CTRL_ACK       (INT_CTRL_BASE + 2) // 中断确认寄存器
#define INT_CTRL_CONFIG    (INT_CTRL_BASE + 3) // 中断配置寄存器

// 中断类型掩码
#define INT_MASK_TIMER     0x01     // 定时器中断掩码
#define INT_MASK_KEYPAD    0x02     // 按键中断掩码
#define INT_MASK_RTC_ALARM 0x04     // 闹钟中断掩码
#define INT_MASK_ALL       0x07     // 所有中断掩码

typedef enum {
    INT_TIMER,    // 定时器中断
    INT_KEYPAD,   // 按键中断
    INT_RTC_ALARM // 闹钟中断
} interrupt_type_t;

typedef void (*interrupt_callback_t)(interrupt_type_t type, void *data);
int interrupt_init(void);
int interrupt_register_handler(interrupt_type_t type, interrupt_callback_t callback);
void interrupt_enable(interrupt_type_t type);
void interrupt_disable(interrupt_type_t type);
int interrupt_close(void);

#endif