/**
	 __  _______  _    ____  _  __      __  __    ____   ___  _____ _____ 
	 \ \/ /_   _|/ \  |  _ \| |/ /     \ \/ /   / ___| / _ \|  ___|_   _|
	  \  /  | | / _ \ | |_) | ' /       \  /____\___ \| | | | |_    | |
	  /  \  | |/ ___ \|  _ <| . \       /  \_____|__) | |_| |  _|   | |
	 /_/\_\ |_/_/   \_\_| \_\_|\_\     /_/\_\   |____/ \___/|_|     |_|
  ****************************************************************************** 
  *           
  * Copyright: XTARK@塔克创新 All rights reserved
  * Website: www.xtark.cn
  * Taobao: https://shop246676508.taobao.com  
  * Blog: www.cnblogs.com/xtark
  * WeChat: 公众号：塔克创新
  *      
  ******************************************************************************
  * @author  Musk Han@XTARK
  * @version V1.0
  * @date    2019-8-2
  * @brief   Motor PWM control function
  *
  ******************************************************************************
  * @note    Adapted for STM32F407VET6 @ 168MHz, APB1 timer clock = 84MHz
  *          左轮: PWM PA3 (TIM5_CH4)，方向控制 PA2/PA4
  *          右轮: PWM PD12 (TIM4_CH1)，方向控制 PD14/PD15（右轮线序反接，方向逻辑已软件交换）
  *          PWM freq = 84MHz/4/2000 = 10.5KHz
  *
  ******************************************************************************
  */

#include "ax_motor.h" 

/**
  * @brief  右轮电机初始化
  * @note   PWM 输出 PA3 (TIM5_CH4, AF2)，方向控制 PA2/PA4
  *         定时器时钟 84MHz，PWM频率 = 84MHz/4/2000 = 10.5KHz
  * @param  None
  * @retval None
  */
void AX_MOTOR_Init(void)
{  
	GPIO_InitTypeDef GPIO_InitStructure;
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	TIM_OCInitTypeDef  TIM_OCInitStructure;
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5, ENABLE);   // Enable TIM5 clock
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);  // Enable GPIOA clock

	// Configure PA3 as alternate function for TIM5_CH4 PWM output
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;              // TIM5_CH4 -> PA3
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;           // Alternate function
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;         // Push-pull
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	// Map PA3 to TIM5 alternate function (AF2)
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource3, GPIO_AF_TIM5);

	// 右轮方向控制引脚 PA2/PA4（推挽输出）
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_4;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;          // Output mode
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	// Initialize TIM5 time base (TIM5 clock = 84MHz on STM32F407)
	TIM_TimeBaseStructure.TIM_Period = 2000 - 1;            // Auto-reload value (2000 levels)
	TIM_TimeBaseStructure.TIM_Prescaler = 4 - 1;           // Prescaler: 84MHz/4 = 21MHz
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM5, &TIM_TimeBaseStructure);

	// Initialize TIM5 Channel4 PWM mode (PA3 = TIM5_CH4)
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;       // PWM mode 1
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = 0;                      // Initial duty = 0
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OC4Init(TIM5, &TIM_OCInitStructure);                // Channel 4
	TIM_OC4PreloadConfig(TIM5, TIM_OCPreload_Enable);      // Enable CCR4 preload

	TIM_Cmd(TIM5, ENABLE);  // Enable TIM5
}

/**
  * @brief  右轮电机初始化
  * @note   PWM 输出 PD12 (TIM4_CH1, AF2)，方向控制 PD14/PD15
  *         右轮电机线序反接，方向逻辑在 SetSpeed_A 中做软件交换
  *         定时器时钟 84MHz，PWM频率 = 84MHz/4/2000 = 10.5KHz
  * @param  None
  * @retval None
  */
void AX_MOTOR_Init_A(void)
{  
	GPIO_InitTypeDef GPIO_InitStructure;
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	TIM_OCInitTypeDef  TIM_OCInitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);   // Enable TIM4 clock
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);  // Enable GPIOD clock

	// Configure PD12 as alternate function for TIM4_CH1 PWM output
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;              // TIM4_CH1 -> PD12
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;           // Alternate function
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;         // Push-pull
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOD, &GPIO_InitStructure);

	// Map PD12 to TIM4 alternate function (AF2)
	GPIO_PinAFConfig(GPIOD, GPIO_PinSource12, GPIO_AF_TIM4);

	// 左轮方向控制引脚 PD14/PD15（推挽输出）
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14 | GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;          // Output mode
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOD, &GPIO_InitStructure);



	// Initialize TIM4 time base (TIM4 clock = 84MHz on STM32F407)
	TIM_TimeBaseStructure.TIM_Period = 2000 - 1;            // Auto-reload value (2000 levels)
	TIM_TimeBaseStructure.TIM_Prescaler = 4 - 1;           // Prescaler: 84MHz/4 = 21MHz
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);

	// Initialize TIM4 Channel1 PWM mode (PD12 = TIM4_CH1)
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;       // PWM mode 1
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = 0;                      // Initial duty = 0
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OC1Init(TIM4, &TIM_OCInitStructure);                // Channel 1
	TIM_OC1PreloadConfig(TIM4, TIM_OCPreload_Enable);      // Enable CCR1 preload

	TIM_Cmd(TIM4, ENABLE);  // Enable TIM4
}

/**
  * @brief  左轮电机速度控制
  * @param  speed: 电机速度，范围 -2000~2000
  *                正数：前进，负数：后退，0：停止
  * @retval None
  */
void AX_MOTOR_SetSpeed(int16_t speed)
{
	uint16_t temp;

	if(speed > 0)
	{
		//前进
		GPIO_ResetBits(GPIOA, GPIO_Pin_2);
		GPIO_SetBits(GPIOA, GPIO_Pin_4);
		temp = speed;
	}
	else if(speed < 0)
	{
		//后退
		GPIO_ResetBits(GPIOA, GPIO_Pin_4);
		GPIO_SetBits(GPIOA, GPIO_Pin_2);
		temp = (-speed);
	}
	else
	{
		//停止：两个方向引脚均拉低
		GPIO_ResetBits(GPIOA, GPIO_Pin_2);
		GPIO_ResetBits(GPIOA, GPIO_Pin_4);
		temp = 0;
	}

	if(temp > 2000)
		temp = 2000;

	TIM_SetCompare4(TIM5, temp);  // Set CCR4 (TIM5_CH4, PA3)
}


/**
  * @brief  右轮电机速度控制
  * @param  speed: 电机速度，范围 -2000~2000
  *                正数：前进，负数：后退，0：停止
  * @note   右轮实际转向与指令相反（电机线序/H桥输入反接），
  *         故在此交换 PD14/PD15 方向逻辑做软件校正，保证正数=车体前进
  * @retval None
  */
void AX_MOTOR_SetSpeed_A(int16_t speed)
{
	uint16_t temp;

	if(speed > 0)
	{
		//前进：交换方向引脚电平（右轮反转校正）
		GPIO_SetBits(GPIOD, GPIO_Pin_14);
		GPIO_ResetBits(GPIOD, GPIO_Pin_15);
		temp = speed;
	}
	else if(speed < 0)
	{
		//后退：交换方向引脚电平（右轮反转校正）
		GPIO_SetBits(GPIOD, GPIO_Pin_15);
		GPIO_ResetBits(GPIOD, GPIO_Pin_14);
		temp = (-speed);
	}
	else
	{
		//停止：两个方向引脚均拉低
		GPIO_ResetBits(GPIOD, GPIO_Pin_14);
		GPIO_ResetBits(GPIOD, GPIO_Pin_15);
		temp = 0;
	}

	if(temp > 2000)
		temp = 2000;

	TIM_SetCompare1(TIM4, temp);  // Set CCR1 (TIM4_CH1, PD12)
}

/******************* (C) Copyright 2019 XTARK **************************************/
