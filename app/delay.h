#ifndef __DELAY_H
#define __DELAY_H

#include "stm32f4xx.h"

void delay_ms(uint32_t ms);//SysTick毫秒级阻塞延时（168MHz）
void delay_us(uint32_t us);//SysTick微秒级阻塞延时（168MHz）

#endif
