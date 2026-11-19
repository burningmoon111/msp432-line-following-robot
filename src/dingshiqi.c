#include "sysinit.h"
#include "delay.h"
#include "msp.h"
#include "my_uart.h"
#include <stdint.h>

void (*timer1_callback)(void) = 0; // 记住中断的时候用哪个函数

// TA0输出PWM引脚定义
#define PORT_PWM			GPIO_PORT_P2
#define PIN_ZUO_PWM			GPIO_PIN7		// P2.7，按键S1控制左电机PWM
#define PIN_YOU_PWM			GPIO_PIN6		// P2.6，按键S2控制右电机PWM

// TA3捕获引脚定义
#define PORT_Zuo_BuHuo_A	GPIO_PORT_P8
#define PIN_Zuo_BuHuo_A		GPIO_PIN2		// P8.2 左电机A
#define PORT_Zuo_BuHuo_B	GPIO_PORT_P9
#define PIN_Zuo_BuHuo_B		GPIO_PIN2		// P9.2 左电机B
#define PORT_You_BuHuo_A	GPIO_PORT_P10	
#define PIN_You_BuHuo_A		GPIO_PIN4		// P10.4 右电机A
#define PORT_You_BuHuo_B	GPIO_PORT_P10
#define PIN_You_BuHuo_B		GPIO_PIN5		// P10.5 右电机B

// 定时器A0初始化
void TA0_ChuShiHua(uint16_t divider, uint16_t period)
{
    // 定时器TA0引脚配置：P2.7和P2.6配置为定时器输出
	GPIO_setAsPeripheralModuleFunctionOutputPin(PORT_PWM, PIN_ZUO_PWM | PIN_YOU_PWM, GPIO_PRIMARY_MODULE_FUNCTION);
    
    // 配置结构体
    /* 关于参数：指导书v2.1 P6：PWM 频率 = SMCLK / divider / period */
    Timer_A_UpModeConfig upConfig;   
    upConfig.clockSource = TIMER_A_CLOCKSOURCE_SMCLK;   // 时钟源: SMCLK (12MHz)   
    upConfig.clockSourceDivider = divider;         
    upConfig.timerPeriod= period;      
    upConfig.timerInterruptEnable_TAIE = TIMER_A_TAIE_INTERRUPT_DISABLE;  // 溢出中断关   
    upConfig.captureCompareInterruptEnable_CCR0_CCIE = TIMER_A_CCIE_CCR0_INTERRUPT_DISABLE;   
    upConfig.timerClear = TIMER_A_DO_CLEAR;    // 启动前清零计数器  

    // 启动配置
    Timer_A_configureUpMode(TIMER_A0_BASE, &upConfig);
}

// 定时器A0输出配置
void TA0_ShuChuPeiZhi(uint16_t cushi_cpmparevalue)
{
    Timer_A_CompareModeConfig compareConfig = 
    {   
        .compareOutputMode = TIMER_A_OUTPUTMODE_RESET_SET,   
        // 复位/置位模式：CNT==CCR → OUT=1，CNT==0 → OUT=0（边沿对齐 PWM）   
        .compareValue = cushi_cpmparevalue,                               // 初始占空比 0   
        .compareInterruptEnable = TIMER_A_CAPTURECOMPARE_INTERRUPT_DISABLE   
    };

    // 控制电机1的
	compareConfig.compareRegister = TIMER_A_CAPTURECOMPARE_REGISTER_4; // TA0.4
	Timer_A_initCompare(TIMER_A0_BASE, &compareConfig);
	// 控制电机2的
	compareConfig.compareRegister = TIMER_A_CAPTURECOMPARE_REGISTER_3; // TA0.3
	Timer_A_initCompare(TIMER_A0_BASE, &compareConfig); 
    
    // 启动
    Timer_A_startCounter(TIMER_A0_BASE, TIMER_A_UP_MODE);
}

// 定时器A1初始化（这里是复制的付学长在指导书v2.1 P11的代码，太伟大了，致敬）
void TA1_ChuShiHua(uint16_t period2us, void (*callback)(void))	
{
	timer1_callback = callback;   
	// timer setup   
    Timer_A_stop(TIMER_A1_BASE);// 停止定时器   
    TIMER_A1->CTL = TIMER_A_CTL_SSEL__SMCLK | TIMER_A_CTL_ID__4;   
    TIMER_A1->CCTL[0] = TIMER_A_CCTLN_CCIE;   
    TIMER_A1->CCR[0] = period2us;   
    TIMER_A1->EX0 = TIMER_A_EX0_IDEX__6;   
    // 配置中断   
    Interrupt_setPriority(INT_TA1_0, 2);    // 优先级2（0~7，0 最高）    
    Interrupt_enableInterrupt(INT_TA1_0);   // 使能中断         
	// 启动定时器   
    Timer_A_startCounter(TIMER_A1_BASE, TIMER_A_UP_MODE);
}

// 定时器A3捕获模式设置
void TA3_BuHuoSheZhi(void)
{
	// 输入捕获引脚配置，因为右侧的PORT都是10，所以就写在一行了
	GPIO_setAsPeripheralModuleFunctionInputPin(PORT_Zuo_BuHuo_A,PIN_Zuo_BuHuo_A,GPIO_PRIMARY_MODULE_FUNCTION);
	GPIO_setAsPeripheralModuleFunctionInputPin(PORT_Zuo_BuHuo_B,PIN_Zuo_BuHuo_B,GPIO_PRIMARY_MODULE_FUNCTION);
	GPIO_setAsPeripheralModuleFunctionInputPin(PORT_You_BuHuo_A,PIN_You_BuHuo_A|PIN_You_BuHuo_B,GPIO_PRIMARY_MODULE_FUNCTION);
	// 捕获模式（这个函数段的代码，从这一行开始，和指导书 没 有 一 个 字 母 的差别）
	TIMER_A3->CCTL[0] = TIMER_A3->CCTL[1] = TIMER_A3->CCTL[2] = TIMER_A3->CCTL[3]
	= TIMER_A_CCTLN_CCIS__CCIA    // 捕获输入选择 CCIxA   
    | TIMER_A_CCTLN_CM__BOTH      // 双边沿捕获（上升沿+下降沿）   
    | TIMER_A_CCTLN_SCS           // 同步捕获   
    | TIMER_A_CCTLN_CAP           // 捕获模式（非比较模式）   
    | TIMER_A_CCTLN_CCIE;         // 中断使能
	// 优先级设置
	NVIC->IP[3] = (NVIC->IP[3] & ~(NVIC_IPR3_PRI_14_M | NVIC_IPR3_PRI_15_M))
				| 0x40400000;   
	NVIC->ISER[0] = BIT(TA3_0_IRQn) | BIT(TA3_N_IRQn);  
	// 连续计数模式
	TIMER_A3->CTL = TIMER_A_CTL_MC__STOP;     // 先停   
	TIMER_A3->CTL = TIMER_A_CTL_SSEL__SMCLK;  // 时钟源 SMCLK（12MHz）  
	// ...配置 CCTL 和中断...   
	TIMER_A3->CTL |= TIMER_A_CTL_MC__CONTINUOUS   // 连续模式（计到 0xFFFF 后回绕）   
	              | TIMER_A_CTL_CLR;              // 清零计数器 
}

// 定时器T32初始化
void T32_ChuShiHua(uint32_t aar, uint8_t psc)
{
    MAP_Timer32_initModule(TIMER32_0_BASE, psc, TIMER32_32BIT, TIMER32_PERIODIC_MODE);
	MAP_Timer32_setCount(TIMER32_0_BASE, aar); 
	MAP_Timer32_enableInterrupt(TIMER32_0_BASE);
	MAP_Timer32_startTimer(TIMER32_0_BASE, false);// false连续模式，中断周期性触发
	MAP_Interrupt_enableInterrupt(INT_T32_INT1); 

	MAP_Interrupt_enableMaster(); 
}
