#include "manual_mode.h"
#include "bluetooth.h"
#include "dht11.h"
#include "cleaner.h"
#include "drive.h"
#include "oled.h"
#include "bmp_img.h"

//====================手动操控模式主流程====================
//蓝牙指令：'3'前进 '4'右转 '5'左转 '6'后退 '7'停止 'A'边刷+吸尘器开 'B'关 'C'温湿度界面 '8'退出
void Manual_Mode(void)
{
	uint8_t last_cmd='7';//记录进入温湿度显示前的最后运动命令，用于退出后恢复原运动状态

	while(temp3!='8')
	{
		if(temp3=='C')
		{
			//蓝牙命令'C'：OLED切换为温湿度显示，停止运动状态画面
			if(DHT11_Display()) break;//显示期间收到'8'，退出手动操控模式
			temp3=last_cmd;//收到'7'退出温湿度显示，恢复原运动命令（下一循环执行原动作并重绘对应画面）
		}
		else if(temp3=='3')
		{
			//前进
			last_cmd='3';
			PID_GO(TARGET_SPEED,TARGET_SPEED);
			OLED_DrawBMP(0,0,128,8,BMP1);
		}
		else if(temp3=='4')
		{
			//右转（左轮后退+右轮前进），BMP4=右转箭头图
			last_cmd='4';
			PID_GO(-TARGET_SPEED,TARGET_SPEED);
			OLED_DrawBMP(0,0,128,8,BMP4);
		}
		else if(temp3=='5')
		{
			//左转（左轮前进+右轮后退），BMP3=左转箭头图
			last_cmd='5';
			PID_GO(TARGET_SPEED,-TARGET_SPEED);
			OLED_DrawBMP(0,0,128,8,BMP3);
		}
		else if(temp3=='6')
		{
			//后退
			last_cmd='6';
			PID_GO(-TARGET_SPEED,-TARGET_SPEED);
			OLED_DrawBMP(0,0,128,8,BMP2);
		}
		else if(temp3=='7')
		{
			//停止
			last_cmd='7';
			PID_STOP();
		}
		else if(temp3=='A')
		{
			//边刷与吸尘器同时开启
			Cleaner_Start();
		}
		else if(temp3=='B')
		{
			//边刷与吸尘器同时停止
			Cleaner_Stop();
		}
	}
	//退出手动模式：轮胎停车，边刷与吸尘器同步停止
	PID_STOP();
	Cleaner_Stop();
}
