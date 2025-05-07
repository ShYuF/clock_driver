#include "pc104_simulator.h"
#include "pc104_bus.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/**
 * @brief 简单的PC104总线模拟器测试程序
 * 
 * 此程序测试PC104总线模拟器的基本功能，不涉及电子钟完整功能
 */
int main(int argc, char *argv[]) {
    int ret;
    uint8_t value;
    
    printf("===== PC104总线模拟器测试程序 =====\n");
    
    // 初始化PC104总线
    ret = pc104_init();
    if (ret != 0) {
        fprintf(stderr, "初始化PC104总线失败\n");
        return 1;
    }
    
    printf("PC104总线初始化成功\n");
    
    // 测试读写基本端口
    printf("\n测试基本端口读写：\n");
    
    // 写入数据端口
    printf("写入数据端口(0x%04X)：0xA5\n", PC104_DATA_PORT);
    ret = pc104_write_reg(PC104_DATA_PORT, 0xA5);
    if (ret != 0) {
        fprintf(stderr, "写入数据端口失败\n");
    }
    
    // 读取数据端口
    value = pc104_read_reg(PC104_DATA_PORT);
    printf("读取数据端口(0x%04X)：0x%02X\n", PC104_DATA_PORT, value);
    
    // 测试RTC相关寄存器
    printf("\n测试RTC相关寄存器：\n");
    
    // 读取RTC秒寄存器
    value = pc104_read_reg(RTC_SECOND_REG);
    printf("读取RTC秒寄存器(0x%04X)：0x%02X\n", RTC_SECOND_REG, value);
    
    // 读取RTC分钟寄存器
    value = pc104_read_reg(RTC_MINUTE_REG);
    printf("读取RTC分钟寄存器(0x%04X)：0x%02X\n", RTC_MINUTE_REG, value);
    
    // 读取RTC小时寄存器
    value = pc104_read_reg(RTC_HOUR_REG);
    printf("读取RTC小时寄存器(0x%04X)：0x%02X\n", RTC_HOUR_REG, value);
    
    // 测试按键模拟
    printf("\n测试按键模拟：\n");
    
    // 模拟按下按键1
    printf("模拟按下按键1\n");
    pc104_sim_set_behavior(3, 1, 1); // 设备3是按键，行为1是按键事件，参数1是按键编号
    
    // 读取按键状态寄存器
    value = pc104_read_reg(KEYPAD_STATUS_REG);
    printf("读取按键状态寄存器(0x%04X)：0x%02X\n", KEYPAD_STATUS_REG, value);
    
    // 如果状态寄存器显示有新按键
    if (value & KEYPAD_STATUS_NEW) {
        // 读取按键数据寄存器
        value = pc104_read_reg(KEYPAD_DATA_REG);
        printf("读取按键数据寄存器(0x%04X)：0x%02X\n", KEYPAD_DATA_REG, value);
        
        // 确认按键事件
        pc104_write_reg(KEYPAD_CONTROL_REG, KEYPAD_CTRL_ACK);
        printf("已确认按键事件\n");
    }
    
    // 测试读写寄存器功能
    printf("\n测试完整读写寄存器功能：\n");
    
    // 测试地址0x700（存储起始地址）
    uint16_t test_addr = STORAGE_BASE_ADDR;
    
    // 写入测试数据
    printf("写入地址0x%04X：0x42\n", test_addr);
    ret = pc104_write_reg(test_addr, 0x42);
    if (ret != 0) {
        fprintf(stderr, "写入寄存器失败\n");
    }
    
    // 读取测试数据
    value = pc104_read_reg(test_addr);
    printf("读取地址0x%04X：0x%02X\n", test_addr, value);
    
    if (value == 0x42) {
        printf("✓ 测试成功：读写一致\n");
    } else {
        printf("✗ 测试失败：读写不一致\n");
    }
    
    // 关闭PC104总线
    pc104_close();
    
    printf("\n===== PC104总线模拟器测试完成 =====\n");
    return 0;
}