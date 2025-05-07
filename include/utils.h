#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>

#ifdef __linux__
    #define PLATFORM_LINUX

    #include <sys/ioctl.h>
    #include <linux/types.h>

    #include <linux/kernel.h>
    #include <linux/module.h>

    #ifdef __KERNEL__
        #include <linux/init.h>

        #include <linux/fs.h>
        #include <linux/device.h> 

        #include <linux/cdev.h>

        #include <linux/slab.h>
        #include <asm/uaccess.h>
        #include <linux/uaccess.h>

        # include <asm/io.h>
        #include <linux/io.h>
        #include <linux/ioport.h>

        #include <linux/interrupt.h>

        #include <linux/time.h>
        #include <linux/timer.h>
        #include <linux/jiffies.h>

        #include <linux/isa.h>
    #endif

    #ifdef __MODULE__

    #endif
#elif defined(_WIN32) || defined(_WIN64)
    #define PLATFORM_WINDOWS

    #include <windows.h>
#else
    #define PLATFORM_UNKNOWN
#endif

#endif