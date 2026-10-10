#ifndef __BLUETOOTH_H
#define __BLUETOOTH_H

#include "stm32f4xx.h"

extern volatile uint8_t temp3;//蓝牙最新指令字节（'1'自动/'2'手动/'3'~'7'运动/'A'开刷/'B'关刷/'C'温湿度/'8'退出）

void Usart_3(uint32_t baud);//蓝牙模块初始化：USART3，PB10(TX)/PB11(RX)

#endif
