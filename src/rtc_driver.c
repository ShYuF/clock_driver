#include "rtc_driver.h"
#include "pc104_bus.h"

// 添加内存缓存，记录最后设置的RTC时间
static rtc_time_t g_rtc_cache = {0, 0, 0};
// 标记RTC是否已经被手动设置过
static int g_rtc_set_manually = 0;

/**
 * @brief 将BCD码转换为二进制
 * 
 * @param bcd BCD格式的数据
 * @return 转换后的二进制数据
 */
static uint8_t bcd_to_bin(uint8_t bcd) {
    return (bcd & 0x0F) + ((bcd >> 4) * 10);
}

/**
 * @brief 将二进制转换为BCD码
 * 
 * @param bin 二进制数据
 * @return 转换后的BCD格式数据
 */
static uint8_t bin_to_bcd(uint8_t bin) {
    return ((bin / 10) << 4) | (bin % 10);
}

/**
 * @brief 从硬件直接读取RTC时间，不使用缓存
 * 
 * @param time 存储获取的时间
 * @return 0表示成功，-1表示失败
 */
static int rtc_read_hw_time(rtc_time_t *time) {
    uint8_t second, minute, hour;
    
    if (time == NULL) {
        printf("Invalid time pointer\n");
        return -1;
    }
    
    // 读取秒、分、时寄存器
    second = pc104_read_reg(RTC_SECOND_REG);
    if (second == (uint8_t)-1) {
        printf("Failed to read RTC second register\n");
        return -1;
    }
    
    minute = pc104_read_reg(RTC_MINUTE_REG);
    if (minute == (uint8_t)-1) {
        printf("Failed to read RTC minute register\n");
        return -1;
    }
    
    hour = pc104_read_reg(RTC_HOUR_REG);
    if (hour == (uint8_t)-1) {
        printf("Failed to read RTC hour register\n");
        return -1;
    }
    
    // BCD转二进制
    time->second = bcd_to_bin(second & 0x7F);  // 去掉CH位
    time->minute = bcd_to_bin(minute & 0x7F);
    time->hour = bcd_to_bin(hour & 0x3F);  // 24小时制
    
    return 0;
}

/**
 * @brief 初始化RTC硬件
 * 
 * @return 0表示成功，-1表示失败
 */
int rtc_init(void) {
    uint8_t ctrl;
    
    // 读取控制寄存器
    ctrl = pc104_read_reg(RTC_CONTROL_REG);
    if (ctrl == (uint8_t)-1) {
        printf("Failed to read RTC control register\n");
        return -1;
    }
    
    // 清除HALT位，启动RTC时钟
    ctrl &= ~RTC_CTRL_HALT;
    
    // 清除写保护位，允许写入
    ctrl &= ~RTC_CTRL_WP;
    
    // 写入控制寄存器
    if (pc104_write_reg(RTC_CONTROL_REG, ctrl) != 0) {
        printf("Failed to write RTC control register\n");
        return -1;
    }
    
    // 初始化时，读取一次RTC时间更新缓存
    rtc_time_t init_time;
    if (rtc_read_hw_time(&init_time) == 0) {
        g_rtc_cache = init_time;
    }
    
    printf("RTC initialized successfully\n");
    return 0;
}

/**
 * @brief 获取RTC当前时间
 * 
 * @param time 存储获取的时间
 * @return 0表示成功，-1表示失败
 */
int rtc_get_time(rtc_time_t *time) {
    if (time == NULL) {
        printf("Invalid time pointer\n");
        return -1;
    }
    
    // 如果RTC已被手动设置，则使用内存缓存中的时间并更新秒数
    if (g_rtc_set_manually) {
        // 读取硬件时间以获取从上次设置后经过的秒数（仅在模拟环境中使用）
        rtc_time_t hw_time;
        static rtc_time_t last_hw_time = {0, 0, 0};
        static struct timespec last_read_time = {0, 0};
        struct timespec current_time;
        
        // 获取当前系统时间
        clock_gettime(CLOCK_MONOTONIC, &current_time);
        
        // 如果距离上次读取已经过去至少1秒，则重新读取硬件时间
        if (current_time.tv_sec - last_read_time.tv_sec >= 1) {
            if (rtc_read_hw_time(&hw_time) == 0) {
                // 计算自上次读取以来经过的秒数
                int elapsed_seconds = 0;
                
                if (last_hw_time.hour != 0 || last_hw_time.minute != 0 || last_hw_time.second != 0) {
                    elapsed_seconds = (hw_time.hour - last_hw_time.hour) * 3600 + 
                                     (hw_time.minute - last_hw_time.minute) * 60 + 
                                     (hw_time.second - last_hw_time.second);
                    
                    // 处理跨天情况
                    if (elapsed_seconds < 0) {
                        elapsed_seconds += 24 * 3600;
                    }
                    
                    // 限制最大增量为10秒，防止大幅跳变
                    if (elapsed_seconds > 10) {
                        elapsed_seconds = 1;
                    }
                } else {
                    elapsed_seconds = 1;
                }
                
                // 更新缓存时间
                g_rtc_cache.second += elapsed_seconds;
                
                // 处理进位
                if (g_rtc_cache.second >= 60) {
                    g_rtc_cache.minute += g_rtc_cache.second / 60;
                    g_rtc_cache.second %= 60;
                    
                    if (g_rtc_cache.minute >= 60) {
                        g_rtc_cache.hour += g_rtc_cache.minute / 60;
                        g_rtc_cache.minute %= 60;
                        
                        if (g_rtc_cache.hour >= 24) {
                            g_rtc_cache.hour %= 24;
                        }
                    }
                }
                
                // 记录本次硬件时间读取
                last_hw_time = hw_time;
                last_read_time = current_time;
            }
        }
        
        // 返回缓存的时间
        *time = g_rtc_cache;
        return 0;
    }
    else {
        // 如果未手动设置，则直接读取硬件时间
        return rtc_read_hw_time(time);
    }
}

/**
 * @brief 设置RTC时间
 * 
 * @param time 设置的时间
 * @return 0表示成功，-1表示失败
 */
int rtc_set_time(const rtc_time_t *time) {
    uint8_t ctrl;
    
    if (time == NULL) {
        printf("Invalid time pointer\n");
        return -1;
    }
    
    // 检查时间有效性
    if (time->second >= 60 || time->minute >= 60 || time->hour >= 24) {
        printf("Invalid time value\n");
        return -1;
    }
    
    // 暂停RTC时钟
    ctrl = pc104_read_reg(RTC_CONTROL_REG);
    ctrl |= RTC_CTRL_HALT;
    pc104_write_reg(RTC_CONTROL_REG, ctrl);
    
    // 写入时间寄存器（二进制转BCD）
    if (pc104_write_reg(RTC_SECOND_REG, bin_to_bcd(time->second)) != 0 ||
        pc104_write_reg(RTC_MINUTE_REG, bin_to_bcd(time->minute)) != 0 ||
        pc104_write_reg(RTC_HOUR_REG, bin_to_bcd(time->hour)) != 0) {
        
        printf("Failed to write RTC time registers\n");
        
        // 恢复RTC时钟
        ctrl &= ~RTC_CTRL_HALT;
        pc104_write_reg(RTC_CONTROL_REG, ctrl);
        
        return -1;
    }
    
    // 恢复RTC时钟
    ctrl &= ~RTC_CTRL_HALT;
    pc104_write_reg(RTC_CONTROL_REG, ctrl);
    
    // 设置时间标志为已手动设置
    g_rtc_set_manually = 1;
    
    // 更新内存缓存
    g_rtc_cache = *time;
    
    printf("RTC time set to %02d:%02d:%02d\n", time->hour, time->minute, time->second);
    return 0;
}

/**
 * @brief 关闭RTC驱动
 * 
 * @return 0表示成功
 */
int rtc_close(void) {
    printf("RTC driver closed\n");
    return 0;
}