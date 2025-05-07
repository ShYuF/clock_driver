#ifndef PC104_SIMULATOR_H
#define PC104_SIMULATOR_H

#include "utils.h"

// 模拟PC104总线的内存空间大小
#define PC104_SIM_MEM_SIZE  0x1000

/**
 * @brief 初始化PC104总线模拟器
 * 
 * @return 0表示成功，-1表示失败
 */
int pc104_sim_init(void);

/**
 * @brief 从模拟的PC104端口读取一个字节
 * 
 * @param port 端口地址
 * @return 读取到的值
 */
uint8_t pc104_sim_read_port(uint16_t port);

/**
 * @brief 向模拟的PC104端口写入一个字节
 * 
 * @param value 要写入的值
 * @param port 端口地址
 */
void pc104_sim_write_port(uint8_t value, uint16_t port);

/**
 * @brief 模拟延迟，使硬件模拟更真实
 * 
 * @param microseconds 微秒数
 */
void pc104_sim_delay(uint32_t microseconds);

/**
 * @brief 关闭PC104总线模拟器
 */
void pc104_sim_close(void);

/**
 * @brief 设置特定设备的模拟行为
 * 
 * @param device_id 设备ID (0:PC104, 1:RTC, 2:Display, 3:Keypad, 4:Storage)
 * @param behavior 行为ID
 * @param param 行为参数
 */
void pc104_sim_set_behavior(int device_id, int behavior, uint32_t param);

/**
 * @brief 检查PC104总线模拟器是否初始化
 * 
 * @return 1表示已初始化，0表示未初始化
 */
int pc104_sim_is_initialized(void);

#endif // PC104_SIMULATOR_H