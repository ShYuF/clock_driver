#ifndef STORAGE_DRIVER_H
#define STORAGE_DRIVER_H

#include "utils.h"

// 存储器基地址和大小定义
#define STORAGE_BASE_ADDR           0x700            // 存储器基地址
#define STORAGE_SIZE                0x1000           // 存储器总大小（4KB）
#define STORAGE_PAGE_SIZE           64               // 存储页大小（字节）

// 存储器寄存器地址
#define STORAGE_DATA_REG            (STORAGE_BASE_ADDR + 0)  // 数据寄存器
#define STORAGE_ADDR_REG_L          (STORAGE_BASE_ADDR + 1)  // 地址寄存器（低字节）
#define STORAGE_ADDR_REG_H          (STORAGE_BASE_ADDR + 2)  // 地址寄存器（高字节）
#define STORAGE_CTRL_REG            (STORAGE_BASE_ADDR + 3)  // 控制寄存器
#define STORAGE_STATUS_REG          (STORAGE_BASE_ADDR + 4)  // 状态寄存器

// 控制寄存器命令
#define STORAGE_CTRL_READ           0x01              // 读取命令
#define STORAGE_CTRL_WRITE          0x02              // 写入命令
#define STORAGE_CTRL_ERASE          0x04              // 擦除页命令

// 状态寄存器位定义
#define STORAGE_STATUS_BUSY         0x01              // 忙状态标志
#define STORAGE_STATUS_ERROR        0x80              // 错误状态标志

// 记录区域定义
#define STORAGE_RECORD_BASE_ADDR    0x100             // 记录区域基地址
#define STORAGE_RECORD_SIZE         sizeof(uint32_t)  // 每条记录大小
#define STORAGE_MAX_RECORDS         16                // 最大记录数量

int storage_init(void);
int storage_read(uint16_t addr, uint8_t *buffer, uint16_t size);
int storage_write(uint16_t addr, const uint8_t *buffer, uint16_t size);
int storage_save_record(uint8_t record_id, uint32_t time_ms);
int storage_read_record(uint8_t record_id, uint32_t *time_ms);
int storage_close(void);

#endif