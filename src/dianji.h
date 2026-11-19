#ifndef DIANJI_H
#define DIANJI_H

#include "sysinit.h"
#include "delay.h"
#include "msp.h"
#include <stdint.h>
#include <stdlib.h>

// 电机使能、转向引脚定义
#define PORT_ZhuanXiang		GPIO_PORT_P1
#define PIN_ZUO_ZhuanXiang	GPIO_PIN7		// P1.7 左电机转向
#define PIN_YOU_ZhuanXiang	GPIO_PIN6		// P1.6 右电机转向

#define PORT_ShiNeng		GPIO_PORT_P3
#define PIN_ZUO_ShiNeng		GPIO_PIN7		// P3.7 左电机使能引脚
#define PIN_YOU_ShiNeng		GPIO_PIN6		// P3.6 右电机使能引脚

// 定义电机状态
typedef enum
{
    TingZhi=-1,
	FanZhuan=0,	// 电机状态：-1停止，0反转，1正转
	ZhengZhuan=1,
}DianJi_ZhuangTai;

// 电机初始化函数
void DianJi_ChuShiHua(void);

// 左电机设置（PWM：正=正转，负=反转，0=停止，范围±40000）
void DianJi_Zuo_SheZhi(int32_t pwm);

// 右电机设置（PWM：正=正转，负=反转，0=停止，范围±40000）
void DianJi_You_SheZhi(int32_t pwm);

#endif /* DIANJI_H */
