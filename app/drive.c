#include "drive.h"
#include "ax_motor.h"
#include "ax_encoder.h"

//====================小车运动控制（PID闭环，编码器反馈保证两轮等速直行）====================
//直行时PID实时调节左右轮PWM保证转速一致；转向时固定30% PWM开环。
//PID在TIM1中断中每10ms执行一次，PID_GO仅设目标方向，实际PWM由中断算出。

#define BASE_PWM      600    //直行基准PWM：PWM周期2000，30%占空比=600
#define BOOST_PWM      50    //自动模式PWM增益：左右轮基准同时+50
#define L_OFFSET       75    //左轮相对右轮的固定偏置：左轮PWM=右轮PWM-75
#define PID_MAX_CORR  150    //PID修正量上限±150（右轮PWM范围450~750，22.5%~37.5%）

//PWM基准（自动模式下通过Drive_SetBoost(1)提升为BASE_PWM+BOOST_PWM）
static volatile int16_t base_pwm=BASE_PWM;

//PID参数（需实测标定）：差速PID，diff=左编码器增量-右编码器增量
#define PID_KP   10          //比例增益：每1计数差→10 PWM修正
#define PID_KI   2           //积分增益：消除稳态速度差
#define PID_KD   5           //微分增益：抑制速度差突变

//编码器方向校正：差分安装可能导致右编码器正转计数方向相反
//若PID使跑偏加重，将ENC_DIR_R取反即可
#define ENC_DIR_L   1        //左编码器方向校正
#define ENC_DIR_R   1        //右编码器方向校正

//目标方向符号（正=前进、负=后退、0=停）
static volatile int16_t tgt_l=0;
static volatile int16_t tgt_r=0;

//PID状态
static volatile int32_t pid_integral=0;
static volatile int32_t pid_last_diff=0;
static volatile int16_t pid_correction=0;

//编码器上次计数值
static volatile uint16_t last_enc_l=30000;
static volatile uint16_t last_enc_r=30000;

//实时遥测：左右轮实际PWM + 编码器增量
static volatile int16_t dbg_pwm_l=0,dbg_pwm_r=0;
static volatile int16_t dbg_enc_l=0,dbg_enc_r=0;

//TIM1初始化：10ms周期PID中断（TIM1时钟168MHz）
static void PID_Timer_Init(void)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1,ENABLE);

	//168MHz / 8400 / 200 = 100Hz = 10ms
	TIM_TimeBaseStructure.TIM_Period=200-1;
	TIM_TimeBaseStructure.TIM_Prescaler=8400-1;
	TIM_TimeBaseStructure.TIM_ClockDivision=TIM_CKD_DIV1;
	TIM_TimeBaseStructure.TIM_CounterMode=TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM1,&TIM_TimeBaseStructure);

	TIM_ITConfig(TIM1,TIM_IT_Update,ENABLE);

	NVIC_InitStructure.NVIC_IRQChannel=TIM1_UP_TIM10_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=2;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority=0;
	NVIC_InitStructure.NVIC_IRQChannelCmd=ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	TIM_Cmd(TIM1,ENABLE);
}

void PID_Init(void)
{
	tgt_l=0; tgt_r=0;
	pid_integral=0; pid_last_diff=0; pid_correction=0;
	dbg_pwm_l=0; dbg_pwm_r=0;
	last_enc_l=AX_ENCODER_GetCounter();
	last_enc_r=AX_ENCODER_GetCounter_A();
	AX_MOTOR_SetSpeed(0);
	AX_MOTOR_SetSpeed_A(0);
	PID_Timer_Init();
}

void PID_GO(int16_t target_l,int16_t target_r)//左右轮方向符号：正=前进、负=后退、0=停
{
	tgt_l=target_l;
	tgt_r=target_r;
	//直行（两轮同号同幅）时重置PID状态，从干净状态开始闭环
	if(target_l==target_r && target_l!=0)
	{
		pid_integral=0;
		pid_last_diff=0;
		last_enc_l=AX_ENCODER_GetCounter();
		last_enc_r=AX_ENCODER_GetCounter_A();
	}
}

void PID_STOP(void)//停车：清除目标，PWM立即置0
{
	tgt_l=0; tgt_r=0;
	pid_integral=0; pid_last_diff=0; pid_correction=0;
	dbg_pwm_l=0; dbg_pwm_r=0;
	AX_MOTOR_SetSpeed(0);
	AX_MOTOR_SetSpeed_A(0);
}

void Drive_SetBoost(uint8_t on)//on=1自动模式基准+40；on=0恢复默认30%
{
	base_pwm=on?(BASE_PWM+BOOST_PWM):BASE_PWM;
}

//TIM1中断：每10ms执行PID计算并更新PWM
void TIM1_UP_TIM10_IRQHandler(void)
{
	uint16_t cur_l,cur_r;
	int32_t raw_l,raw_r;
	int16_t delta_l,delta_r;

	if(TIM_GetITStatus(TIM1,TIM_IT_Update)!=RESET)
	{
		TIM_ClearITPendingBit(TIM1,TIM_IT_Update);

		//读取编码器增量（处理60000周期回绕）
		cur_l=AX_ENCODER_GetCounter();
		cur_r=AX_ENCODER_GetCounter_A();
		raw_l=(int32_t)cur_l-(int32_t)last_enc_l;
		raw_r=(int32_t)cur_r-(int32_t)last_enc_r;
		if(raw_l>30000) raw_l-=60000;
		else if(raw_l<-30000) raw_l+=60000;
		if(raw_r>30000) raw_r-=60000;
		else if(raw_r<-30000) raw_r+=60000;
		last_enc_l=cur_l;
		last_enc_r=cur_r;

		//方向校正后的实际增量
		delta_l=(int16_t)(ENC_DIR_L*raw_l);
		delta_r=(int16_t)(ENC_DIR_R*raw_r);
		dbg_enc_l=delta_l;
		dbg_enc_r=delta_r;

		if(tgt_l==tgt_r && tgt_l!=0)
		{
			//直行：差速PID，保证两轮等速
			int16_t diff=delta_l-delta_r;
			int16_t derivative=diff-pid_last_diff;
			int32_t corr=PID_KP*diff+PID_KI*pid_integral+PID_KD*derivative;
			int16_t pwm_l,pwm_r;
			diff=delta_l-delta_r;  //>0=左轮快
			pid_integral+=diff;
			if(pid_integral>PID_MAX_CORR) pid_integral=PID_MAX_CORR;
			if(pid_integral<-PID_MAX_CORR) pid_integral=-PID_MAX_CORR;
			derivative=diff-pid_last_diff;
			pid_last_diff=diff;
			corr=PID_KP*diff+PID_KI*pid_integral+PID_KD*derivative;
			if(corr>PID_MAX_CORR) corr=PID_MAX_CORR;
			if(corr<-PID_MAX_CORR) corr=-PID_MAX_CORR;
			pid_correction=(int16_t)corr;

			//右轮=基准+PID修正，左轮恒=右轮-L_OFFSET（保证左比右少75）
			pwm_r=base_pwm+pid_correction;
			pwm_l=pwm_r-L_OFFSET;
			if(tgt_l>0){dbg_pwm_l=pwm_l;dbg_pwm_r=pwm_r;}
			else{dbg_pwm_l=-pwm_l;dbg_pwm_r=-pwm_r;}
			AX_MOTOR_SetSpeed(dbg_pwm_l);
			AX_MOTOR_SetSpeed_A(dbg_pwm_r);
		}
		else if(tgt_l!=0||tgt_r!=0)
		{
			//转向：固定PWM开环，不做PID
			pid_correction=0;
			if(tgt_l>0) dbg_pwm_l=base_pwm;
			else if(tgt_l<0) dbg_pwm_l=-base_pwm;
			else dbg_pwm_l=0;
			if(tgt_r>0) dbg_pwm_r=base_pwm;
			else if(tgt_r<0) dbg_pwm_r=-base_pwm;
			else dbg_pwm_r=0;
			AX_MOTOR_SetSpeed(dbg_pwm_l);
			AX_MOTOR_SetSpeed_A(dbg_pwm_r);
		}
		else
		{
			//停止
			dbg_pwm_l=0;
			dbg_pwm_r=0;
		}
	}
}

//====================蓝牙串口(USART3)遥测====================
static void dbg_u32(int32_t v)
{
	char buf[8];
	uint8_t i=0,n;
	if(v<0){USART_SendData(USART3,'-');while(USART_GetFlagStatus(USART3,USART_FLAG_TXE)==RESET);v=-v;}
	if(v==0) buf[i++]='0';
	while(v>0 && i<7){buf[i++]=(char)('0'+v%10);v/=10;}
	for(n=i;n>0;n--){USART_SendData(USART3,buf[n-1]);while(USART_GetFlagStatus(USART3,USART_FLAG_TXE)==RESET);}
}

//遥测格式：=左PWM,右PWM,左编码增量,右编码增量
void Drive_DebugReport(void)
{
	USART_SendData(USART3,'=');while(USART_GetFlagStatus(USART3,USART_FLAG_TXE)==RESET);
	dbg_u32(dbg_pwm_l);USART_SendData(USART3,',');while(USART_GetFlagStatus(USART3,USART_FLAG_TXE)==RESET);
	dbg_u32(dbg_pwm_r);USART_SendData(USART3,',');while(USART_GetFlagStatus(USART3,USART_FLAG_TXE)==RESET);
	dbg_u32(dbg_enc_l);USART_SendData(USART3,',');while(USART_GetFlagStatus(USART3,USART_FLAG_TXE)==RESET);
	dbg_u32(dbg_enc_r);USART_SendData(USART3,'\r');while(USART_GetFlagStatus(USART3,USART_FLAG_TXE)==RESET);
	USART_SendData(USART3,'\n');while(USART_GetFlagStatus(USART3,USART_FLAG_TXE)==RESET);
}
