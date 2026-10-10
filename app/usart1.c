#include "usart1.h"
#include <stdio.h>

static uint32_t temp1;//串口1接收到的字节（接收中断内回显用）

//====================printf输出重定向到USART1====================
#pragma import(__use_no_semihosting_swi)
struct __FILE { int handle;}; //重定向时必须自己定义
FILE __stdout;
FILE __stdin;

int fputc(int ch,FILE *f)
{
	while(USART_GetFlagStatus(USART1,USART_FLAG_TC)==RESET);
	USART_SendData(USART1,ch);
	return ch;
}

void _sys_exit(int return_code)
{
	label: goto label;
}

//====================串口1初始化（PA9/PA10，115200，收发中断）====================
void Usart_1(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA,ENABLE);
	//PA9/PA10复用映射为USART1
	GPIO_PinAFConfig(GPIOA,GPIO_PinSource9,GPIO_AF_USART1);
	GPIO_PinAFConfig(GPIOA,GPIO_PinSource10,GPIO_AF_USART1);
	//优先级分组2
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	//配置GPIO
	GPIO_InitStructure.GPIO_Pin=GPIO_Pin_9 | GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed=GPIO_High_Speed;
	GPIO_InitStructure.GPIO_OType=GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd=GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOA,&GPIO_InitStructure);
	//配置USART1
	USART_InitStructure.USART_BaudRate=115200;
	USART_InitStructure.USART_WordLength=USART_WordLength_8b;
	USART_InitStructure.USART_StopBits=USART_StopBits_1;
	USART_InitStructure.USART_Parity=USART_Parity_No;
	USART_InitStructure.USART_Mode=USART_Mode_Rx | USART_Mode_Tx;
	USART_InitStructure.USART_HardwareFlowControl=USART_HardwareFlowControl_None;
	USART_Init(USART1,&USART_InitStructure);
	//配置NVIC
	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0;//抢占优先级
	NVIC_InitStructure.NVIC_IRQChannelSubPriority=0;//响应优先级
	NVIC_InitStructure.NVIC_IRQChannelCmd=ENABLE;//使能
	NVIC_Init(&NVIC_InitStructure);
	//串口使能
	USART_Cmd(USART1,ENABLE);
}

void USART1_IRQHandler(void)
{
	if(USART_GetITStatus(USART1,USART_IT_RXNE)==SET)
	{
		temp1=USART_ReceiveData(USART1);
		while(USART_GetFlagStatus(USART1,USART_FLAG_TC)!=SET);//等待发送完成
		USART_SendData(USART1,temp1);//收到什么回发什么（串口助手回显）
		USART_ClearITPendingBit(USART1,USART_FLAG_RXNE);
	}
}
