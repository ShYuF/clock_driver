#include "storage_driver.h"
#include "pc104_bus.h"

/**
 * @brief 等待存储器就绪
 * 
 * @return 0表示成功，-1表示超时或错误
 */
static int storage_wait_ready(void) {
    int timeout = PC104_TIMEOUT;
    uint8_t status;
    
    do {
        status = pc104_read_reg(STORAGE_STATUS_REG);
        
        // 尝试处理0xFF初始状态
        if (status == 0xFF) {
            // 尝试发送命令复位存储器
            pc104_write_reg(STORAGE_CTRL_REG, 0);
            usleep(10000);  // 等待10ms
            continue;
        }
        
        // 检查错误状态
        if (status & STORAGE_STATUS_ERROR) {
            printf("Storage error detected: status=0x%02X\n", status);
            return -1;
        }
        
        // 检查是否就绪
        if (!(status & STORAGE_STATUS_BUSY)) {
            return 0;  // 存储器就绪
        }
        
        usleep(1);  // 延时1微秒
    } while (--timeout > 0);
    
    printf("Storage timeout waiting for ready\n");
    return -1;  // 超时
}

/**
 * @brief 设置存储器访问地址
 * 
 * @param addr 要访问的地址
 * @return 0表示成功，-1表示失败
 */
static int storage_set_address(uint16_t addr) {
    // 检查地址有效性
    if (addr >= STORAGE_SIZE) {
        printf("Invalid storage address: 0x%04X\n", addr);
        return -1;
    }
    
    // 写入地址寄存器
    if (pc104_write_reg(STORAGE_ADDR_REG_L, addr & 0xFF) != 0 ||
        pc104_write_reg(STORAGE_ADDR_REG_H, (addr >> 8) & 0xFF) != 0) {
        
        printf("Failed to set storage address\n");
        return -1;
    }
    
    return 0;
}

/**
 * @brief 初始化存储模块
 * 
 * @return 0表示成功，-1表示失败
 */
int storage_init(void) {
    uint8_t status;
    
    // 读取状态寄存器
    status = pc104_read_reg(STORAGE_STATUS_REG);
    if (status & STORAGE_STATUS_ERROR) {
        printf("Storage in error state during initialization\n");
        return -1;
    }
    
    // 等待存储器就绪
    if (storage_wait_ready() != 0) {
        printf("Storage not ready during initialization\n");
        return -1;
    }
    
    printf("Storage driver initialized successfully\n");
    return 0;
}

/**
 * @brief 从存储器读取数据
 * 
 * @param addr 存储器地址
 * @param buffer 数据缓冲区
 * @param size 要读取的字节数
 * @return 0表示成功，-1表示失败
 */
int storage_read(uint16_t addr, uint8_t *buffer, uint16_t size) { 
    // 检查参数
    if (buffer == NULL) {
        printf("Invalid buffer pointer\n");
        return -1;
    }
    
    if (addr + size > STORAGE_SIZE) {
        printf("Read operation exceeds storage size\n");
        return -1;
    }
    
    // 等待存储器就绪
    if (storage_wait_ready() != 0) {
        return -1;
    }
    
    // 设置起始地址
    if (storage_set_address(addr) != 0) {
        return -1;
    }
    
    // 发送读命令
    if (pc104_write_reg(STORAGE_CTRL_REG, STORAGE_CTRL_READ) != 0) {
        printf("Failed to send read command\n");
        return -1;
    }
    
    // 等待存储器就绪
    if (storage_wait_ready() != 0) {
        return -1;
    }
    
    // 逐字节读取数据
    for (uint16_t i = 0; i < size; i++) {
        // 设置读取地址
        if (storage_set_address(addr + i) != 0) {
            return -1;
        }
        
        // 读取数据
        buffer[i] = pc104_read_reg(STORAGE_DATA_REG);
        
        // 检查读取是否成功
        if (buffer[i] == (uint8_t)-1) {
            printf("Failed to read data at address 0x%04X\n", addr + i);
            return -1;
        }
    }
    
    return 0;
}

/**
 * @brief 向存储器写入数据
 * 
 * @param addr 存储器地址
 * @param buffer 数据缓冲区
 * @param size 要写入的字节数
 * @return 0表示成功，-1表示失败
 */
int storage_write(uint16_t addr, const uint8_t *buffer, uint16_t size) {
    // 检查参数
    if (buffer == NULL) {
        printf("Invalid buffer pointer\n");
        return -1;
    }
    
    if (addr + size > STORAGE_SIZE) {
        printf("Write operation exceeds storage size\n");
        return -1;
    }
    
    // 等待存储器就绪
    if (storage_wait_ready() != 0) {
        return -1;
    }
    
    // 逐字节写入数据
    for (uint16_t i = 0; i < size; i++) {
        // 设置写入地址
        if (storage_set_address(addr + i) != 0) {
            return -1;
        }
        
        // 写入数据
        if (pc104_write_reg(STORAGE_DATA_REG, buffer[i]) != 0) {
            printf("Failed to write data at address 0x%04X\n", addr + i);
            return -1;
        }
        
        // 发送写命令
        if (pc104_write_reg(STORAGE_CTRL_REG, STORAGE_CTRL_WRITE) != 0) {
            printf("Failed to send write command\n");
            return -1;
        }
        
        // 等待写入完成
        if (storage_wait_ready() != 0) {
            return -1;
        }
    }
    
    return 0;
}

/**
 * @brief 保存秒表记录
 * 
 * @param record_id 记录ID
 * @param time_ms 秒表时间（毫秒）
 * @return 0表示成功，-1表示失败
 */
int storage_save_record(uint8_t record_id, uint32_t time_ms) {
    // 检查记录ID是否有效
    if (record_id >= STORAGE_MAX_RECORDS) {
        printf("Invalid record ID: %d\n", record_id);
        return -1;
    }

    uint16_t record_addr;    
    // 计算记录地址
    record_addr = STORAGE_RECORD_BASE_ADDR + (record_id * STORAGE_RECORD_SIZE);
    
    // 写入记录数据
    return storage_write(record_addr, (const uint8_t*)&time_ms, sizeof(time_ms));
}

/**
 * @brief 读取秒表记录
 * 
 * @param record_id 记录ID
 * @param time_ms 保存秒表时间的指针
 * @return 0表示成功，-1表示失败
 */
int storage_read_record(uint8_t record_id, uint32_t *time_ms) {
    // 检查参数
    if (time_ms == NULL) {
        printf("Invalid time_ms pointer\n");
        return -1;
    }
    
    // 检查记录ID是否有效
    if (record_id >= STORAGE_MAX_RECORDS) {
        printf("Invalid record ID: %d\n", record_id);
        return -1;
    }
    
    uint16_t record_addr;
    // 计算记录地址
    record_addr = STORAGE_RECORD_BASE_ADDR + (record_id * STORAGE_RECORD_SIZE);
    
    // 读取记录数据
    return storage_read(record_addr, (uint8_t*)time_ms, sizeof(*time_ms));
}

/**
 * @brief 关闭存储模块
 * 
 * @return 0表示成功
 */
int storage_close(void) {
    printf("Storage driver closed\n");
    return 0;
}