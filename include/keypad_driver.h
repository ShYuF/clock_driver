#ifndef KEYPAD_DRIVER_H
#define KEYPAD_DRIVER_H

#include "utils.h"

// 按键寄存器地址定义
#define KEYPAD_BASE_ADDR     0x600                    // 按键模块基地址
#define KEYPAD_STATUS_REG    (KEYPAD_BASE_ADDR + 0)   // 按键状态寄存器
#define KEYPAD_DATA_REG      (KEYPAD_BASE_ADDR + 1)   // 按键数据寄存器
#define KEYPAD_CONTROL_REG   (KEYPAD_BASE_ADDR + 2)   // 按键控制寄存器

// 状态寄存器位定义
#define KEYPAD_STATUS_NEW    0x01  // 新按键事件标志
#define KEYPAD_STATUS_ERROR  0x80  // 错误标志

// 控制寄存器命令
#define KEYPAD_CTRL_ENABLE   0x01  // 使能按键模块
#define KEYPAD_CTRL_DISABLE  0x00  // 禁用按键模块
#define KEYPAD_CTRL_ACK      0x80  // 确认已读取按键事件

// 按键数据格式: [7:6]事件类型 [5:0]按键编号
#define KEYPAD_EVENT_MASK    0xC0  // 事件类型掩码
#define KEYPAD_EVENT_PRESS   0x40  // 按下事件
#define KEYPAD_EVENT_RELEASE 0x80  // 释放事件
#define KEYPAD_EVENT_LONG    0xC0  // 长按事件
#define KEYPAD_CODE_MASK     0x3F  // 按键编号掩码

typedef enum {
    KEY_PRESSED,
    KEY_RELEASED,
    KEY_LONG_PRESSED
} key_event_t;

typedef void (*key_callback_t)(uint8_t key_code, key_event_t event);
int keypad_init(void);
void keypad_register_callback(key_callback_t callback);
void keypad_poll(void);
int keypad_close(void);
#endif