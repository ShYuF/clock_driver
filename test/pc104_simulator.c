#include "pc104_simulator.h"
#include "pc104_bus.h"
#include "rtc_driver.h"
#include "keypad_driver.h"
#include "interrupt_handler.h"
#include "storage_driver.h"
#include "display_driver.h"

#include <time.h>
#include <pthread.h>

// 模拟的PC104内存空间
static uint8_t g_pc104_memory[PC104_SIM_MEM_SIZE];

// 互斥量，保护对模拟内存的访问
static pthread_mutex_t g_pc104_mutex = PTHREAD_MUTEX_INITIALIZER;

// 模拟器状态
static int g_simulator_initialized = 0;

// 模拟RTC时钟
static time_t g_rtc_base_time = 0;
static uint8_t g_rtc_registers[8]; // RTC寄存器状态

// 设备模拟状态
typedef struct {
    int device_id;
    int behavior;
    uint32_t param;
} device_behavior_t;

static device_behavior_t g_device_behavior[5]; // 0:PC104, 1:RTC, 2:Display, 3:Keypad, 4:Storage

/**
 * @brief 初始化PC104总线模拟器
 * 
 * @return 0表示成功，-1表示失败
 */
int pc104_sim_init(void) {
    // 初始化互斥量
    pthread_mutex_init(&g_pc104_mutex, NULL);
    
    // 初始化模拟内存
    memset(g_pc104_memory, 0xFF, sizeof(g_pc104_memory));
    memset(g_device_behavior, 0, sizeof(g_device_behavior));
    
    // 设置各设备状态寄存器为正常状态（非忙，无错误）
    g_pc104_memory[PC104_STATUS_PORT - PC104_BASE_ADDR] = 0x00;
    g_pc104_memory[DISPLAY_STATUS_REG - PC104_BASE_ADDR] = 0x00;
    g_pc104_memory[KEYPAD_STATUS_REG - PC104_BASE_ADDR] = 0x00;
    g_pc104_memory[STORAGE_STATUS_REG - PC104_BASE_ADDR] = 0x00;
    g_pc104_memory[INT_CTRL_STATUS - PC104_BASE_ADDR] = 0x00;
    
    // 初始化RTC模拟
    time(&g_rtc_base_time);
    
    // 初始化RTC寄存器
    struct tm* lt = localtime(&g_rtc_base_time);
    g_rtc_registers[0] = ((lt->tm_sec / 10) << 4) | (lt->tm_sec % 10);  // 秒，BCD格式
    g_rtc_registers[1] = ((lt->tm_min / 10) << 4) | (lt->tm_min % 10);  // 分，BCD格式
    g_rtc_registers[2] = ((lt->tm_hour / 10) << 4) | (lt->tm_hour % 10); // 时，BCD格式
    
    g_simulator_initialized = 1;
    printf("[SIM] PC104 simulator initialized\n");
    return 0;
}

/**
 * @brief 检查PC104总线模拟器是否初始化
 * 
 * @return 1表示已初始化，0表示未初始化
 */
int pc104_sim_is_initialized(void) {
    return g_simulator_initialized;
}

/**
 * @brief 从模拟的PC104端口读取一个字节
 * 
 * @param port 端口地址
 * @return 读取到的值
 */
uint8_t pc104_sim_read_port(uint16_t port) {
    uint8_t value;
    uint16_t offset;
    
    if (!g_simulator_initialized) {
        printf("[SIM] Error: PC104 simulator not initialized\n");
        return 0xFF;
    }
    
    // 计算端口偏移量
    offset = port - PC104_BASE_ADDR;
    
    // 检查端口地址是否超出范围
    if (offset >= PC104_SIM_MEM_SIZE) {
        printf("[SIM] Warning: Reading from out-of-range port: 0x%04X\n", port);
        return 0xFF;
    }
    
    // 读取模拟端口值
    pthread_mutex_lock(&g_pc104_mutex);
    
    // 处理特殊端口
    if (port >= RTC_BASE_ADDR && port < RTC_BASE_ADDR + 8) {
        // 读取RTC寄存器
        int reg_index = port - RTC_BASE_ADDR;
        if (reg_index == 0 || reg_index == 1 || reg_index == 2) {
            // 更新时间寄存器
            time_t now;
            time(&now);
            struct tm* lt = localtime(&now);
            
            g_rtc_registers[0] = ((lt->tm_sec / 10) << 4) | (lt->tm_sec % 10);  // 秒，BCD格式
            g_rtc_registers[1] = ((lt->tm_min / 10) << 4) | (lt->tm_min % 10);  // 分，BCD格式
            g_rtc_registers[2] = ((lt->tm_hour / 10) << 4) | (lt->tm_hour % 10); // 时，BCD格式
        }
        
        value = g_rtc_registers[reg_index];
        pthread_mutex_unlock(&g_pc104_mutex);
        return value;
    }
    
    // 从模拟内存中读取值
    value = g_pc104_memory[offset];
    
    pthread_mutex_unlock(&g_pc104_mutex);
    
    // 特殊口处理
    if (port == PC104_STATUS_PORT) {
        // 模拟随机繁忙状态，增加真实性
        if (g_device_behavior[0].behavior == 1 && (rand() % 10) == 0) {
            value |= PC104_STATUS_BUSY;
        }
    } else if (port == KEYPAD_STATUS_REG) {
        // 检查是否需要模拟按键输入
        if (g_device_behavior[3].behavior == 1) {
            // 设置新按键事件标志
            value |= KEYPAD_STATUS_NEW;
        }
    } else if (port == KEYPAD_DATA_REG) {
        // 如果有按键事件，返回按键数据
        if (g_device_behavior[3].behavior == 1) {
            value = (KEYPAD_EVENT_PRESS | (g_device_behavior[3].param & KEYPAD_CODE_MASK));
            // 处理完后清除按键事件
            g_device_behavior[3].behavior = 0;
        }
    }
    
    return value;
}

/**
 * @brief 向模拟的PC104端口写入一个字节
 * 
 * @param value 要写入的值
 * @param port 端口地址
 */
void pc104_sim_write_port(uint8_t value, uint16_t port) {
    uint16_t offset;
    
    if (!g_simulator_initialized) {
        printf("[SIM] Error: PC104 simulator not initialized\n");
        return;
    }
    
    // 计算端口偏移量
    offset = port - PC104_BASE_ADDR;
    
    // 检查端口地址是否超出范围
    if (offset >= PC104_SIM_MEM_SIZE) {
        printf("[SIM] Warning: Writing to out-of-range port: 0x%04X\n", port);
        return;
    }
    
    // 写入模拟端口值
    pthread_mutex_lock(&g_pc104_mutex);
    
    // 处理特殊端口
    if (port >= RTC_BASE_ADDR && port < RTC_BASE_ADDR + 8) {
        // 写入RTC寄存器
        int reg_index = port - RTC_BASE_ADDR;
        g_rtc_registers[reg_index] = value;
        
        // 如果修改了时间寄存器，更新RTC基础时间
        if (reg_index == 0 || reg_index == 1 || reg_index == 2) {
            struct tm newtime;
            time(&g_rtc_base_time);
            newtime = *localtime(&g_rtc_base_time);
            
            // 解析BCD格式
            if (reg_index == 0) {
                newtime.tm_sec = ((value >> 4) & 0x0F) * 10 + (value & 0x0F);
            } else if (reg_index == 1) {
                newtime.tm_min = ((value >> 4) & 0x0F) * 10 + (value & 0x0F);
            } else if (reg_index == 2) {
                newtime.tm_hour = ((value >> 4) & 0x0F) * 10 + (value & 0x0F);
            }
            
            g_rtc_base_time = mktime(&newtime);
        }
        
        pthread_mutex_unlock(&g_pc104_mutex);
        return;
    }
    
    // 将值写入模拟内存
    g_pc104_memory[offset] = value;
    
    // 特殊口处理
    if (port == PC104_CMD_PORT) {
        // 命令处理，如果是读写命令，自动清除忙状态
        if (value == PC104_CMD_READ || value == PC104_CMD_WRITE) {
            g_pc104_memory[PC104_STATUS_PORT - PC104_BASE_ADDR] &= ~PC104_STATUS_BUSY;
        }
    } else if (port == KEYPAD_CONTROL_REG) {
        // 按键控制寄存器，如果是确认命令，清除新按键事件标志
        if (value == KEYPAD_CTRL_ACK) {
            g_pc104_memory[KEYPAD_STATUS_REG - PC104_BASE_ADDR] &= ~KEYPAD_STATUS_NEW;
        }
    }
    
    pthread_mutex_unlock(&g_pc104_mutex);
}

/**
 * @brief 模拟延迟，使硬件模拟更真实
 * 
 * @param microseconds 微秒数
 */
void pc104_sim_delay(uint32_t microseconds) {
    usleep(microseconds);
}

/**
 * @brief 关闭PC104总线模拟器
 */
void pc104_sim_close(void) {
    if (!g_simulator_initialized) {
        return;
    }
    
    pthread_mutex_destroy(&g_pc104_mutex);
    g_simulator_initialized = 0;
    
    printf("[SIM] PC104 simulator closed\n");
}

/**
 * @brief 设置特定设备的模拟行为
 * 
 * @param device_id 设备ID (0:PC104, 1:RTC, 2:Display, 3:Keypad, 4:Storage)
 * @param behavior 行为ID
 * @param param 行为参数
 */
void pc104_sim_set_behavior(int device_id, int behavior, uint32_t param) {
    if (device_id < 0 || device_id >= 5) {
        printf("[SIM] Invalid device ID: %d\n", device_id);
        return;
    }
    
    pthread_mutex_lock(&g_pc104_mutex);
    g_device_behavior[device_id].behavior = behavior;
    g_device_behavior[device_id].param = param;
    pthread_mutex_unlock(&g_pc104_mutex);
    
    printf("[SIM] Set device %d behavior to %d with param %u\n", 
           device_id, behavior, param);
}