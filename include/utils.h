#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __linux__
    #define PLATFORM_LINUX

    #ifndef __KERNEL__
        #include <unistd.h>
        #include <fcntl.h>
        #include <sys/ioctl.h>
        #include <sys/mman.h>
        #include <sys/io.h>
        #include <pthread.h>
    #endif

    #ifdef __KERNEL__
        #include <linux/init.h>
        #include <linux/kernel.h>
        #include <linux/module.h>
        #include <linux/fs.h>
        #include <linux/device.h>
        #include <linux/cdev.h>
        #include <linux/slab.h>
        #include <linux/uaccess.h>
        #include <linux/ioport.h>
        #include <linux/interrupt.h>
        #include <linux/time.h>
        #include <linux/timer.h>
        #include <linux/jiffies.h>
        #include <linux/isa.h>
        #include <asm/io.h>
    #endif

#elif defined(_WIN32) || defined(_WIN64)
    #define PLATFORM_WINDOWS
    #include <windows.h>
    #include <pthread.h>
    #include <pthread_time.h>
#else
    #define PLATFORM_UNKNOWN
#endif

#endif