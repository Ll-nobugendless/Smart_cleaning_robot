#ifndef __DRIVE_H
#define __DRIVE_H

#include "stm32f4xx.h"

#define TARGET_SPEED  100    //方向符号参数：正=前进/负=后退/0=停，幅值固定30% PWM

void PID_Init(void);//运动控制初始化（PID闭环，启动TIM1中断，上电停车）
void PID_GO(int16_t target_l,int16_t target_r);//左右轮方向符号，直行PID闭环，转向固定30%
void PID_STOP(void);//左右轮PWM立即置0
void Drive_SetBoost(uint8_t on);//on=1自动模式左右轮基准+40；on=0恢复默认30%
void Drive_DebugReport(void);//蓝牙遥测一帧：回读左右轮当前实际PWM

#endif
