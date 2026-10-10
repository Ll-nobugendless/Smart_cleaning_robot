#ifndef __DHT11_APP_H
#define __DHT11_APP_H

#include "stm32f4xx.h"

void DHT11_Init(void);//DHT11初始化（数据线PB8，空闲高电平）
int8_t DHT11_ReadData(uint8_t *dht_data);//读取温湿度5字节：0成功，负数失败
uint8_t DHT11_Display(void);//OLED循环显示温湿度：收到'7'返回0，收到'8'返回1

#endif
