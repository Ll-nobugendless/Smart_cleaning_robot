#ifndef __CLEANER_H
#define __CLEANER_H

#include "stm32f4xx.h"

void MY_Side_brush_motor_Init(void);//边刷电机引脚初始化（PD0~PD3，推挽输出，默认停止）
void MY_Vacuum_Init(void);//吸尘器引脚初始化（PD4/PD5，推挽输出，默认停止）
void Cleaner_Start(void);//边刷与吸尘器同时开启
void Cleaner_Stop(void);//边刷与吸尘器同时停止

#endif
