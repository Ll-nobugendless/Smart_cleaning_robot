/**
  ******************************************************************************
  * @file    ax_timer_int.c
  * @author  Musk Han @ XTARK
  * @version V1.0
  * @date    2018-01-01
  * @brief   X-CTR100 ��ʱ����������ж��ļ�
  *
  ******************************************************************************
  * @xtark  ʵ��ƽ̨�����˳�Ʒ X-CTR100 ������
  *         ���˹���������http://www.xtark.cn
  * 
  ******************************************************************************
  */  

#include "ax_tim.h" 

//�ж�ѭ��״̬���Ʊ�־λ
uint8_t ax_flag_tim = 0;

/**
  * @��  ��  TIM��ʱ����ʼ��������жϣ�
  * @��  ��  cnt_us �����������ֵ����λ1us����Χ0-65535 
  * @����ֵ  ��
  */
void AX_TIM_Init(uint16_t cnt_us)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	NVIC_InitTypeDef NVIC_InitStructure; 

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM7, ENABLE); //ʱ��ʹ��
	
  // �ۼ� TIM_Period�������һ�����»����ж�
	TIM_TimeBaseInitStructure.TIM_Period = cnt_us-1;  //�Զ���װ��ֵ�����65535
	// �趨��ʱ��Ƶ��Ϊ=TIMxCLK/(TIM_Prescaler+1)=1000000Hz
	TIM_TimeBaseInitStructure.TIM_Prescaler = 84-1; //��ʱ����Ƶ,�������ڣ�period x 1us��
	TIM_TimeBaseInitStructure.TIM_CounterMode=TIM_CounterMode_Up; //���ϼ���ģʽ
	TIM_TimeBaseInitStructure.TIM_ClockDivision=TIM_CKD_DIV1;
	
	TIM_TimeBaseInit(TIM7, &TIM_TimeBaseInitStructure); //����ָ���Ĳ�����ʼ��TIMx��ʱ�������λ
 
	TIM_ITConfig(TIM7,TIM_IT_Update,ENABLE ); //ʹ��ָ����TIM�ж�,���������ж�

	//�ж����ȼ�NVIC����
	NVIC_InitStructure.NVIC_IRQChannel = TIM7_IRQn;  //TIM�ж�
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;  //��ռ���ȼ�0��
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;  //�����ȼ�3��
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; //IRQͨ����ʹ��
	NVIC_Init(&NVIC_InitStructure);  //��ʼ��NVIC�Ĵ���

	TIM_Cmd(TIM7, ENABLE);  //ʹ��TIMx					 
}


/**
  * @��  ��  TIM ��ʱ�������ر�
  * @��  ��  NewState  This parameter can be: ENABLE or DISABLE.
  * @����ֵ  ��
  */
void AX_TIM_Cmd(FunctionalState NewState)
{
	TIM_SetCounter(TIM7, 0);
	TIM_Cmd(TIM7, NewState);
}

/**
  * @��  ��  TIM �жϴ�������
  * @��  ��  ��
  * @����ֵ  ��
  */
void  TIM7_IRQHandler(void)
{
	if ( TIM_GetITStatus( TIM7, TIM_IT_Update) != RESET ) 
	{	
		//�жϴ�������
		ax_flag_tim = 1;  //��λ10ms��־λ
		
		TIM_ClearITPendingBit(TIM7 , TIM_IT_Update);  		 
	}		 	
}

/**
  * @brief  ����Ƿ�����ж�
  * @param  None
  * @retval None
  */
uint8_t AX_TIM_CheckIrqStatus(void)
{
	//ȷ���ж�,�����������
	if(ax_flag_tim != 0) 
	{
		ax_flag_tim = 0;
		return 1;
	}
	else
	{
		return 0;
	}
}


/******************* (C) ��Ȩ 2018 XTARK **************************************/
