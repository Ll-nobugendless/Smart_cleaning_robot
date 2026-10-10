#include "dht11.h"
#include "bsp_io.h"
#include "delay.h"
#include "drive.h"
#include "bluetooth.h"
#include "oled.h"

//====================DHT11温湿度模块（单总线数据线PB8）====================
static void DHT11_OUT_Init(void)//PB8配置为推挽输出（主机发送启动信号）
{
	GPIO_InitTypeDef GPIO_InitStruct;
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
	GPIO_InitStruct.GPIO_Pin=GPIO_Pin_8;
	GPIO_InitStruct.GPIO_Mode=GPIO_Mode_OUT;
	GPIO_InitStruct.GPIO_Speed=GPIO_High_Speed;
	GPIO_InitStruct.GPIO_OType=GPIO_OType_PP;
	GPIO_InitStruct.GPIO_PuPd=GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOB,&GPIO_InitStruct);
}

static void DHT11_IN_Init(void)//PB8配置为输入（主机接收DHT11数据）
{
	GPIO_InitTypeDef GPIO_InitStruct;
	GPIO_InitStruct.GPIO_Pin=GPIO_Pin_8;
	GPIO_InitStruct.GPIO_Mode=GPIO_Mode_IN;
	GPIO_InitStruct.GPIO_PuPd=GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOB,&GPIO_InitStruct);
}

void DHT11_Init(void)//DHT11初始化：数据线空闲保持高电平
{
	DHT11_OUT_Init();
	PBout(8)=1;
}

//读取温湿度5字节：dht_data[0]湿度整数 [1]湿度小数 [2]温度整数 [3]温度小数 [4]校验，返回0成功/负数失败
int8_t DHT11_ReadData(uint8_t *dht_data)
{
	uint32_t time_out=0;
	int32_t i,j,check_num;
	int8_t ret=0;
	uint8_t *p=dht_data, data;

	__disable_irq();//DHT11时序为微秒级，关中断防止蓝牙USART接收打断时序

	//1.主机发送启动信号：拉低>=18ms后拉高20~40us
	DHT11_OUT_Init();
	PBout(8)=0;
	delay_ms(20);
	PBout(8)=1;
	delay_us(30);

	//2.切换输入，等待DHT11响应（80us低电平+80us高电平）
	DHT11_IN_Init();
	time_out=0;
	while(PBin(8)==1)//等待响应拉低
	{
		time_out++;
		delay_us(1);
		if(time_out>4000){ret=-1;goto dht_end;}
	}
	time_out=0;
	while(PBin(8)==0)//等待响应低电平结束
	{
		time_out++;
		delay_us(1);
		if(time_out>100){ret=-2;goto dht_end;}
	}
	time_out=0;
	while(PBin(8)==1)//等待响应高电平结束
	{
		time_out++;
		delay_us(1);
		if(time_out>100){ret=-3;goto dht_end;}
	}

	//3.接收5字节（40bit），高位先出
	for(j=0;j<5;j++)
	{
		for(data=0,i=7;i>=0;i--)
		{
			time_out=0;
			while(PBin(8)==0)//先滤掉每位起始的50us低电平
			{
				time_out++;
				delay_us(1);
				if(time_out>100){ret=-4;goto dht_end;}
			}
			delay_us(40);//延时40us后仍为高电平则是数据1，否则为数据0
			if(PBin(8)==1)
			{
				data|=1<<i;
				time_out=0;
				while(PBin(8)==1)//等待数据位高电平结束
				{
					time_out++;
					delay_us(1);
					if(time_out>100){ret=-5;goto dht_end;}
				}
			}
		}
		p[j]=data;
	}
	delay_us(100);

	//4.校验：前4字节之和低8位应等于第5字节
	check_num=(p[0]+p[1]+p[2]+p[3])&0xff;
	if(check_num!=p[4]) ret=-6;

dht_end:
	__enable_irq();//恢复中断
	DHT11_OUT_Init();
	PBout(8)=1;//释放总线，恢复空闲高电平
	return ret;
}

//温湿度显示界面：停车并在OLED循环显示温湿度，直到蓝牙收到'7'(返回0)或'8'(返回1)
uint8_t DHT11_Display(void)
{
	uint8_t buf[5];
	int8_t rt;

	//停止轮胎运动，冻结运动状态画面
	PID_STOP();
	OLED_Clear();
	OLED_ShowString(0,0,(u8*)"Temp:",16);
	OLED_ShowString(0,2,(u8*)"Humi:",16);

	while(temp3!='7' && temp3!='8')
	{
		rt=DHT11_ReadData(buf);
		if(rt==0)
		{
			//温度整数（DHT11小数位恒为0）
			OLED_ShowNum(48,0,buf[2],2,16);
			OLED_ShowString(72,0,(u8*)" C",16);
			//湿度整数
			OLED_ShowNum(48,2,buf[0],2,16);
			OLED_ShowString(72,2,(u8*)" %",16);
			OLED_ShowString(0,4,(u8*)"Monitoring ",16);
		}
		else
		{
			//读取失败时显示错误，不显示错误数据
			OLED_ShowString(0,4,(u8*)"DHT11 Error",16);
		}
		delay_ms(1000);//DHT11采样周期不小于1s
	}

	OLED_Clear();
	return (temp3=='8')?1:0;
}
