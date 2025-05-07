#include "clock_driver.h"
#include "pc104_bus.h"
#include "keypad_driver.h"
#include "interrupt_handler.h"
#include "storage_driver.h"
#include "display_driver.h"
#include "rtc_driver.h"

#include <signal.h>

// 退出标志，用于控制主循环退出
static volatile int g_running = 1;

/**
 * @brief 信号处理函数，用于优雅退出
 * 
 * @param sig 信号编号
 */
void signal_handler(int sig) {
    printf("received signal %d, exiting...\n", sig);
    g_running = 0;
}

/**
 * @brief 注册信号处理函数
 */
void setup_signal_handlers(void) {
    signal(SIGINT, signal_handler);   // 处理CTRL+C
    signal(SIGTERM, signal_handler);  // 处理终止信号
}

/**
 * @brief 电子钟应用程序主函数
 * 
 * @param argc 命令行参数数量
 * @param argv 命令行参数值
 * @return int 程序退出状态码
 */
int main(int argc, char *argv[]) {
    int ret;
    
    printf("starting electronic clock application...\n");
    
    // 注册信号处理函数
    setup_signal_handlers();
    
    // 初始化电子钟驱动
    ret = clock_driver_init();
    if (ret != 0) {
        fprintf(stderr, "initialize clock driver failed\n");
        return 1;
    }
    
    // 启动电子钟
    ret = clock_start();
    if (ret != 0) {
        fprintf(stderr, "start clock failed\n");
        return 1;
    }
    
    printf("clock started successfully\n");
    
    // 主循环
    while (g_running) {
        // 轮询按键
        keypad_poll();
        
        // 主线程休眠，避免CPU占用过高
        usleep(10000); // 10ms
    }
    
    // 关闭资源
    clock_stop();
    
    // 关闭各个驱动模块
    display_close();
    keypad_close();
    interrupt_close();
    storage_close();
    rtc_close();
    pc104_close();
    
    printf("application exited gracefully\n");
    return 0;
}