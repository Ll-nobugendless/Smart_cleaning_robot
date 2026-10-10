#include "cleaner.h"
#include "bsp_io.h"

//====================边刷电机引脚初始化（PD0~PD3）====================
void MY_Side_brush_motor_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStruct;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);

	GPIO_InitStruct.GPIO_Pin=GPIO_Pin_0 | GPIO_Pin_1 |GPIO_Pin_2 | GPIO_Pin_3;
	GPIO_InitStruct.GPIO_Mode=GPIO_Mode_OUT;
	GPIO_InitStruct.GPIO_Speed=GPIO_High_Speed;
	GPIO_InitStruct.GPIO_OType=GPIO_OType_PP;
	GPIO_InitStruct.GPIO_PuPd=GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOD,&GPIO_InitStruct);

	GPIO_ResetBits(GPIOD,GPIO_Pin_0 | GPIO_Pin_1 |GPIO_Pin_2 | GPIO_Pin_3);//默认停止
}

//====================吸尘器引脚初始化（仿照边刷：PD4/PD5推挽输出）====================
void MY_Vacuum_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStruct;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);

	GPIO_InitStruct.GPIO_Pin=GPIO_Pin_4 | GPIO_Pin_5;
	GPIO_InitStruct.GPIO_Mode=GPIO_Mode_OUT;
	GPIO_InitStruct.GPIO_Speed=GPIO_High_Speed;
	GPIO_InitStruct.GPIO_OType=GPIO_OType_PP;
	GPIO_InitStruct.GPIO_PuPd=GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOD,&GPIO_InitStruct);

	GPIO_ResetBits(GPIOD,GPIO_Pin_4 | GPIO_Pin_5);//默认停止
}

//边刷与吸尘器绑定：同时开启（驱动电平与边刷原有逻辑一致）
void Cleaner_Start(void)
{
	PDout(0)=1;PDout(1)=0;PDout(2)=0;PDout(3)=1;
	PDout(4)=1;PDout(5)=0;
}

//边刷与吸尘器绑定：同时停止
void Cleaner_Stop(void)
{
	PDout(0)=0;PDout(1)=0;PDout(2)=0;PDout(3)=0;
	PDout(4)=0;PDout(5)=0;
}
