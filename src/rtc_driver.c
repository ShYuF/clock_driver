#include "rtc_driver.h"
#include "pc104_bus.h"

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