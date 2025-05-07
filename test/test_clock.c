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
#include <time.h>
#include <stdarg.h>

// 退出标志，用于控制主循环退出
static volatile int g_running = 1;
// 测试计时器，用于有限时间测试
static time_t g_test_start_time = 0;
static int g_test_duration = 0;
// 日志文件指针
static FILE* g_log_file = NULL;

/**
 * @brief 定制化输出函数，根据配置输出到终端或日志文件
 * 
 * @param format 格式化字符串
 * @param ... 可变参数列表
 */
void test_printf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    if (g_log_file) {
        // 输出时间戳
        time_t now;
        struct tm *timeinfo;
        char timestamp[20];
        
        time(&now);
        timeinfo = localtime(&now);
        strftime(timestamp, sizeof(timestamp), "%H:%M:%S", timeinfo);
        
        fprintf(g_log_file, "[%s] ", timestamp);
        vfprintf(g_log_file, format, args);
        fflush(g_log_file);
    } else {
        vprintf(format, args);
        fflush(stdout);
    }
    
    va_end(args);
}

/**
 * @brief 信号处理函数，用于优雅退出
 * 
 * @param sig 信号编号
 */
void signal_handler(int sig) {
    test_printf("接收到信号 %d，正在退出测试...\n", sig);
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
    test_printf("[测试] 按键模拟线程启动\n");
    
    // 等待3秒，让系统初始化
    sleep(3);
    
    // 第1步：进入时间设置模式
    test_printf("[测试] 模拟按下按键1 - 进入设置模式\n");
    pc104_sim_set_behavior(3, 1, 1);
    keypad_poll();
    sleep(1);
    
    // 第2步：设置小时+1
    test_printf("[测试] 模拟按下按键2 - 小时+1\n");
    pc104_sim_set_behavior(3, 1, 2);
    keypad_poll();
    sleep(1);
    
    // 第3步：设置分钟+1
    test_printf("[测试] 模拟按下按键3 - 分钟+1\n"); 
    pc104_sim_set_behavior(3, 1, 3);
    keypad_poll();
    sleep(1);
    test_printf("[测试] 注意: 设置分钟会自动将秒清零\n");
    
    // 第4步：返回正常模式
    test_printf("[测试] 模拟按下按键1 - 返回正常模式\n");
    pc104_sim_set_behavior(3, 1, 1);
    keypad_poll();
    sleep(1);
    test_printf("[测试] 已应用时间设置并返回正常模式\n");
    sleep(1);
    
    // 第5步：进入秒表模式
    test_printf("[测试] 模拟按下按键2 - 进入秒表模式\n");
    pc104_sim_set_behavior(3, 1, 2);
    keypad_poll();
    sleep(1);
    test_printf("[测试] 秒表初始状态: 00:00\n");
    
    // 第6步：启动秒表
    test_printf("[测试] 模拟按下按键2 - 启动秒表\n");
    pc104_sim_set_behavior(3, 1, 2);
    keypad_poll();
    
    // 让秒表运行约3秒
    test_printf("[测试] 秒表正在运行 - 等待3秒...\n");
    sleep(3); 
    
    // 第7步：保存秒表记录
    test_printf("[测试] 模拟按下按键3 - 保存秒表记录 (秒表仍在运行)\n");
    pc104_sim_set_behavior(3, 1, 3);
    keypad_poll();
    sleep(1);
    
    // 第8步：暂停秒表
    test_printf("[测试] 模拟按下按键2 - 暂停秒表\n");
    pc104_sim_set_behavior(3, 1, 2);
    keypad_poll();
    sleep(1);
    
    // 第9步：复位秒表（在秒表暂停状态下按3）
    test_printf("[测试] 模拟按下按键3 - 复位秒表 (秒表已暂停)\n");
    pc104_sim_set_behavior(3, 1, 3);
    keypad_poll();
    test_printf("[测试] 秒表已复位至 00:00\n");
    sleep(1);
    
    // 第10步：再次启动秒表并等待一小段时间
    test_printf("[测试] 模拟按下按键2 - 再次启动秒表\n");
    pc104_sim_set_behavior(3, 1, 2);
    keypad_poll();
    sleep(2);
    
    // 第11步：暂停秒表
    test_printf("[测试] 模拟按下按键2 - 再次暂停秒表\n");
    pc104_sim_set_behavior(3, 1, 2);
    keypad_poll();
    test_printf("[测试] 检查秒表暂停值是否约为2秒\n");
    sleep(1);
    
    // 第12步：返回正常模式
    test_printf("[测试] 模拟按下按键1 - 返回正常模式\n");
    pc104_sim_set_behavior(3, 1, 1);
    keypad_poll();
    
    test_printf("[测试] 按键模拟测试序列完成\n");
    test_printf("[测试] 按键模拟线程退出\n");
    return NULL;
}

/**
 * @brief 打印时钟状态
 * 
 * @param time 当前时间
 */
void print_clock_status(rtc_time_t *time) {
    test_printf("[测试] 当前时间：%02d:%02d:%02d\n", 
           time->hour, time->minute, time->second);
}

/**
 * @brief 显示使用帮助
 */
void show_usage(const char* program_name) {
    printf("用法: %s [测试时长] [输出日志]\n", program_name);
    printf("  测试时长: 测试持续的时间(秒)，不给出则为无限\n");
    printf("  输出日志: 1=输出到日志文件, 0=输出到终端，默认为0\n");
    printf("  注意: 这两个参数要么全部给出，要么全不给出\n");
    printf("例子:\n");
    printf("  %s         # 无限时长测试，输出到终端\n", program_name);
    printf("  %s 60 1    # 测试60秒，输出到日志文件\n", program_name);
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
    int log_to_file = 0;
    
    // 解析命令行参数
    if (argc != 1 && argc != 3) {
        show_usage(argv[0]);
        return 1;
    }
    
    if (argc == 3) {
        // 解析测试时长
        g_test_duration = atoi(argv[1]);
        if (g_test_duration <= 0) {
            printf("错误: 测试时长必须是正整数\n");
            show_usage(argv[0]);
            return 1;
        }
        
        // 解析日志输出选项
        log_to_file = atoi(argv[2]);
        if (log_to_file != 0 && log_to_file != 1) {
            printf("错误: 输出日志参数必须是0或1\n");
            show_usage(argv[0]);
            return 1;
        }
        
        // 如果需要输出到日志文件，创建日志文件
        if (log_to_file) {
            // 生成包含日期时间的日志文件名
            time_t now;
            struct tm *timeinfo;
            char filename[64];
            
            time(&now);
            timeinfo = localtime(&now);
            strftime(filename, sizeof(filename), "clock_test_%Y%m%d_%H%M%S.log", timeinfo);
            
            g_log_file = fopen(filename, "w");
            if (g_log_file == NULL) {
                perror("无法创建日志文件");
                return 1;
            }
            printf("测试日志将输出到文件: %s\n", filename);
        }
        
        // 记录测试开始时间
        time(&g_test_start_time);
    }
    
    test_printf("===== 电子钟模拟测试程序启动 =====\n");
    if (g_test_duration > 0) {
        test_printf("测试将持续 %d 秒\n", g_test_duration);
    } else {
        test_printf("测试将无限期运行，直到手动中止\n");
    }
    test_printf("注意：此程序在虚拟环境中运行，使用模拟的PC104总线\n");
    
    // 注册信号处理函数
    setup_signal_handlers();
    
    // 初始化电子钟驱动
    ret = clock_driver_init();
    if (ret != 0) {
        test_printf("初始化电子钟驱动失败\n");
        if (g_log_file) fclose(g_log_file);
        return 1;
    }
    
    // 启动电子钟
    ret = clock_start();
    if (ret != 0) {
        test_printf("启动电子钟失败\n");
        if (g_log_file) fclose(g_log_file);
        return 1;
    }
    
    test_printf("电子钟已启动，开始测试\n");
    
    // 创建按键模拟线程
    ret = pthread_create(&keypad_thread, NULL, keypad_simulation_thread, NULL);
    if (ret != 0) {
        test_printf("创建按键模拟线程失败\n");
        if (g_log_file) fclose(g_log_file);
        return 1;
    }
    
    // 主循环
    int cycle = 0;
    while (g_running) {
        // 检查测试时长是否已到
        if (g_test_duration > 0) {
            time_t current_time;
            time(&current_time);
            if (difftime(current_time, g_test_start_time) >= g_test_duration) {
                test_printf("测试时长已到，正在退出测试...\n");
                g_running = 0;
                continue;
            }
        }
        
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
                test_printf("[测试] 尝试切换到秒表模式\n");
                pc104_sim_set_behavior(3, 1, 2); // 模拟按下切换到秒表模式的按键
                keypad_poll();
                usleep(500000);
            }
            // 启动秒表
            else if (cycle == 20) {
                test_printf("[测试] 尝试启动秒表\n");
                pc104_sim_set_behavior(3, 1, 2);
                keypad_poll();
                usleep(500000);
            }
            // 停止秒表
            else if (cycle == 30) {
                test_printf("[测试] 尝试停止秒表\n");
                pc104_sim_set_behavior(3, 1, 2);
                keypad_poll();
                usleep(500000);
            }
            // 复位秒表
            else if (cycle == 40) {
                test_printf("[测试] 尝试复位秒表\n");
                pc104_sim_set_behavior(3, 1, 3);
                keypad_poll();
                usleep(500000);
            }
            // 返回普通模式
            else if (cycle == 50) {
                test_printf("[测试] 尝试返回普通模式\n");
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
    
    test_printf("===== 电子钟测试程序已安全退出 =====\n");
    
    // 关闭日志文件
    if (g_log_file) {
        fclose(g_log_file);
        printf("测试日志已保存\n");
    }
    
    return 0;
}