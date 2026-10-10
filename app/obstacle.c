#include "obstacle.h"
#include "delay.h"

//====================红外避障引脚初始化（PC0/PC1/PC2 浮空输入，检测到障碍输入低电平）====================
void Red_GPIO(void)
{
	GPIO_InitTypeDef GPIO_InitStruct;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);

	GPIO_InitStruct.GPIO_Pin=GPIO_Pin_0 |GPIO_Pin_1 |GPIO_Pin_2;
	GPIO_InitStruct.GPIO_Mode=GPIO_Mode_IN;
	GPIO_InitStruct.GPIO_PuPd=GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOC,&GPIO_InitStruct);
}

//====================超声波引脚初始化（左PB0/PB1，右PB12/PB13）====================
void Ultra_GPIO(void)
{
	GPIO_InitTypeDef GPIO_InitStruct;
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStruct;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

	//Trig触发引脚：PB0左超声Trig、PB12右超声Trig，推挽输出，默认低电平
	GPIO_InitStruct.GPIO_Pin=GPIO_Pin_0 | GPIO_Pin_12;
	GPIO_InitStruct.GPIO_Mode=GPIO_Mode_OUT;
	GPIO_InitStruct.GPIO_Speed=GPIO_High_Speed;
	GPIO_InitStruct.GPIO_OType=GPIO_OType_PP;
	GPIO_InitStruct.GPIO_PuPd=GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOB,&GPIO_InitStruct);
	GPIO_ResetBits(GPIOB,GPIO_Pin_0 | GPIO_Pin_12);//Trig默认输出低电平

	//Echo回响引脚：PB1左超声Echo、PB13右超声Echo，下拉输入，空闲保持低电平
	GPIO_InitStruct.GPIO_Pin=GPIO_Pin_1 | GPIO_Pin_13;
	GPIO_InitStruct.GPIO_Mode=GPIO_Mode_IN;
	GPIO_InitStruct.GPIO_PuPd=GPIO_PuPd_DOWN;
	GPIO_Init(GPIOB,&GPIO_InitStruct);

	//TIM6作为微秒计时器（APB1定时器时钟84MHz，84分频=1MHz，即1计数=1us），用于测量Echo高电平宽度
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM6, ENABLE);
	TIM_TimeBaseStruct.TIM_Period=65535;
	TIM_TimeBaseStruct.TIM_Prescaler=84-1;
	TIM_TimeBaseStruct.TIM_CounterMode=TIM_CounterMode_Up;
	TIM_TimeBaseStruct.TIM_ClockDivision=TIM_CKD_DIV1;
	TIM_TimeBaseInit(TIM6,&TIM_TimeBaseStruct);
}

//超声波测距：side 0=左超声(PB0/PB1) 1=右超声(PB12/PB13)，返回距离cm，无回波/超量程返回999
uint16_t Ultra_GetDistance(uint8_t side)
{
	uint16_t trig_pin, echo_pin;
	uint32_t t;

	if(side == 0)
	{
		trig_pin = GPIO_Pin_0;   //左超声Trig=PB0
		echo_pin = GPIO_Pin_1;   //左超声Echo=PB1
	}
	else
	{
		trig_pin = GPIO_Pin_12;  //右超声Trig=PB12
		echo_pin = GPIO_Pin_13;  //右超声Echo=PB13
	}

	//发送Trig触发脉冲：先拉低再输出>=10us高电平
	GPIO_ResetBits(GPIOB, trig_pin);
	delay_us(2);
	GPIO_SetBits(GPIOB, trig_pin);
	delay_us(20);
	GPIO_ResetBits(GPIOB, trig_pin);

	//等待Echo上升沿，30ms超时判定为前方无障碍（防止永久阻塞）
	TIM_SetCounter(TIM6, 0);
	TIM_Cmd(TIM6, ENABLE);
	while(GPIO_ReadInputDataBit(GPIOB, echo_pin) == 0)
	{
		if(TIM_GetCounter(TIM6) > 30000)
		{
			TIM_Cmd(TIM6, DISABLE);
			return 999;
		}
	}

	//测量Echo高电平宽度（1计数=1us），30ms超时按超量程处理
	TIM_SetCounter(TIM6, 0);
	while(GPIO_ReadInputDataBit(GPIOB, echo_pin) == 1)
	{
		if(TIM_GetCounter(TIM6) > 30000) break;
	}
	t = TIM_GetCounter(TIM6);
	TIM_Cmd(TIM6, DISABLE);

	if(t >= 30000) return 999;             //无回波/超量程
	return (uint16_t)(t * 17 / 1000);      //距离cm = t(us) * 声速340m/s / 2 ≈ t*0.017
}
