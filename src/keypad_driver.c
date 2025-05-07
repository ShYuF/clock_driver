#include "keypad_driver.h"
#include "pc104_bus.h"

// 按键处理相关
static key_callback_t g_key_callback = NULL;  // 按键回调函数
static uint8_t g_last_key_state = 0;          // 上一次按键状态

/**
 * @brief 初始化按键驱动模块
 * 
 * @return 0表示成功，-1表示失败
 */
int keypad_init(void) {
    int timeout = PC104_TIMEOUT;
    uint8_t status;
    do {
        // 读取状态寄存器
        status = pc104_read_reg(KEYPAD_STATUS_REG);
        if (status == (uint8_t)-1) {
            printf("Failed to read keypad status register\n");
            return -1;
        }
        
        if (status == 0xFF) {
            printf("Keypad status register has initial value 0xFF, attempting reset\n");
            // 重置按键模块
            pc104_write_reg(KEYPAD_CONTROL_REG, 0);
            usleep(10000); // 等待10ms让设备有时间响应
            continue;
        }

        // 检查是否有错误状态
        if (status & KEYPAD_STATUS_ERROR) {
            printf("Keypad module in error state\n");
            // 清除错误状态 - 写入控制寄存器的复位命令
            pc104_write_reg(KEYPAD_CONTROL_REG, 0);
            usleep(10000); // 等待10ms让设备有时间响应
            continue;
        }
        
        // 启用按键模块
        if (pc104_write_reg(KEYPAD_CONTROL_REG, KEYPAD_CTRL_ENABLE) != 0) {
            printf("Failed to enable keypad module\n");
            return -1;
        }
        
        break;
    } while (--timeout > 0);
    
    // 初始化全局变量
    g_key_callback = NULL;
    g_last_key_state = 0;
    
    printf("Keypad driver initialized\n");
    return 0;
}

/**
 * @brief 注册按键回调函数
 * 
 * @param callback 按键事件回调函数
 */
void keypad_register_callback(key_callback_t callback) {
    g_key_callback = callback;
    printf("Keypad callback registered\n");
}

/**
 * @brief 轮询按键状态
 * 
 * 此函数应该被定期调用以检查按键事件
 */
void keypad_poll(void) {
    uint8_t status, key_data;
    uint8_t key_code, event_type;
    key_event_t event;
    
    // 读取按键状态寄存器
    status = pc104_read_reg(KEYPAD_STATUS_REG);
    
    // 检查是否有新按键事件
    if (status & KEYPAD_STATUS_NEW) {
        // 读取按键数据
        key_data = pc104_read_reg(KEYPAD_DATA_REG);
        
        // 提取按键编码和事件类型
        key_code = key_data & KEYPAD_CODE_MASK;
        event_type = key_data & KEYPAD_EVENT_MASK;
        
        // 确认已读取按键事件
        pc104_write_reg(KEYPAD_CONTROL_REG, KEYPAD_CTRL_ACK);
        
        // 将事件类型转换为枚举值
        switch (event_type) {
            case KEYPAD_EVENT_PRESS:
                event = KEY_PRESSED;
                break;
                
            case KEYPAD_EVENT_RELEASE:
                event = KEY_RELEASED;
                break;
                
            case KEYPAD_EVENT_LONG:
                event = KEY_LONG_PRESSED;
                break;
                
            default:
                // 未知事件类型，忽略
                return;
        }
        
        // 更新最后按键状态
        g_last_key_state = key_data;
        
        // 调用回调函数（如果已注册）
        if (g_key_callback != NULL) {
            g_key_callback(key_code, event);
        }
    }
}

/**
 * @brief 关闭按键驱动模块
 * 
 * @return 0表示成功
 */
int keypad_close(void) {
    // 禁用按键模块
    pc104_write_reg(KEYPAD_CONTROL_REG, KEYPAD_CTRL_DISABLE);
    
    // 清除回调函数
    g_key_callback = NULL;
    
    printf("Keypad driver closed\n");
    return 0;
}