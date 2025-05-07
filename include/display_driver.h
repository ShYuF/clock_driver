#ifndef DISPLAY_DRIVER_H
#define DISPLAY_DRIVER_H

#include "rtc_driver.h"

// 显示器基地址和寄存器定义
#define DISPLAY_BASE_ADDR       0x800       // 显示器基地址
#define DISPLAY_DATA_REG        (DISPLAY_BASE_ADDR + 0)  // 数据寄存器
#define DISPLAY_POS_REG         (DISPLAY_BASE_ADDR + 1)  // 位置寄存器（选择哪个数码管）
#define DISPLAY_CTRL_REG        (DISPLAY_BASE_ADDR + 2)  // 控制寄存器
#define DISPLAY_STATUS_REG      (DISPLAY_BASE_ADDR + 3)  // 状态寄存器

// 控制寄存器命令
#define DISPLAY_CTRL_CLEAR      0x01  // 清屏命令
#define DISPLAY_CTRL_BLINK      0x02  // 闪烁命令
#define DISPLAY_CTRL_POINT      0x04  // 控制小数点
#define DISPLAY_CTRL_MODE       0x08  // 设置显示模式

// 状态寄存器位定义
#define DISPLAY_STATUS_BUSY     0x01  // 忙状态标志
#define DISPLAY_STATUS_ERROR    0x80  // 错误状态标志

// 8段数码管定义
#define DISPLAY_DIGITS          4      // 数码管数量
#define DISPLAY_DIGIT_0         0      // 第一位（最右）
#define DISPLAY_DIGIT_1         1      // 第二位
#define DISPLAY_DIGIT_2         2      // 第三位
#define DISPLAY_DIGIT_3         3      // 第四位（最左）

// 8段数码管段位定义（共阴极）
#define SEGMENT_A               0x01   // A段（顶部）
#define SEGMENT_B               0x02   // B段（右上）
#define SEGMENT_C               0x04   // C段（右下）
#define SEGMENT_D               0x08   // D段（底部）
#define SEGMENT_E               0x10   // E段（左下）
#define SEGMENT_F               0x20   // F段（左上）
#define SEGMENT_G               0x40   // G段（中间）
#define SEGMENT_DP              0x80   // 小数点

// 显示模式定义
typedef enum {
    DISPLAY_MODE_CLOCK,     // 时钟显示模式
    DISPLAY_MODE_SETTING,   // 设置模式
    DISPLAY_MODE_STOPWATCH  // 秒表模式
} display_mode_t;

/**
 * @brief 初始化显示模块
 * 
 * @return 0表示成功，-1表示失败
 */
int display_init(void);

/**
 * @brief 更新显示的时间
 * 
 * @param time 要显示的时间
 */
void display_update_time(const rtc_time_t *time);

/**
 * @brief 更新秒表显示
 * 
 * @param milliseconds 要显示的毫秒数
 */
void display_update_stopwatch(uint32_t milliseconds);

/**
 * @brief 设置显示模式
 * 
 * @param mode 显示模式
 */
void display_set_mode(display_mode_t mode);

/**
 * @brief 设置数字在指定数码管位置显示
 * 
 * @param position 数码管位置(0-3)
 * @param digit 要显示的数字(0-9)
 * @param dp 是否显示小数点
 * @return 0表示成功，-1表示失败
 */
int display_set_digit(uint8_t position, uint8_t digit, uint8_t dp);

/**
 * @brief 设置编辑位置闪烁
 * 
 * @param position 闪烁的数码管位置
 */
void display_set_blink_position(uint8_t position);

#endif