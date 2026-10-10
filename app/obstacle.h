#ifndef __OBSTACLE_H
#define __OBSTACLE_H

#include "stm32f4xx.h"

#define ULTRA_SAFE_DIST 15 //超声波避障安全距离，单位cm

void Red_GPIO(void);//红外避障引脚初始化（PC0左/PC1右/PC2中，浮空输入）
void Ultra_GPIO(void);//超声波引脚初始化（左PB0/PB1，右PB12/PB13；TIM6微秒计时）
uint16_t Ultra_GetDistance(uint8_t side);//超声波测距：side 0=左超声 1=右超声，返回距离(cm)，无回波返回999

#endif
