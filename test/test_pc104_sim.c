#include "pc104_simulator.h"
#include "pc104_bus.h"
#include "rtc_driver.h"
#include "keypad_driver.h"
#include "interrupt_handler.h"
#include "storage_driver.h"
#include "display_driver.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/**
 * @brief PC104总线模拟器测试程序的主函数
 * 
 * @param argc 命令行参数数量
 * @param argv 命令行参数值
 * @return int 程序退出状态码
 */
int main(int argc, char *argv[]) {
    uint8_t value;
    
    printf("===== PC104总线模拟器测试程序 =====\n");
    
    // 初始化PC104总线模拟器
    if (pc104_sim_init() != 0) {
        fprintf(stderr, "初始化PC104总线模拟器失败\n");
        return 1;
    }
    
    printf("PC104总线模拟器初始化成功\n");
    
    // 测试写入端口
    printf("\n测试写入端口：\n");
    pc104_sim_write_port(0xA5, PC104_DATA_PORT);
    printf("写入数据端口(0x%04X)：0xA5\n", PC104_DATA_PORT);
    
    // 测试读取端口
    printf("\n测试读取端口：\n");
    value = pc104_sim_read_port(PC104_DATA_PORT);
    printf("读取数据端口(0x%04X)：0x%02X\n", PC104_DATA_PORT, value);
    
    if (value == 0xA5) {
        printf("✓ 测试通过：读写一致\n");
    } else {
        printf("✗ 测试失败：读写不一致\n");
    }
    
    // 测试RTC寄存器
    printf("\n测试RTC寄存器模拟：\n");
    // 写入并读取RTC小时寄存器
    pc104_sim_write_port(0x12, RTC_HOUR_REG); // 设置为12小时，BCD格式
    value = pc104_sim_read_port(RTC_HOUR_REG);
    printf("读取RTC小时寄存器(0x%04X)：0x%02X\n", RTC_HOUR_REG, value);
    
    // 测试按键模拟
    printf("\n测试按键事件模拟：\n");
    // 模拟按键1被按下
    pc104_sim_set_behavior(3, 1, 1);
    printf("模拟按键1被按下\n");
    
    // 读取按键状态
    value = pc104_sim_read_port(KEYPAD_STATUS_REG);
    printf("读取按键状态寄存器：0x%02X\n", value);
    
    if (value & KEYPAD_STATUS_NEW) {
        printf("✓ 测试通过：检测到新按键事件\n");
    } else {
        printf("✗ 测试失败：未检测到新按键事件\n");
    }
    
    // 读取按键数据
    value = pc104_sim_read_port(KEYPAD_DATA_REG);
    printf("读取按键数据寄存器：0x%02X\n", value);
    
    // 测试随机延迟和繁忙状态
    printf("\n测试随机繁忙状态：\n");
    pc104_sim_set_behavior(0, 1, 0); // 设置PC104总线随机繁忙
    
    for (int i = 0; i < 10; i++) {
        value = pc104_sim_read_port(PC104_STATUS_PORT);
        printf("读取状态端口 #%d：0x%02X %s\n", 
               i, value, (value & PC104_STATUS_BUSY) ? "(忙)" : "(就绪)");
        usleep(10000); // 延时10ms
    }
    
    // 关闭模拟器
    pc104_sim_close();
    
    printf("\n===== PC104总线模拟器测试完成 =====\n");
    return 0;
}