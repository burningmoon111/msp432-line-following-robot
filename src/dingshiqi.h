#ifndef DINGSHIQI_H
#define DINGSHIQI_H

#include "sysinit.h"
#include "delay.h"
#include "msp.h"
#include "my_uart.h"
#include <stdint.h>

// 定时器A0初始化
void TA0_ChuShiHua(uint16_t divider, uint16_t period);

// 定时器A0输出配置
void TA0_ShuChuPeiZhi(uint16_t cushi_cpmparevalue);

// 定时器A0修改占空比
void TA0_XiuGaiZhanKongBi(uint16_t duty);

// 定时器A1初始化
void TA1_ChuShiHua(uint16_t period2us, void (*callback)(void));

// 定时器A3捕获模式设置
void TA3_BuHuoSheZhi(void);

// 定时器T32初始化
void T32_ChuShiHua(uint32_t aar, uint8_t psc);

#endif /* DINGSHIQI_H */
