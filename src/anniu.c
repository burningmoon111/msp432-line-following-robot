#include "anniu.h"
#include "msp.h"


// 按钮初始化函数
void AnNiu_ChuShiHua(void)
{
	MAP_GPIO_setAsInputPinWithPullUpResistor(PORT_S1, PIN_S1); 		// S1配置为输入，启用上拉电阻
	MAP_GPIO_setAsInputPinWithPullUpResistor(PORT_S2, PIN_S2); 		// S2配置为输入，启用上拉电阻
}


/*
下面是按钮检测函数，分以下几种情况考虑：
1.按钮是按下状态
1.1按钮刚被按下：更改按钮状态，不累积时间
1.2按钮被持续按压：累积时间

2.按钮是松开状态（此时如果按钮一直是松开的，不需要处理；如果按钮上一时刻是按下的，判断刚才是长按还是短按）
2.1松开按钮：判断长按还是短按，把上一时刻按钮状态置0，把按钮时间归0
*/
void AnNiu_JianCe(AnNiu *S)		// 按钮检测函数
{
	if (!MAP_GPIO_getInputPinValue(S->PORT, S->PIN)) // 检测到按钮被按着（按钮低电平，取反为1，True）
	{
		if (!S->AnXia_before) // 按钮上一时刻未按下
		{
			S->AnXia_before = 1; // 更改按钮状态为被按下
		}
		else // 按钮上一时刻已按下
		{
			if (S->AnXia_ShiJian < 3000) // 到了3000就不累加了，要不万一按着不放溢出了就出事故了
			{
				S->AnXia_ShiJian += 100; // 累加按钮被按的时间
			}
		}
	}
	else // 按钮被松开
	{
		if (S->AnXia_before) // 按钮上一时刻被按下
		{
			if (S->AnXia_ShiJian < 3000) // 短按
			{
				S->DuanAn = 1; // 设置短按标记
			}
			else // 长按
			{
				S->ChangAn = 1; // 设置长按标记
			}
			S->AnXia_before = 0; // 更改按钮状态为未按下
			S->AnXia_ShiJian = 0; // 按钮被按的时间归0
		}
	}
}
