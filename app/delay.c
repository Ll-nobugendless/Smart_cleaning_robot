#include "delay.h"

//====================SysTick阻塞延时（系统时钟168MHz）====================
void delay_ms(uint32_t ms)
{
	while(ms--)
	{
		SysTick->CTRL = 0; 			//关闭系统定时器
		SysTick->LOAD = 168000-1; 		//装载计数值，1ms定时时间
		SysTick->VAL = 0; 			//清空当前值，清除计数标志位
		SysTick->CTRL = 5; 			//使能系统定时器，时钟源为系统时钟168MHz
		while ((SysTick->CTRL & (1<<16))==0);	//等待系统定时器计时完成
		SysTick->CTRL = 0; 			//关闭系统定时器
	}
}

void delay_us(uint32_t us)
{
	SysTick->CTRL = 0; 			//关闭系统定时器
	SysTick->LOAD = 168*us-1; 		//装载计数值，us级定时时间
	SysTick->VAL = 0; 			//清空当前值，清除计数标志位
	SysTick->CTRL = 5; 			//使能系统定时器，时钟源为系统时钟168MHz
	while ((SysTick->CTRL & (1<<16))==0);	//等待系统定时器计时完成
	SysTick->CTRL = 0;
}
