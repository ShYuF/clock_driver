#include "clock_driver.h"
#include "pc104_bus.h"
#include "keypad_driver.h"
#include "interrupt_handler.h"
#include "storage_driver.h"

// 全局变量
static clock_mode_t g_current_mode = CLOCK_MODE_NORMAL;  // 当前工作模式
static uint32_t g_stopwatch_ms = 0;                      // 秒表计时（毫秒）
static int g_stopwatch_running = 0;                      // 秒表运行状态标志
static rtc_time_t g_current_time;                        // 当前时间缓存

/**
 * @brief 电子钟驱动初始化
 * 
 * @return 0表示成功，-1表示失败
 */
int clock_driver_init(void) {
    int ret;
    
    // 初始化PC104总线
    ret = pc104_init();
    if (ret != 0) {
        printf("Failed to initialize PC104 bus\n");
        return -1;
    }
    
    // 初始化RTC
    ret = rtc_init();
    if (ret != 0) {
        printf("Failed to initialize RTC\n");
        return -1;
    }
    
    // 初始化显示模块
    ret = display_init();
    if (ret != 0) {
        printf("Failed to initialize display\n");
        return -1;
    }
    
    // 初始化按键模块
    ret = keypad_init();
    if (ret != 0) {
        printf("Failed to initialize keypad\n");
        return -1;
    }
    
    // 初始化存储模块
    ret = storage_init();
    if (ret != 0) {
        printf("Failed to initialize storage\n");
        return -1;
    }
    
    // 初始化中断处理
    ret = interrupt_init();
    if (ret != 0) {
        printf("Failed to initialize interrupt handler\n");
        return -1;
    }
    
    // 注册按键回调函数
    keypad_register_callback(clock_keypad_callback);
    
    // 获取当前时间
    rtc_get_time(&g_current_time);
    
    // 显示当前时间
    display_update_time(&g_current_time);
    
    printf("Clock driver initialized successfully\n");
    return 0;
}

/**
 * @brief 启动电子钟
 * 
 * @return 0表示成功，-1表示失败
 */
int clock_start(void) {
    // 注册定时器中断
    int ret = interrupt_register_handler(INT_TIMER, clock_timer_callback);
    if (ret != 0) {
        printf("Failed to register timer interrupt handler\n");
        return -1;
    }
    
    // 启用定时器中断
    interrupt_enable(INT_TIMER);
    
    printf("Clock started\n");
    return 0;
}

/**
 * @brief 停止电子钟
 */
void clock_stop(void) {
    // 禁用定时器中断
    interrupt_disable(INT_TIMER);
    printf("Clock stopped\n");
}

/**
 * @brief 设置电子钟时间
 * 
 * @param time 要设置的时间
 * @return 0表示成功，-1表示失败
 */
int clock_set_time(const rtc_time_t *time) {
    int ret = rtc_set_time(time);
    if (ret != 0) {
        printf("Failed to set RTC time\n");
        return -1;
    }
    
    // 更新当前时间缓存
    memcpy(&g_current_time, time, sizeof(rtc_time_t));
    
    // 更新显示
    display_update_time(&g_current_time);
    
    printf("Clock time set to %02d:%02d:%02d\n", 
           g_current_time.hour, g_current_time.minute, g_current_time.second);
    return 0;
}

/**
 * @brief 获取电子钟时间
 * 
 * @param time 存储获取的时间
 * @return 0表示成功，-1表示失败
 */
int clock_get_time(rtc_time_t *time) {
    int ret = rtc_get_time(time);
    if (ret != 0) {
        printf("Failed to get RTC time\n");
        return -1;
    }
    
    // 更新当前时间缓存
    memcpy(&g_current_time, time, sizeof(rtc_time_t));
    
    return 0;
}

/**
 * @brief 启动秒表
 */
void clock_stopwatch_start(void) {
    if (g_current_mode != CLOCK_MODE_STOPWATCH) {
        printf("Not in stopwatch mode\n");
        return;
    }
    
    g_stopwatch_running = 1;
    printf("Stopwatch started\n");
}

/**
 * @brief 暂停秒表
 */
void clock_stopwatch_pause(void) {
    if (g_current_mode != CLOCK_MODE_STOPWATCH) {
        printf("Not in stopwatch mode\n");
        return;
    }
    
    g_stopwatch_running = 0;
    printf("Stopwatch paused at %u.%03u seconds\n", 
           g_stopwatch_ms / 1000, g_stopwatch_ms % 1000);
}

/**
 * @brief 复位秒表
 */
void clock_stopwatch_reset(void) {
    if (g_current_mode != CLOCK_MODE_STOPWATCH) {
        printf("Not in stopwatch mode\n");
        return;
    }
    
    g_stopwatch_running = 0;
    g_stopwatch_ms = 0;
    display_update_stopwatch(g_stopwatch_ms);
    printf("Stopwatch reset\n");
}

/**
 * @brief 保存秒表记录
 * 
 * @param record_id 记录ID
 * @return 0表示成功，-1表示失败
 */
int clock_stopwatch_save_record(uint8_t record_id) {
    if (g_current_mode != CLOCK_MODE_STOPWATCH) {
        printf("Not in stopwatch mode\n");
        return -1;
    }
    
    int ret = storage_save_record(record_id, g_stopwatch_ms);
    if (ret != 0) {
        printf("Failed to save stopwatch record\n");
        return -1;
    }
    
    printf("Stopwatch record #%u saved: %u.%03u seconds\n", 
           record_id, g_stopwatch_ms / 1000, g_stopwatch_ms % 1000);
    return 0;
}

/**
 * @brief 设置工作模式
 * 
 * @param mode 要设置的模式
 */
void clock_set_mode(clock_mode_t mode) {
    g_current_mode = mode;
    
    // 更新显示模式
    switch (mode) {
        case CLOCK_MODE_NORMAL:
            display_set_mode(DISPLAY_MODE_CLOCK);
            display_update_time(&g_current_time);
            printf("Switched to normal clock mode\n");
            break;
            
        case CLOCK_MODE_SETTING:
            display_set_mode(DISPLAY_MODE_SETTING);
            printf("Switched to time setting mode\n");
            break;
            
        case CLOCK_MODE_STOPWATCH:
            display_set_mode(DISPLAY_MODE_STOPWATCH);
            display_update_stopwatch(g_stopwatch_ms);
            printf("Switched to stopwatch mode\n");
            break;
            
        default:
            printf("Invalid mode\n");
            break;
    }
}

/**
 * @brief 定时器中断回调函数
 * 
 * @param type 中断类型
 * @param data 中断相关数据
 */
void clock_timer_callback(interrupt_type_t type, void *data) {
    static uint8_t tick_count = 0;
    
    // 每10ms调用一次
    switch (g_current_mode) {
        case CLOCK_MODE_NORMAL:
        case CLOCK_MODE_SETTING:
            // 每秒更新一次显示（100个10ms = 1秒）
            if (++tick_count >= 100) {
                tick_count = 0;
                rtc_get_time(&g_current_time);
                display_update_time(&g_current_time);
            }
            break;
            
        case CLOCK_MODE_STOPWATCH:
            // 秒表模式下的处理
            if (g_stopwatch_running) {
                // 更新秒表计时（每10ms增加10ms）
                g_stopwatch_ms += 10;
                
                // 每100ms更新一次显示（减少显示刷新频率）
                if (tick_count % 10 == 0) {
                    display_update_stopwatch(g_stopwatch_ms);
                }
            }
            
            // 不论秒表是否运行，都需要增加tick_count并在到达100时重置
            if (++tick_count >= 100) {
                tick_count = 0;
            }
            break;
            
        default:
            break;
    }
}

/**
 * @brief 按键回调函数
 * 
 * @param key_code 按键代码
 * @param event 按键事件类型
 */
void clock_keypad_callback(uint8_t key_code, key_event_t event) {
    // 仅处理按键按下事件
    if (event != KEY_PRESSED) {
        return;
    }
    
    printf("Keypad button %d pressed in mode %d\n", key_code, g_current_mode);
    
    // 确保按键编码在有效范围内（1-3）
    if (key_code < 1 || key_code > 3) {
        printf("Invalid key code: %d\n", key_code);
        return;
    }
    
    // 根据当前模式处理按键
    switch (g_current_mode) {
        case CLOCK_MODE_NORMAL:
            // 处理普通模式下的按键
            switch (key_code) {
                case 1: // 1号键为模式切换键
                    clock_set_mode(CLOCK_MODE_SETTING);
                    break;
                    
                case 2: // 2号键为秒表模式切换键
                    clock_set_mode(CLOCK_MODE_STOPWATCH);
                    break;
                    
                case 3: // 3号键在普通模式下无特殊功能
                    // 可以添加额外功能，如背光控制等
                    printf("Key 3 pressed in normal mode - no action defined\n");
                    break;
            }
            break;
            
        case CLOCK_MODE_SETTING:
            // 处理设置模式下的按键
            switch (key_code) {
                case 1: // 1号键为确认键，返回普通模式
                    clock_set_mode(CLOCK_MODE_NORMAL);
                    break;
                    
                case 2: // 2号键为小时增加键
                    g_current_time.hour = (g_current_time.hour + 1) % 24;
                    clock_set_time(&g_current_time);
                    break;
                    
                case 3: // 3号键为分钟增加键
                    g_current_time.minute = (g_current_time.minute + 1) % 60;
                    g_current_time.second = 0; // 重置秒钟，更符合用户设置时间的预期
                    clock_set_time(&g_current_time);
                    break;
            }
            break;
            
        case CLOCK_MODE_STOPWATCH:
            // 处理秒表模式下的按键
            switch (key_code) {
                case 1: // 1号键为返回普通模式
                    clock_set_mode(CLOCK_MODE_NORMAL);
                    break;
                    
                case 2: // 2号键为启动/暂停键
                    if (g_stopwatch_running) {
                        clock_stopwatch_pause();
                    } else {
                        clock_stopwatch_start();
                    }
                    break;
                    
                case 3: // 3号键为复位/保存记录键
                    if (g_stopwatch_running) {
                        // 运行中保存记录
                        clock_stopwatch_save_record(0);
                    } else {
                        // 停止时复位秒表
                        clock_stopwatch_reset();
                    }
                    break;
            }
            break;
            
        default:
            break;
    }
}

