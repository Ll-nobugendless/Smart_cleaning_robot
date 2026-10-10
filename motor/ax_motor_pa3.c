/**
	 __  _______  _    ____  _  __      __  __    ____   ___  _____ _____
	 \ \/ /_   _|/ \  |  _ \| |/ /     \ \/ /   / ___| / _ \|  ___|_   _|
	  \  /  | | / _ \ | |_) | ' /       \  /____\___ \| | | | |_    | |
	  /  \  | |/ ___ \|  _ <| . \       /  \_____|__) | |_| |  _|   | |
	 /_/\_\ |_/_/   \_\_| \_\_|\_\     /_/\_\   |____/ \___/|_|     |_|
  ******************************************************************************
  *           Copyright: XTARK All rights reserved
  ******************************************************************************
  * @author  Musk Han@XTARK
  * @version V1.2
  * @date    2026-9-1
  * @brief   Motor PWM control - STM32F407VET6 @ 168MHz, PA3 (TIM5_CH4)
  *
  ******************************************************************************
  * @note    Uses TIM5 (NOT TIM2) to avoid conflict with encoder (TIM2).
  *          - MCU:       STM32F407VET6 @ 168MHz
  *          - PWM pin:   PA3 = TIM5_CH4  (AF2, NOT AF1=TIM2)
  *          - DIR pins:  PA12 (fwd), PA11 (rev)
  *          - TIM5 clock = 84MHz (APB1 timer clock)
  *          - PWM freq  = 84MHz / 4 / 2000 = 10.5 KHz
  *          - Speed range: -2000 ~ +2000
  *
  *          IMPORTANT: Only compile ONE of ax_motor.c / ax_motor_pa3.c
  *                     in the same Keil project to avoid duplicate symbols.
  *                     The header ax_motor.h is shared and unchanged.
  ******************************************************************************
  */

#include "ax_motor.h"

/**
  * @brief  Motor PWM control initialization
  *         Uses TIM5_CH4 on PA3 (AF2) to avoid TIM2 conflict with encoder.
  *         PWM frequency: 84MHz / 4 / 2000 = 10.5 KHz
  * @param  None
  * @retval None
  */
void AX_MOTOR_Init(void)
{
	GPIO_InitTypeDef        GPIO_InitStructure;
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	TIM_OCInitTypeDef       TIM_OCInitStructure;

	/* Enable TIM5 and GPIOA clocks */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

	/* --- PWM output pin: PA3 = TIM5_CH4 (AF2, NOT TIM2!) -------- */
	GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/* Map PA3 to TIM5 alternate function (AF2) */
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource3, GPIO_AF_TIM5);

	/* --- Direction control pins: PA12, PA11 ---------------------- */
	GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_12 | GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/* --- TIM5 time base (TIM5 clock = 84 MHz) ------------------- */
	TIM_TimeBaseStructure.TIM_Period        = 2000 - 1;    /* ARR */
	TIM_TimeBaseStructure.TIM_Prescaler     = 4 - 1;        /* PSC: 84/4=21MHz */
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode   = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM5, &TIM_TimeBaseStructure);

	/* --- TIM5 Channel 4 PWM (PA3 = TIM5_CH4) -------------------- */
	TIM_OCInitStructure.TIM_OCMode      = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse      = 0;                  /* start at 0% duty */
	TIM_OCInitStructure.TIM_OCPolarity  = TIM_OCPolarity_High;
	TIM_OC4Init(TIM5, &TIM_OCInitStructure);                 /* Channel 4 */
	TIM_OC4PreloadConfig(TIM5, TIM_OCPreload_Enable);        /* CCR4 preload */

	TIM_Cmd(TIM5, ENABLE);                                    /* Enable TIM5 */
}

/**
  * @brief  Motor PWM speed control
  * @param  speed: -2000 ~ +2000
  *                > 0 : forward (PA12 high, PA11 low)
  *                < 0 : reverse (PA12 low,  PA11 high)
  *                = 0 : stop    (both low)
  * @retval None
  */
void AX_MOTOR_SetSpeed(int16_t speed)
{
	uint16_t temp;

	if (speed > 0)
	{
		GPIO_ResetBits(GPIOA, GPIO_Pin_11);
		GPIO_SetBits  (GPIOA, GPIO_Pin_12);
		temp = (uint16_t)speed;
	}
	else if (speed < 0)
	{
		GPIO_ResetBits(GPIOA, GPIO_Pin_12);
		GPIO_SetBits  (GPIOA, GPIO_Pin_11);
		temp = (uint16_t)(-speed);
	}
	else
	{
		GPIO_ResetBits(GPIOA, GPIO_Pin_12);
		GPIO_ResetBits(GPIOA, GPIO_Pin_11);
		temp = 0;
	}

	if (temp > 2000)
		temp = 2000;

	TIM_SetCompare4(TIM5, temp);   /* CCR4 (TIM5_CH4 = PA3) */
}

/******************* (C) Copyright 2019 XTARK **************************************/
