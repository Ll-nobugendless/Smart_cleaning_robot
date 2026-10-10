#include "bluetooth.h"

volatile uint8_t temp3=0;//蓝牙接收中断中更新的最新指令

//====================蓝牙初始化（USART3，PB10/PB11）====================
void Usart_3(uint32_t baud)
{
	//声明结构体
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	//使能时钟
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB,ENABLE);
	//PB10，PB11对应引脚复用映射为USART3
	GPIO_PinAFConfig(GPIOB,GPIO_PinSource10,GPIO_AF_USART3);
	GPIO_PinAFConfig(GPIOB,GPIO_PinSource11,GPIO_AF_USART3);
	//优先级分组2
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	//配置GPIO
	GPIO_InitStructure.GPIO_Pin=GPIO_Pin_10 | GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed=GPIO_High_Speed;
	GPIO_InitStructure.GPIO_OType=GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd=GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOB,&GPIO_InitStructure);
	//配置USART3
	USART_InitStructure.USART_BaudRate=baud;
	USART_InitStructure.USART_WordLength=USART_WordLength_8b;
	USART_InitStructure.USART_StopBits=USART_StopBits_1;
	USART_InitStructure.USART_Parity=USART_Parity_No;
	USART_InitStructure.USART_Mode=USART_Mode_Rx | USART_Mode_Tx;
	USART_InitStructure.USART_HardwareFlowControl=USART_HardwareFlowControl_None;
	USART_Init(USART3,&USART_InitStructure);
	//使能接收中断
	USART_ITConfig(USART3,USART_IT_RXNE,ENABLE);
	//配置NVIC
	NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0;//抢占优先级
	NVIC_InitStructure.NVIC_IRQChannelSubPriority=0;//响应优先级
	NVIC_InitStructure.NVIC_IRQChannelCmd=ENABLE;//使能
	NVIC_Init(&NVIC_InitStructure);
	//串口使能
	USART_Cmd(USART3,ENABLE);
}

void USART3_IRQHandler(void)
{
	//检查是否由接收数据完毕触发中断
	if(USART_GetITStatus(USART3,USART_IT_RXNE)==SET)
	{
		temp3=USART_ReceiveData(USART3);
	}

	//清空标志位
	USART_ClearITPendingBit(USART3,USART_FLAG_RXNE);
}
