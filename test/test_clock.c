#include "clock_driver.h"
#include "pc104_bus.h"
#include "pc104_simulator.h"
#include "keypad_driver.h"
#include "interrupt_handler.h"
#include "storage_driver.h"
#include "display_driver.h"
#include "rtc_driver.h"

#include <signal.h>
#include <pthread.h>

// 退出标志，用于控制主循环退出
static volatile int g_running = 1;

/**
 * @brief 信号处理函数，用于优雅退出
 * 
 * @param sig 信号编号
 */
void signal_handler(int sig) {
    printf("接收到信号 %d，正在退出测试...\n", sig);
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
 * @brief 模拟按键线程
 * 
 * 该线程定期模拟按键操作，测试电子钟响应
 */
void* keypad_simulation_thread(void *arg) {
    printf("[测试] 按键模拟线程启动\n");
    
    // 等待3秒，让系统初始化
    sleep(3);
    
    // 第1步：进入时间设置模式
    printf("[测试] 模拟按下按键1 - 进入设置模式\n");
    pc104_sim_set_behavior(3, 1, 1);
    keypad_poll();
    sleep(1);
    
    // 第2步：设置小时+1
    printf("[测试] 模拟按下按键2 - 小时+1\n");
    pc104_sim_set_behavior(3, 1, 2);
    keypad_poll();
    sleep(1);
    
    // 第3步：设置分钟+1
    printf("[测试] 模拟按下按键3 - 分钟+1\n"); 
    pc104_sim_set_behavior(3, 1, 3);
    keypad_poll();
    sleep(1);
    
    // 第4步：返回正常模式
    printf("[测试] 模拟按下按键1 - 返回正常模式\n");
    pc104_sim_set_behavior(3, 1, 1);
    keypad_poll();
    sleep(2);
    
    // 第5步：进入秒表模式
    printf("[测试] 模拟按下按键2 - 进入秒表模式\n");
    pc104_sim_set_behavior(3, 1, 2);
    keypad_poll();
    sleep(1);
    
    // 第6步：启动秒表
    printf("[测试] 模拟按下按键2 - 启动秒表\n");
    pc104_sim_set_behavior(3, 1, 2);
    keypad_poll();
    sleep(2); // 让秒表运行2秒
    
    // 第7步：保存秒表记录
    printf("[测试] 模拟按下按键3 - 保存秒表记录\n");
    pc104_sim_set_behavior(3, 1, 3);
    keypad_poll();
    sleep(1);
    
    // 第8步：暂停秒表
    printf("[测试] 模拟按下按键2 - 暂停秒表\n");
    pc104_sim_set_behavior(3, 1, 2);
    keypad_poll();
    sleep(1);
    
    // 第9步：复位秒表（在秒表暂停状态下按3）
    printf("[测试] 模拟按下按键3 - 复位秒表\n");
    pc104_sim_set_behavior(3, 1, 3);
    keypad_poll();
    sleep(1);
    
    // 第10步：返回正常模式
    printf("[测试] 模拟按下按键1 - 返回正常模式\n");
    pc104_sim_set_behavior(3, 1, 1);
    keypad_poll();
    
    printf("[测试] 按键模拟测试序列完成\n");
    
    // 测试完成后不再随机测试，避免干扰结果观察
    
    printf("[测试] 按键模拟线程退出\n");
    return NULL;
}

/**
 * @brief 打印时钟状态
 * 
 * @param time 当前时间
 */
void print_clock_status(rtc_time_t *time) {
    printf("[测试] 当前时间：%02d:%02d:%02d\n", 
           time->hour, time->minute, time->second);
}

/**
 * @brief 电子钟测试程序主函数
 * 
 * @param argc 命令行参数数量
 * @param argv 命令行参数值
 * @return int 程序退出状态码
 */
int main(int argc, char *argv[]) {
    int ret;
    pthread_t keypad_thread;
    rtc_time_t current_time;
    
    printf("===== 电子钟模拟测试程序启动 =====\n");
    printf("注意：此程序在虚拟环境中运行，使用模拟的PC104总线\n");
    
    // 注册信号处理函数
    setup_signal_handlers();
    
    // 初始化电子钟驱动
    ret = clock_driver_init();
    if (ret != 0) {
        fprintf(stderr, "初始化电子钟驱动失败\n");
        return 1;
    }
    
    // 启动电子钟
    ret = clock_start();
    if (ret != 0) {
        fprintf(stderr, "启动电子钟失败\n");
        return 1;
    }
    
    printf("电子钟已启动，开始测试\n");
    
    // 创建按键模拟线程
    ret = pthread_create(&keypad_thread, NULL, keypad_simulation_thread, NULL);
    if (ret != 0) {
        fprintf(stderr, "创建按键模拟线程失败\n");
        return 1;
    }
    
    // 主循环
    int cycle = 0;
    while (g_running) {
        // 定期获取并显示当前时间
        if (clock_get_time(&current_time) == 0) {
            print_clock_status(&current_time);
        }
        
        // 轮询按键
        keypad_poll();
        
        // 每10个周期测试一次秒表功能
        if (++cycle % 10 == 0) {
            // 在适当时机，尝试切换到秒表模式
            if (cycle == 10) {
                printf("[测试] 尝试切换到秒表模式\n");
                pc104_sim_set_behavior(3, 1, 2); // 模拟按下切换到秒表模式的按键
                keypad_poll();
                usleep(500000);
            }
            // 启动秒表
            else if (cycle == 20) {
                printf("[测试] 尝试启动秒表\n");
                pc104_sim_set_behavior(3, 1, 2);
                keypad_poll();
                usleep(500000);
            }
            // 停止秒表
            else if (cycle == 30) {
                printf("[测试] 尝试停止秒表\n");
                pc104_sim_set_behavior(3, 1, 2);
                keypad_poll();
                usleep(500000);
            }
            // 复位秒表
            else if (cycle == 40) {
                printf("[测试] 尝试复位秒表\n");
                pc104_sim_set_behavior(3, 1, 3);
                keypad_poll();
                usleep(500000);
            }
            // 返回普通模式
            else if (cycle == 50) {
                printf("[测试] 尝试返回普通模式\n");
                pc104_sim_set_behavior(3, 1, 1);
                keypad_poll();
                cycle = 0; // 重置循环
            }
        }
        
        // 主线程休眠，避免CPU占用过高
        usleep(1000000); // 1秒
    }
    
    // 等待按键模拟线程结束
    pthread_join(keypad_thread, NULL);
    
    // 关闭资源
    clock_stop();
    
    // 关闭各个驱动模块
    display_close();
    keypad_close();
    interrupt_close();
    storage_close();
    rtc_close();
    pc104_close();
    
    printf("===== 电子钟测试程序已安全退出 =====\n");
    return 0;
}