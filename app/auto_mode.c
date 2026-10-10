#include "auto_mode.h"
#include "delay.h"
#include "bluetooth.h"
#include "bsp_io.h"
#include "obstacle.h"
#include "dht11.h"
#include "cleaner.h"
#include "drive.h"
#include "oled.h"
#include "bmp_img.h"

//====================弓形（碰障回弹式）路径规划参数：均需实测标定====================
#define TURN_90_MS    600    //弓形转向时间(ms)：转向PWM已提升至650，转不足加大/转过减小
#define BACK_MS       300    //直行遇障碍时先短暂后退的时间(ms)
#define STRAIGHT_MS   1000   //换向后短直行保持时间(ms)：默认1秒

//测距融合：左/右超声+左/右/中红外 → l_obs/r_obs/c_obs（1=有障碍）
static void auto_sense(uint16_t *dl,uint16_t *dr,uint8_t *lo,uint8_t *ro,uint8_t *co)
{
	*dl=Ultra_GetDistance(0);
	*dr=Ultra_GetDistance(1);
	*lo=(PCin(0)==0)||(*dl<ULTRA_SAFE_DIST);
	*ro=(PCin(1)==0)||(*dr<ULTRA_SAFE_DIST);
	*co=(PCin(2)==0)||(*dl<ULTRA_SAFE_DIST&&*dr<ULTRA_SAFE_DIST);
}

//边刷+吸尘器联动：temp3=='A'开，'B'关，其他指令保持当前状态
static void auto_brush(void)
{
	if(temp3=='A')
	{
		Cleaner_Start();
	}
	else if(temp3=='B')
	{
		Cleaner_Stop();
	}
}

//撞障后重判定：先短暂后退拉开与障碍物的距离，再重新测左右障碍
//撞障瞬间正前方障碍必然使两侧超声距离同时小于安全距离(15cm)，
//若直接判定会把"正前方有墙"误判成"两侧都有障碍"，导致只做单次转向脱困而跳过弓形换向
static void auto_rejudge(uint16_t *dl,uint16_t *dr,uint8_t *lo,uint8_t *ro,uint8_t *co)
{
	PID_GO(-TARGET_SPEED,-TARGET_SPEED);
	OLED_DrawBMP(0,0,128,8,BMP2);
	delay_ms(BACK_MS);
	auto_sense(dl,dr,lo,ro,co);
	auto_brush();
}

//原地转向：dir=1右转，dir=0左转，持续TURN_90_MS
//方向指令与实测正确的手动模式一致：'4'右转=PID_GO(-X,+X)，'5'左转=PID_GO(+X,-X)
//BMP4=右转箭头图，BMP3=左转箭头图（实测标定），与实际转向对应
static void auto_turn(uint8_t dir)
{
	if(dir){ PID_GO(-TARGET_SPEED,TARGET_SPEED); OLED_DrawBMP(0,0,128,8,BMP4); }
	else   { PID_GO(TARGET_SPEED,-TARGET_SPEED); OLED_DrawBMP(0,0,128,8,BMP3); }
	delay_ms(TURN_90_MS);
}

//直行指定毫秒数，期间持续检测前方障碍，撞障提前结束（随时随地避障）
//返回1=因前方障碍提前结束，0=时间到或收到'8'结束
static uint8_t auto_go_ms(uint32_t ms)
{
	uint32_t t=0;
	uint16_t dl,dr; uint8_t lo,ro,co;
	PID_GO(TARGET_SPEED,TARGET_SPEED);
	OLED_DrawBMP(0,0,128,8,BMP1);
	while(t<ms)
	{
		if(temp3=='8') return 0;
		auto_sense(&dl,&dr,&lo,&ro,&co);
		auto_brush();
		if(co) return 1;//前方有障碍，提前结束直行
		delay_ms(20);
		t+=20;
	}
	return 0;
}

//弓形180°换向（U-turn）：两次90°转向中间夹直行1秒，反向并行清扫
//dir=1右U-turn（右转→直行1秒→右转），dir=0左U-turn（左转→直行1秒→左转）
static void auto_uturn(uint8_t dir)
{
	auto_turn(dir);
	auto_go_ms(STRAIGHT_MS);
	auto_turn(dir);
}

//直行直到前方有障碍或收到'8'退出；返回1表示收到'8'需退出
static uint8_t auto_go_until_obstacle(uint16_t *dl,uint16_t *dr,uint8_t *lo,uint8_t *ro,uint8_t *co)
{
	uint16_t dbg_cnt=0;
	PID_GO(TARGET_SPEED,TARGET_SPEED);
	OLED_DrawBMP(0,0,128,8,BMP1);
	while(1)
	{
		if(temp3=='8') return 1;
		if(temp3=='C')
		{
			if(DHT11_Display()) return 1;
			temp3='1';
			PID_GO(TARGET_SPEED,TARGET_SPEED);
		}
		auto_sense(dl,dr,lo,ro,co);
		auto_brush();
		if(*co) return 0;//前方有障碍，结束直行
		OLED_DrawBMP(0,0,128,8,BMP1);
		if(++dbg_cnt>=50){dbg_cnt=0;Drive_DebugReport();}//约1秒一帧PWM遥测
		delay_ms(20);
	}
}

//====================自动模式主流程====================
//先一直直行撞障 → 后退拉开距离重新判定左右障碍（避免正前方墙体误判为两侧都堵）：
//  两侧均无：无障碍之字循环（右U-turn/左U-turn交替，转向→直行1秒→再转向→直行至撞障），直到某侧出现障碍
//  仅左侧有：右U-turn远离 → 先左U-turn交替的镜像之字循环
//  仅右侧有：左U-turn远离 → 先右U-turn交替的之字循环
//  两侧都有：后退右转脱困；随后回到主流程重新判定
void Auto_Mode(void)
{
	uint16_t dist_l, dist_r;    //左/右超声波测距结果(cm)，999=无回波/超量程
	uint8_t l_obs, r_obs, c_obs;//左/右/前 障碍标志（红外+超声融合，1=有障碍）
	uint8_t exit_flag=0;       //收到蓝牙'8'退出标志

	Drive_SetBoost(1);//自动模式：左右轮基准PWM同时+40（600→640）

	while(temp3!='8' && !exit_flag)
	{
		//先一直直行直到前方有障碍物（途中持续检测+边刷控制）
		if(auto_go_until_obstacle(&dist_l,&dist_r,&l_obs,&r_obs,&c_obs)) break;

		//后退拉开距离后重新判定左右障碍，避免正前方墙体被误判为两侧都有障碍
		auto_rejudge(&dist_l,&dist_r,&l_obs,&r_obs,&c_obs);

		//两边都无障碍：无障碍之字循环，右U-turn/左U-turn交替，直到某侧出现障碍
		if(!l_obs && !r_obs)
		{
			uint8_t turn=1;//1=右U-turn，0=左U-turn，交替
			while(temp3!='8')
			{
				auto_uturn(turn);            //U-turn换向（两次90°转向中间直行1秒）
				if(auto_go_until_obstacle(&dist_l,&dist_r,&l_obs,&r_obs,&c_obs)){exit_flag=1;break;}
				auto_rejudge(&dist_l,&dist_r,&l_obs,&r_obs,&c_obs);//后退拉开距离后重判
				if(l_obs||r_obs) break;      //某侧出现障碍，结束之字循环进入侧障碍判定
				turn=!turn;                  //交替下一次反向U-turn
			}
			if(exit_flag) break;
		}

		//侧障碍处理：按左右障碍执行U-turn远离并进入镜像之字循环
		if(l_obs && !r_obs)
		{
			//左侧有障碍、右侧无：右U-turn远离左障碍，再进入先左U-turn交替的之字循环
			uint8_t turn=0;//先左U-turn，交替
			auto_uturn(1);
			while(temp3!='8')
			{
				if(auto_go_until_obstacle(&dist_l,&dist_r,&l_obs,&r_obs,&c_obs)){exit_flag=1;break;}
				auto_rejudge(&dist_l,&dist_r,&l_obs,&r_obs,&c_obs);//后退拉开距离后重判
				if(l_obs||r_obs) break;      //某侧出现障碍，结束镜像循环
				auto_uturn(turn);
				turn=!turn;
			}
		}
		else if(!l_obs && r_obs)
		{
			//右侧有障碍、左侧无：左U-turn远离右障碍，再进入先右U-turn交替的之字循环
			uint8_t turn=1;//先右U-turn，交替
			auto_uturn(0);
			while(temp3!='8')
			{
				if(auto_go_until_obstacle(&dist_l,&dist_r,&l_obs,&r_obs,&c_obs)){exit_flag=1;break;}
				auto_rejudge(&dist_l,&dist_r,&l_obs,&r_obs,&c_obs);//后退拉开距离后重判
				if(l_obs||r_obs) break;
				auto_uturn(turn);
				turn=!turn;
			}
		}
		else if(l_obs && r_obs)
		{
			//两侧都有障碍（三面堵死）：先后退再右转尝试脱困
			PID_GO(-TARGET_SPEED,-TARGET_SPEED);
			OLED_DrawBMP(0,0,128,8,BMP2);
			delay_ms(BACK_MS);
			auto_turn(1);
			PID_GO(TARGET_SPEED,TARGET_SPEED);
			OLED_DrawBMP(0,0,128,8,BMP1);
		}
	}
	//退出自动模式：轮胎停车，边刷与吸尘器同步停止，PWM基准恢复默认
	Drive_SetBoost(0);
	PID_STOP();
	Cleaner_Stop();
}
