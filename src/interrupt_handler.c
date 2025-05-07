#include "interrupt_handler.h"
#include "pc104_bus.h"

// 中断回调函数数组
static interrupt_callback_t g_int_handlers[3] = {NULL, NULL, NULL};

// 中断服务线程相关变量
static int g_int_thread_running = 0;
static pthread_t g_int_thread;
static pthread_mutex_t g_int_mutex;

/**
 * @brief 获取中断类型对应的掩码
 * 
 * @param type 中断类型
 * @return 对应的中断掩码，如果类型无效则返回0
 */
static uint8_t get_interrupt_mask(interrupt_type_t type) {
    switch (type) {
        case INT_TIMER:
            return INT_MASK_TIMER;
        case INT_KEYPAD:
            return INT_MASK_KEYPAD;
        case INT_RTC_ALARM:
            return INT_MASK_RTC_ALARM;
        default:
            return 0;
    }
}

/**
 * @brief 中断服务线程函数
 * 
 * @param arg 线程参数（未使用）
 * @return void* 线程返回值（未使用）
 */
static void* interrupt_service_thread(void* arg) {
    uint8_t int_status;
    
    printf("Interrupt service thread started\n");
    
    while (g_int_thread_running) {
        // 获取中断状态
        int_status = pc104_read_reg(INT_CTRL_STATUS);
        
        if (int_status) {
            // 处理每个中断
            if (int_status & INT_MASK_TIMER) {
                pthread_mutex_lock(&g_int_mutex);
                if (g_int_handlers[INT_TIMER]) {
                    g_int_handlers[INT_TIMER](INT_TIMER, NULL);
                }
                pthread_mutex_unlock(&g_int_mutex);
                
                // 确认中断
                pc104_write_reg(INT_CTRL_ACK, INT_MASK_TIMER);
            }
            
            if (int_status & INT_MASK_KEYPAD) {
                pthread_mutex_lock(&g_int_mutex);
                if (g_int_handlers[INT_KEYPAD]) {
                    g_int_handlers[INT_KEYPAD](INT_KEYPAD, NULL);
                }
                pthread_mutex_unlock(&g_int_mutex);
                
                // 确认中断
                pc104_write_reg(INT_CTRL_ACK, INT_MASK_KEYPAD);
            }
            
            if (int_status & INT_MASK_RTC_ALARM) {
                pthread_mutex_lock(&g_int_mutex);
                if (g_int_handlers[INT_RTC_ALARM]) {
                    g_int_handlers[INT_RTC_ALARM](INT_RTC_ALARM, NULL);
                }
                pthread_mutex_unlock(&g_int_mutex);
                
                // 确认中断
                pc104_write_reg(INT_CTRL_ACK, INT_MASK_RTC_ALARM);
            }
        }
        
        // 休眠一小段时间，避免过度占用CPU
        usleep(1000); // 休眠1ms
    }
    
    printf("Interrupt service thread stopped\n");
    return NULL;
}

/**
 * @brief 初始化中断处理模块
 * 
 * @return 0表示成功，-1表示失败
 */
int interrupt_init(void) {
    int ret;
    
    // 初始化互斥量
    pthread_mutex_init(&g_int_mutex, NULL);
    
    // 初始化中断控制器
    pc104_write_reg(INT_CTRL_MASK, 0); // 屏蔽所有中断
    pc104_write_reg(INT_CTRL_ACK, INT_MASK_ALL); // 确认所有中断
    pc104_write_reg(INT_CTRL_CONFIG, 0x01); // 配置中断控制器（启用）
    
    // 启动中断服务线程
    g_int_thread_running = 1;
    ret = pthread_create(&g_int_thread, NULL, interrupt_service_thread, NULL);
    if (ret != 0) {
        printf("Failed to create interrupt service thread\n");
        return -1;
    }
    
    printf("Interrupt handler initialized\n");
    return 0;
}

/**
 * @brief 注册中断处理函数
 * 
 * @param type 中断类型
 * @param callback 中断处理回调函数
 * @return 0表示成功，-1表示失败
 */
int interrupt_register_handler(interrupt_type_t type, interrupt_callback_t callback) {
    if (type < 0 || type >= 3) {
        printf("Invalid interrupt type\n");
        return -1;
    }
    
    if (callback == NULL) {
        printf("NULL interrupt handler\n");
        return -1;
    }
    
    // 注册回调函数
    pthread_mutex_lock(&g_int_mutex);
    g_int_handlers[type] = callback;
    pthread_mutex_unlock(&g_int_mutex);
    
    printf("Interrupt handler registered for type %d\n", type);
    return 0;
}

/**
 * @brief 启用指定类型的中断
 * 
 * @param type 中断类型
 */
void interrupt_enable(interrupt_type_t type) {
    uint8_t mask = get_interrupt_mask(type);
    if (mask == 0) {
        printf("Invalid interrupt type to enable\n");
        return;
    }
    
    uint8_t current_mask = pc104_read_reg(INT_CTRL_MASK);
    pc104_write_reg(INT_CTRL_MASK, current_mask | mask);
    
    printf("Interrupt type %d enabled\n", type);
}

/**
 * @brief 禁用指定类型的中断
 * 
 * @param type 中断类型
 */
void interrupt_disable(interrupt_type_t type) {
    uint8_t mask = get_interrupt_mask(type);
    if (mask == 0) {
        printf("Invalid interrupt type to disable\n");
        return;
    }
    
    uint8_t current_mask = pc104_read_reg(INT_CTRL_MASK);
    pc104_write_reg(INT_CTRL_MASK, current_mask & ~mask);
    
    printf("Interrupt type %d disabled\n", type);
}

/**
 * @brief 关闭中断处理模块，释放资源
 * 
 * @return 0表示成功
 */
int interrupt_close(void) {
    // 停止中断服务线程
    g_int_thread_running = 0;
    pthread_join(g_int_thread, NULL);
    
    // 禁用所有中断
    pc104_write_reg(INT_CTRL_MASK, 0);
    
    // 销毁互斥量
    pthread_mutex_destroy(&g_int_mutex);
    
    printf("Interrupt handler closed\n");
    return 0;
}