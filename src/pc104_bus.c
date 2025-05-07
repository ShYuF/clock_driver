#include "pc104_bus.h"

// 设备文件描述符
int g_pc104_fd = -1;

// PC104总线映射的I/O地址范围
void *g_pc104_io_mem = NULL;

/**
 * @brief 从I/O端口读取一个字节
 * 
 * @param port 端口地址
 * @return 读取到的值，失败返回0xFF
 */
static uint8_t port_read_byte(uint16_t port) {
    uint8_t value = 0xFF;
    
    if (g_pc104_fd < 0) return value;
    
    if (lseek(g_pc104_fd, port, SEEK_SET) != port) {
        perror("lseek failed");
        return value;
    }
    
    if (read(g_pc104_fd, &value, 1) != 1) {
        perror("read from port failed");
    }
    
    return value;
}

/**
 * @brief 向I/O端口写入一个字节
 * 
 * @param value 要写入的值
 * @param port 端口地址
 * @return 0表示成功，-1表示失败
 */
static int port_write_byte(uint8_t value, uint16_t port) {
    if (g_pc104_fd < 0) return -1;
    
    if (lseek(g_pc104_fd, port, SEEK_SET) != port) {
        perror("lseek failed");
        return -1;
    }
    
    if (write(g_pc104_fd, &value, 1) != 1) {
        perror("write to port failed");
        return -1;
    }
    
    return 0;
}

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
        status = port_read_byte(PC104_STATUS_PORT);
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
    uint8_t status;
    
    status = port_read_byte(PC104_STATUS_PORT);
    
    // 调试输出当前读取到的状态值
    printf("PC104 status register value: 0x%02X\n", status);
    
    // 初始状态可能是0xFF，这是一个特殊情况，我们不将其视为错误
    if (status == 0xFF) {
        printf("PC104 bus initial state detected, attempting reset\n");
        // 尝试向命令端口写入复位命令(0x00)
        port_write_byte(0x00, PC104_CMD_PORT);
        usleep(1000); // 等待1ms让设备有时间响应
        return 0;
    }
    
    // 只检查特定的错误位
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
    int retry_count = 3; // 添加重试机制
    
    #ifdef PLATFORM_LINUX
        // 在Linux系统下通过/dev/port访问I/O端口
        g_pc104_fd = open("/dev/port", O_RDWR);
        if (g_pc104_fd < 0) {
            perror("Failed to open /dev/port");
            return -1;
        }
        
        printf("Port device opened successfully\n");
    #elif defined(PLATFORM_WINDOWS)
    #elif defined(PLATFORM_UNKNOWN)
    #else
        printf("Unsupported platform\n");
        return -1;
    #endif

    // 初始化PC104总线 - 添加重试逻辑
    while (retry_count--) {
        // 向命令端口写入复位命令(0x00)
        port_write_byte(0x00, PC104_CMD_PORT);
        usleep(10000); // 等待10ms
        
        // 检查PC104总线状态
        if (pc104_check_error() == 0) {
            printf("PC104 bus initialized successfully at base address 0x%X\n", PC104_BASE_ADDR);
            return 0;
        }
        
        printf("Retrying PC104 initialization, attempts left: %d\n", retry_count);
        usleep(100000); // 重试前等待100ms
    }
    
    printf("PC104 bus initialization failed after multiple attempts\n");
    pc104_close();
    return -1;
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
    port_write_byte(addr & 0xFF, PC104_ADDR_PORT);        // 低8位
    port_write_byte((addr >> 8) & 0xFF, PC104_ADDR_PORT + 1);  // 高8位
    
    // 发送读命令
    port_write_byte(PC104_CMD_READ, PC104_CMD_PORT);
    
    // 等待操作完成
    if (pc104_wait_ready() != 0) {
        return -1;
    }
    
    // 检查是否有错误
    if (pc104_check_error() != 0) {
        return -1;
    }
    
    // 读取数据
    data = port_read_byte(PC104_DATA_PORT);
    
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
    port_write_byte(addr & 0xFF, PC104_ADDR_PORT);        // 低8位
    port_write_byte((addr >> 8) & 0xFF, PC104_ADDR_PORT + 1);  // 高8位
    
    // 写入数据
    port_write_byte(data, PC104_DATA_PORT);
    
    // 发送写命令
    port_write_byte(PC104_CMD_WRITE, PC104_CMD_PORT);
    
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