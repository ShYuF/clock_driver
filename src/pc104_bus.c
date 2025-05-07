#include "pc104_bus.h"

// 设备文件描述符
int g_pc104_fd = -1;

// PC104总线映射的I/O地址范围
void *g_pc104_io_mem = NULL;

/**
 * @brief 等待PC104总线就绪
 * 
 * @return 0表示成功，-1表示超时
 */
static int pc104_wait_ready(void) {
    int timeout = PC104_TIMEOUT;
    uint8_t status;
    
    // 等待总线就绪（非忙状态）
    do {
        status = inb(PC104_STATUS_PORT);
        if (!(status & PC104_STATUS_BUSY)) {
            return 0; // 总线就绪
        }
        usleep(1); // 延迟1微秒
    } while (--timeout > 0);
    
    printf("PC104 bus timeout waiting for ready\n");
    return -1; // 超时
}

/**
 * @brief 检查PC104总线错误状态
 * 
 * @return 0表示无错误，-1表示有错误
 */
static int pc104_check_error(void) {
    uint8_t status = inb(PC104_STATUS_PORT);
    
    if (status & PC104_STATUS_ERROR) {
        printf("PC104 bus error detected: status=0x%02X\n", status);
        return -1; // 总线错误
    }
    
    return 0; // 无错误
}

/**
 * @brief 初始化PC104总线
 * 
 * @return 0表示成功，-1表示失败
 */
int pc104_init(void) {
    #ifdef PLATFORM_LINUX

        // 在Linux系统下通过/dev/mem映射I/O端口
        g_pc104_fd = open("/dev/mem", O_RDWR | O_SYNC);
        if (g_pc104_fd < 0) {
            perror("Failed to open /dev/mem");
            return -1;
        }
        
        g_pc104_io_mem = mmap(NULL, 4, PROT_READ | PROT_WRITE, MAP_SHARED, g_pc104_fd, PC104_BASE_ADDR);
        if (g_pc104_io_mem == MAP_FAILED) {
            perror("Failed to map I/O memory");
            close(g_pc104_fd);
            g_pc104_fd = -1;
            return -1;
        }
    #elif defined(PLATFORM_WINDOWS)
    #elif defined(PLATFORM_UNKNOWN)
    #else
        printf("Unsupported platform\n");
        return -1;
    #endif

    // 检查PC104总线状态
    if (pc104_check_error() != 0) {
        printf("PC104 bus in error state during initialization\n");
        pc104_close();
        return -1;
    }
    
    printf("PC104 bus initialized successfully at base address 0x%X\n", PC104_BASE_ADDR);
    return 0;
}

/**
 * @brief 从PC104总线读取寄存器的值
 * 
 * @param addr 寄存器地址
 * @return 读取到的值，如果返回-1表示出错
 */
int pc104_read_reg(uint16_t addr) {
    uint8_t data;
    
    // 等待总线就绪
    if (pc104_wait_ready() != 0) {
        return -1;
    }
    
    // 写入地址
    outb(addr & 0xFF, PC104_ADDR_PORT);        // 低8位
    outb((addr >> 8) & 0xFF, PC104_ADDR_PORT + 1);  // 高8位
    
    // 发送读命令
    outb(PC104_CMD_READ, PC104_CMD_PORT);
    
    // 等待操作完成
    if (pc104_wait_ready() != 0) {
        return -1;
    }
    
    // 检查是否有错误
    if (pc104_check_error() != 0) {
        return -1;
    }
    
    // 读取数据
    data = inb(PC104_DATA_PORT);
    
    return data;
}

/**
 * @brief 向PC104总线写入寄存器的值
 * 
 * @param addr 寄存器地址
 * @param data 要写入的值
 * @return 0表示成功，-1表示失败
 */
int pc104_write_reg(uint16_t addr, uint8_t data) {
    // 等待总线就绪
    if (pc104_wait_ready() != 0) {
        return -1;
    }
    
    // 写入地址
    outb(addr & 0xFF, PC104_ADDR_PORT);        // 低8位
    outb((addr >> 8) & 0xFF, PC104_ADDR_PORT + 1);  // 高8位
    
    // 写入数据
    outb(data, PC104_DATA_PORT);
    
    // 发送写命令
    outb(PC104_CMD_WRITE, PC104_CMD_PORT);
    
    // 等待操作完成
    if (pc104_wait_ready() != 0) {
        return -1;
    }
    
    // 检查是否有错误
    if (pc104_check_error() != 0) {
        return -1;
    }
    
    return 0;
}

/**
 * @brief 关闭PC104总线
 * 
 * @return 0表示成功，-1表示失败
 */
int pc104_close(void) {
    #ifdef PLATFORM_LINUX
        if (g_pc104_io_mem != NULL) {
            munmap(g_pc104_io_mem, 4);
            g_pc104_io_mem = NULL;
        }
        
        if (g_pc104_fd >= 0) {
            close(g_pc104_fd);
            g_pc104_fd = -1;
        }
    #elif defined(PLATFORM_WINDOWS)
    #elif defined(PLATFORM_UNKNOWN)
    #else
    #endif

    printf("PC104 bus closed\n");
    return 0;
}