#ifndef ANNIU_H
#define ANNIU_H

#include "sysinit.h"
#include "delay.h"
#include <stdint.h>
#include "msp.h"
#include <stdlib.h>

#define PORT_S1	      		GPIO_PORT_P1
#define PIN_S1          	GPIO_PIN1		// P1.1，按键S1

#define PORT_S2      		GPIO_PORT_P1
#define PIN_S2          	GPIO_PIN4		// P1.4，按键S2

// 定义按钮结构体
typedef struct
{
	uint16_t PORT;
	uint16_t PIN;						// 按钮引脚

	volatile uint8_t AnXia_before;		// 按钮上一时刻是否按下
	volatile uint16_t AnXia_ShiJian;	// 按钮被按了多久

	volatile uint8_t DuanAn;			// 按钮被短按
	volatile uint8_t ChangAn;			// 按钮被长按
}AnNiu;

// 按钮初始化函数
void AnNiu_ChuShiHua(void);

// 按钮检测函数
void AnNiu_JianCe(AnNiu *S);

#endif /* ANNIU_H */
