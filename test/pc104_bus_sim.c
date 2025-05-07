#include "pc104_bus.h"
#include "pc104_simulator.h"
#include <stdio.h>

/**
 * @brief 初始化PC104总线 - 模拟版本
 * 
 * 该函数使用模拟器而不是真实硬件
 * 
 * @return 0表示成功，-1表示失败
 */
int pc104_init(void) {
    int ret;
    
    // 初始化PC104总线模拟器
    ret = pc104_sim_init();
    if (ret != 0) {
        printf("Failed to initialize PC104 simulator\n");
        return -1;
    }
    
    printf("PC104 bus simulator initialized at base address 0x%X\n", PC104_BASE_ADDR);
    return 0;
}

/**
 * @brief 从PC104总线读取寄存器的值 - 模拟版本
 * 
 * @param addr 寄存器地址
 * @return 读取到的值，如果返回-1表示出错
 */
int pc104_read_reg(uint16_t addr) {
    uint8_t data;
    
    // 通过模拟器读取端口
    data = pc104_sim_read_port(addr);
    
    return data;
}

/**
 * @brief 向PC104总线写入寄存器的值 - 模拟版本
 * 
 * @param addr 寄存器地址
 * @param data 要写入的值
 * @return 0表示成功，-1表示失败
 */
int pc104_write_reg(uint16_t addr, uint8_t data) {
    // 通过模拟器写入端口
    pc104_sim_write_port(data, addr);
    
    return 0;
}

/**
 * @brief 关闭PC104总线 - 模拟版本
 * 
 * @return 0表示成功，-1表示失败
 */
int pc104_close(void) {
    // 关闭模拟器
    pc104_sim_close();
    
    printf("PC104 bus simulator closed\n");
    return 0;
}