#include "stm32f4xx.h"  // Device header
#include "stdio.h"
#include "ax_encoder.h"
#include "ax_motor.h"
#include "oled.h"
#include "delay.h"
#include "usart1.h"
#include "bluetooth.h"
#include "obstacle.h"
#include "dht11.h"
#include "cleaner.h"
#include "drive.h"
#include "auto_mode.h"
#include "manual_mode.h"

//====================清洁小车主程序====================
//功能函数已按模块封装到 app/ 目录：
//  delay      SysTick延时
//  usart1     串口1（printf重定向）
//  bluetooth  蓝牙USART3及指令temp3
//  obstacle   红外/超声波避障
//  dht11      温湿度采集与显示
//  cleaner    边刷电机+吸尘器
//  drive      轮胎电机运动控制（PID闭环，编码器反馈保证两轮等速直行）
//  auto_mode  自动碰障回弹式弓字清扫
//  manual_mode 手动蓝牙操控
int main()
{
	Usart_1();
	Red_GPIO();//红外避障初始化引脚
	Ultra_GPIO();//超声波引脚初始化
	MY_Side_brush_motor_Init();//边刷电机初始化配置
	MY_Vacuum_Init();//吸尘器初始化配置（PD4/PD5）
	DHT11_Init();//DHT11温湿度模块初始化（数据线PB8）
	OLED_Init();//OLED初始化
	Usart_3(115200);//蓝牙模块初始化
	OLED_Clear();

	AX_ENCODER_Init(60000);
	AX_ENCODER_Init_A(60000);
	AX_ENCODER_SetCounter(30000);
	AX_ENCODER_SetCounter_A(30000);
	printf("success\r\n");

	AX_MOTOR_Init();
	AX_MOTOR_Init_A();
	AX_MOTOR_SetSpeed(0);//右轮
	AX_MOTOR_SetSpeed_A(0);//左轮
	PID_Init();//运动控制初始化（PID闭环，编码器/电机就绪后启动TIM1中断）

	while (1)
	{
		if(temp3=='1') Auto_Mode();//自动模式：碰障回弹式弓字清扫
		if(temp3=='2') Manual_Mode();//手动操控模式
	}
}
