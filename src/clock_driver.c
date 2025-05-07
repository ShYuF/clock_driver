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
static uint32_t g_last_timer_tick = 0;                  // 上次定时器触发时间

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
    rtc_time_t old_time;
    
    if (time == NULL) {
        printf("Failed to set time: NULL pointer\n");
        return -1;
    }
    
    // 保存旧的时间值以便进行比较
    memcpy(&old_time, &g_current_time, sizeof(rtc_time_t));
    
    // 设置RTC的时间
    int ret = rtc_set_time(time);
    if (ret != 0) {
        printf("Failed to set RTC time\n");
        return -1;
    }
    
    // 更新当前时间缓存
    memcpy(&g_current_time, time, sizeof(rtc_time_t));
    
    // 更新显示
    display_update_time(&g_current_time);
    
    // 输出详细日志，包括哪些时间单位被更改
    printf("RTC time set from %02d:%02d:%02d to %02d:%02d:%02d\n", 
           old_time.hour, old_time.minute, old_time.second,
           time->hour, time->minute, time->second);
    
    // 提示用户秒钟被重置的场景
    if (old_time.second != 0 && time->second == 0 && 
        (old_time.hour != time->hour || old_time.minute != time->minute)) {
        printf("Note: Seconds reset to 00 as part of time adjustment\n");
    }
    
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
    
    // 重置秒表计时器，如果从暂停状态继续则保留当前值
    if (g_stopwatch_ms == 0) {
        printf("Stopwatch started from 0.000 seconds\n");
    } else {
        printf("Stopwatch resumed from %u.%03u seconds\n", g_stopwatch_ms / 1000, g_stopwatch_ms % 1000);
    }
    
    // 重置计时器基准点，确保从当前时间开始计时
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    g_last_timer_tick = (ts.tv_sec * 1000) + (ts.tv_nsec / 1000000);
    
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
    printf("Stopwatch paused at %02u.%02u seconds (%u ms)\n", 
           g_stopwatch_ms / 1000, (g_stopwatch_ms % 1000) / 10, g_stopwatch_ms);
    
    // 更新显示确保最终值显示正确
    display_update_stopwatch(g_stopwatch_ms);
}

/**
 * @brief 复位秒表
 */
void clock_stopwatch_reset(void) {
    if (g_current_mode != CLOCK_MODE_STOPWATCH) {
        printf("Not in stopwatch mode\n");
        return;
    }
    
    printf("Stopwatch reset from %02u.%02u seconds to 00.00\n", 
           g_stopwatch_ms / 1000, (g_stopwatch_ms % 1000) / 10);
    
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
    
    // 输出当前秒表的值，格式为：秒.厘秒
    printf("Saving stopwatch record: %02u.%02u seconds (%u ms)\n", 
           g_stopwatch_ms / 1000, (g_stopwatch_ms % 1000) / 10, g_stopwatch_ms);
           
    int ret = storage_save_record(record_id, g_stopwatch_ms);
    if (ret != 0) {
        printf("Failed to save stopwatch record\n");
        return -1;
    }
    
    printf("Stopwatch record #%u saved: %02u.%02u seconds\n", 
           record_id, g_stopwatch_ms / 1000, (g_stopwatch_ms % 1000) / 10);
    return 0;
}

/**
 * @brief 设置工作模式
 * 
 * @param mode 要设置的模式
 */
void clock_set_mode(clock_mode_t mode) {
    // 特殊处理：从设置模式切换回正常模式时，再次获取RTC时间确保显示正确
    if (g_current_mode == CLOCK_MODE_SETTING && mode == CLOCK_MODE_NORMAL) {
        rtc_get_time(&g_current_time);
    }
    
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
    uint32_t current_tick;
    struct timespec ts;
    
    // 获取当前系统时间（以毫秒为单位）
    clock_gettime(CLOCK_MONOTONIC, &ts);
    current_tick = (ts.tv_sec * 1000) + (ts.tv_nsec / 1000000);
    
    // 首次调用初始化last_tick
    if (g_last_timer_tick == 0) {
        g_last_timer_tick = current_tick;
    }
    
    // 计算实际经过的时间（毫秒）
    uint32_t elapsed_ms = current_tick - g_last_timer_tick;
    g_last_timer_tick = current_tick;
    
    // 每10ms调用一次
    switch (g_current_mode) {
        case CLOCK_MODE_NORMAL:
            // 在正常模式下，每秒从RTC获取一次时间（100个10ms = 1秒）
            if (++tick_count >= 100) {
                tick_count = 0;
                rtc_get_time(&g_current_time);
                display_update_time(&g_current_time);
            }
            break;
            
        case CLOCK_MODE_SETTING:
            // 在设置模式下，只更新显示，不从RTC获取时间，防止覆盖用户设置
            if (++tick_count >= 100) {
                tick_count = 0;
                // 只刷新显示，使用当前内存中的时间
                display_update_time(&g_current_time);
                printf("Setting mode - using current memory time: %02d:%02d:%02d\n", 
                      g_current_time.hour, g_current_time.minute, g_current_time.second);
            }
            break;
            
        case CLOCK_MODE_STOPWATCH:
            // 秒表模式下的处理
            if (g_stopwatch_running) {
                // 使用实际经过的时间更新秒表，而不是假设固定10ms
                g_stopwatch_ms += elapsed_ms;
                
                // 每10ms更新一次显示，提高精度
                display_update_stopwatch(g_stopwatch_ms);
                
                // 每秒输出一次当前秒表值，便于调试
                if (++tick_count >= 100) {
                    tick_count = 0;
                    printf("Stopwatch running: %02u.%02u seconds (%u ms)\n", 
                           g_stopwatch_ms / 1000, (g_stopwatch_ms % 1000) / 10, g_stopwatch_ms);
                }
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

