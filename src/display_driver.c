#include "display_driver.h"
#include "pc104_bus.h"

// 当前显示模式
static display_mode_t g_current_display_mode = DISPLAY_MODE_CLOCK;

// 编辑模式下的闪烁位置
static uint8_t g_blink_position = 0xFF;

// 段码表：0-9的数字对应的段码
static const uint8_t g_segment_patterns[] = {
    SEGMENT_A | SEGMENT_B | SEGMENT_C | SEGMENT_D | SEGMENT_E | SEGMENT_F,          // 0
    SEGMENT_B | SEGMENT_C,                                                          // 1
    SEGMENT_A | SEGMENT_B | SEGMENT_G | SEGMENT_E | SEGMENT_D,                      // 2
    SEGMENT_A | SEGMENT_B | SEGMENT_G | SEGMENT_C | SEGMENT_D,                      // 3
    SEGMENT_F | SEGMENT_G | SEGMENT_B | SEGMENT_C,                                  // 4
    SEGMENT_A | SEGMENT_F | SEGMENT_G | SEGMENT_C | SEGMENT_D,                      // 5
    SEGMENT_A | SEGMENT_F | SEGMENT_E | SEGMENT_D | SEGMENT_C | SEGMENT_G,          // 6
    SEGMENT_A | SEGMENT_B | SEGMENT_C,                                              // 7
    SEGMENT_A | SEGMENT_B | SEGMENT_C | SEGMENT_D | SEGMENT_E | SEGMENT_F | SEGMENT_G, // 8
    SEGMENT_A | SEGMENT_B | SEGMENT_C | SEGMENT_D | SEGMENT_F | SEGMENT_G           // 9
};

// 当前数码管显示内容
static uint8_t g_current_display[DISPLAY_DIGITS] = {0};
static uint8_t g_dp_status[DISPLAY_DIGITS] = {0}; // 小数点状态

/**
 * @brief 等待显示器就绪
 * 
 * @return 0表示成功，-1表示超时或错误
 */
static int display_wait_ready(void) {
    int timeout = PC104_TIMEOUT;
    uint8_t status;
    
    do {
        status = pc104_read_reg(DISPLAY_STATUS_REG);
        
        // 0xFF可能是未初始化状态，尝试重置
        if (status == 0xFF) {
            // 发送清屏命令尝试重置显示器
            pc104_write_reg(DISPLAY_CTRL_REG, DISPLAY_CTRL_CLEAR);
            usleep(10000);  // 等待10ms让设备有时间响应
            continue;
        }
        
        // 检查错误状态
        if (status & DISPLAY_STATUS_ERROR) {
            printf("Display error detected: status=0x%02X\n", status);
            return -1;
        }
        
        // 检查是否就绪
        if (!(status & DISPLAY_STATUS_BUSY)) {
            return 0;  // 显示器就绪
        }
        
        usleep(1);  // 延时1微秒
    } while (--timeout > 0);
    
    printf("Display timeout waiting for ready\n");
    return -1;  // 超时
}

/**
 * @brief 清除所有数码管显示
 * 
 * @return 0表示成功，-1表示失败
 */
static int display_clear(void) {
    // 等待显示器就绪
    if (display_wait_ready() != 0) {
        return -1;
    }
    
    // 写入清屏命令
    if (pc104_write_reg(DISPLAY_CTRL_REG, DISPLAY_CTRL_CLEAR) != 0) {
        printf("Failed to clear display\n");
        return -1;
    }
    
    // 清除当前显示内容缓存
    for (int i = 0; i < DISPLAY_DIGITS; i++) {
        g_current_display[i] = 0;
        g_dp_status[i] = 0;
    }
    
    return 0;
}

/**
 * @brief 设置数字在指定数码管位置显示
 * 
 * @param position 数码管位置(0-3)
 * @param digit 要显示的数字(0-9)
 * @param dp 是否显示小数点
 * @return 0表示成功，-1表示失败
 */
int display_set_digit(uint8_t position, uint8_t digit, uint8_t dp) {
    uint8_t segment_code;
    
    // 检查参数有效性
    if (position >= DISPLAY_DIGITS) {
        printf("Invalid display position: %d\n", position);
        return -1;
    }
    
    if (digit > 9) {
        printf("Invalid digit value: %d\n", digit);
        return -1;
    }
    
    // 获取段码
    segment_code = g_segment_patterns[digit];
    
    // 添加小数点(如果需要)
    if (dp) {
        segment_code |= SEGMENT_DP;
        g_dp_status[position] = 1;
    } else {
        g_dp_status[position] = 0;
    }
    
    // 等待显示器就绪
    if (display_wait_ready() != 0) {
        return -1;
    }
    
    // 设置位置寄存器（选择数码管）
    if (pc104_write_reg(DISPLAY_POS_REG, position) != 0) {
        printf("Failed to set display position\n");
        return -1;
    }
    
    // 写入数据寄存器（段码）
    if (pc104_write_reg(DISPLAY_DATA_REG, segment_code) != 0) {
        printf("Failed to write segment data\n");
        return -1;
    }
    
    // 保存当前显示内容
    g_current_display[position] = digit;
    
    return 0;
}

/**
 * @brief 设置编辑位置闪烁
 * 
 * @param position 闪烁的数码管位置，0xFF表示不闪烁
 */
void display_set_blink_position(uint8_t position) {
    uint8_t ctrl_val = 0;
    
    g_blink_position = position;
    
    // 如果位置有效，设置闪烁控制位
    if (position < DISPLAY_DIGITS) {
        // 等待显示器就绪
        if (display_wait_ready() != 0) {
            return;
        }
        
        // 设置位置寄存器（选择数码管）
        if (pc104_write_reg(DISPLAY_POS_REG, position) != 0) {
            printf("Failed to set blink position\n");
            return;
        }
        
        // 设置闪烁控制位
        ctrl_val = DISPLAY_CTRL_BLINK;
        
        // 写入控制寄存器
        if (pc104_write_reg(DISPLAY_CTRL_REG, ctrl_val) != 0) {
            printf("Failed to set blink mode\n");
            return;
        }
    } else {
        // 取消所有闪烁
        for (int i = 0; i < DISPLAY_DIGITS; i++) {
            // 设置位置寄存器（选择数码管）
            if (display_wait_ready() != 0) continue;
            if (pc104_write_reg(DISPLAY_POS_REG, i) != 0) continue;
            if (pc104_write_reg(DISPLAY_CTRL_REG, 0) != 0) continue;
        }
    }
}

/**
 * @brief 初始化显示模块
 * 
 * @return 0表示成功，-1表示失败
 */
int display_init(void) {
    // 清除显示内容
    if (display_clear() != 0) {
        return -1;
    }
    
    // 设置初始模式（时钟模式）
    g_current_display_mode = DISPLAY_MODE_CLOCK;
    g_blink_position = 0xFF; // 不闪烁
    
    // 显示数字8的测试模式
    for (int i = 0; i < DISPLAY_DIGITS; i++) {
        if (display_set_digit(i, 8, 0) != 0) {
            printf("Failed to set test pattern\n");
            return -1;
        }
    }
    
    // 延时1秒
    sleep(1);
    
    // 清屏
    if (display_clear() != 0) {
        return -1;
    }
    
    printf("Display driver initialized successfully\n");
    return 0;
}

/**
 * @brief 更新显示的时间
 * 
 * @param time 要显示的时间
 */
void display_update_time(const rtc_time_t *time) {
    if (time == NULL) {
        printf("Invalid time pointer\n");
        return;
    }
    
    // 根据当前模式决定显示内容
    switch (g_current_display_mode) {
        case DISPLAY_MODE_CLOCK:
        case DISPLAY_MODE_SETTING:
            // 显示小时的十位和个位
            display_set_digit(DISPLAY_DIGIT_3, time->hour / 10, 0);
            display_set_digit(DISPLAY_DIGIT_2, time->hour % 10, 1); // 带小数点分隔
            
            // 显示分钟的十位和个位
            display_set_digit(DISPLAY_DIGIT_1, time->minute / 10, 0);
            display_set_digit(DISPLAY_DIGIT_0, time->minute % 10, 0);
            
            // 在设置模式下，设置闪烁位置
            if (g_current_display_mode == DISPLAY_MODE_SETTING) {
                // 根据当前编辑的是小时还是分钟设置闪烁
                if (g_blink_position == 0) { // 编辑小时
                    display_set_blink_position(DISPLAY_DIGIT_2);
                    display_set_blink_position(DISPLAY_DIGIT_3);
                } else { // 编辑分钟
                    display_set_blink_position(DISPLAY_DIGIT_0);
                    display_set_blink_position(DISPLAY_DIGIT_1);
                }
            }
            break;
            
        default:
            // 其他模式下不特别处理
            break;
    }
}

/**
 * @brief 更新秒表显示
 * 
 * @param milliseconds 要显示的毫秒数
 */
void display_update_stopwatch(uint32_t milliseconds) {
    uint8_t minutes, seconds;
    
    if (g_current_display_mode != DISPLAY_MODE_STOPWATCH) {
        return;
    }
    
    // 计算分钟和秒
    seconds = (milliseconds / 1000) % 60;
    minutes = (milliseconds / 60000) % 100; // 限制在0-99分钟
    
    // 显示分钟的十位和个位
    display_set_digit(DISPLAY_DIGIT_3, minutes / 10, 0);
    display_set_digit(DISPLAY_DIGIT_2, minutes % 10, 1); // 带小数点分隔
    
    // 显示秒钟的十位和个位
    display_set_digit(DISPLAY_DIGIT_1, seconds / 10, 0);
    display_set_digit(DISPLAY_DIGIT_0, seconds % 10, 0);
}

/**
 * @brief 设置显示模式
 * 
 * @param mode 显示模式
 */
void display_set_mode(display_mode_t mode) {
    // 保存当前模式
    g_current_display_mode = mode;
    
    // 清除显示并取消所有闪烁
    display_clear();
    display_set_blink_position(0xFF);
    
    // 根据模式设置显示参数
    switch (mode) {
        case DISPLAY_MODE_CLOCK:
            // 常规时钟模式，不需要特别设置
            break;
            
        case DISPLAY_MODE_SETTING:
            // 默认编辑小时
            g_blink_position = 0;
            break;
            
        case DISPLAY_MODE_STOPWATCH:
            // 秒表初始显示 00:00
            for (int i = 0; i < DISPLAY_DIGITS; i++) {
                display_set_digit(i, 0, (i == 2) ? 1 : 0); // 位置2添加小数点
            }
            break;
            
        default:
            printf("Invalid display mode\n");
            break;
    }
}

/**
 * @brief 关闭显示模块
 * 
 * @return 0表示成功
 */
int display_close(void) {
    // 清除显示内容
    display_clear();
    
    printf("Display driver closed\n");
    return 0;
}