#ifndef PC104_BUS_H
#define PC104_BUS_H

#include "utils.h"

// PC104总线基地址和端口
#define PC104_BASE_ADDR    0x300                  // PC104总线基地址
#define PC104_DATA_PORT    (PC104_BASE_ADDR + 0)  // 数据端口
#define PC104_ADDR_PORT    (PC104_BASE_ADDR + 1)  // 地址端口
#define PC104_CMD_PORT     (PC104_BASE_ADDR + 2)  // 命令端口
#define PC104_STATUS_PORT  (PC104_BASE_ADDR + 3)  // 状态端口

#define PC104_CMD_READ      0x01    // 读命令
#define PC104_CMD_WRITE     0x02    // 写命令

#define PC104_STATUS_BUSY   0x01    // 总线忙状态位
#define PC104_STATUS_ERROR  0x02    // 总线错误状态位

#define PC104_TIMEOUT       1000 // 超时时间（单位：微秒）

extern int g_pc104_fd;             // PC104总线文件描述符
extern void *g_pc104_io_mem;       // PC104总线映射的I/O内存

int pc104_init(void);
int pc104_read_reg(uint16_t addr);
int pc104_write_reg(uint16_t addr, uint8_t data);
int pc104_close(void);

#endif