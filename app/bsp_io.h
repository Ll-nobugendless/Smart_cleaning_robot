#ifndef __BSP_IO_H
#define __BSP_IO_H

#include "stm32f4xx.h"

//====================GPIO位带操作宏（直接读写单个引脚，等价于51的sbit）====================
#define PBin(x)	    *(volatile uint32_t *)(0x42000000 + (GPIOB_BASE + 0x10 - 0x40000000)*32 + x*4)
#define PBout(x)	*(volatile uint32_t *)(0x42000000 + (GPIOB_BASE + 0x14 - 0x40000000)*32 + x*4)
#define PCin(x)	    *(volatile uint32_t *)(0x42000000 + (GPIOC_BASE + 0x10 - 0x40000000)*32 + x*4)
#define PDout(x)	*(volatile uint32_t *)(0x42000000 + (GPIOD_BASE + 0x14 - 0x40000000)*32 + x*4)

#endif
