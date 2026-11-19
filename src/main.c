#include "sysinit.h"
#include "delay.h"
#include "msp.h"
#include "my_uart.h"
#include <stdint.h>
#include <stdlib.h>
#include "dingshiqi.h"
#include "anniu.h"
#include "dianji.h"
#include "chuanganqi.h"

// 引脚定义
#define PORT_LED1       	GPIO_PORT_P1
#define PIN_LED1        	GPIO_PIN0       // P1.0，LED1

#define PORT_LED2			GPIO_PORT_P2
#define PIN_LED2_R			GPIO_PIN0		// P2.0，LED2红色部分
#define PIN_LED2_G 			GPIO_PIN1		// P2.1，LED2绿色部分
#define PIN_LED2_B 			GPIO_PIN2		// P2.2，LED2蓝色部分



// 函数声明
void LED_ChuShiHua(void);				// 硬件GPIO初始化
void AnNiu_AnXiaQingKuang(void);		// 根据按钮按下情况修改电机状态
void T32_ZhongDuan(void);
void TA1_ZhongDuan(void);
void ChuFa_TongJi(void);				// 统计电机脉冲数，通知打印
void SuDu_HuanSuan(void);				// 换算速度为角速度

// 全局变量
AnNiu S1={PORT_S1, PIN_S1,0,0,0,0};	// 按键S1
AnNiu S2={PORT_S2, PIN_S2,0,0,0,0};	// 按键S2

DianJi_ZhuangTai DianJi_1=TingZhi;			// 电机1状态
DianJi_ZhuangTai DianJi_2=TingZhi;			// 电机2状态

volatile int32_t ChuFa_Zuo=0;		
volatile int32_t ChuFa_You=0;				// 100ms触发次数增量
volatile int32_t Zuo_before=0;
volatile int32_t You_before=0;
volatile int32_t Zuo_now=0;
volatile int32_t You_now=0;
volatile uint8_t TA1_ZhongDuan_BiaoJi = 0; 
volatile uint8_t T32_ZhongDuan_BiaoJi = 0;
volatile int32_t SuDu_Zuo_ZhengShu=0;		// 电机1速度整数部分
volatile int32_t SuDu_You_ZhengShu=0;		// 电机2速度整数部分
volatile int32_t SuDu_Zuo_XiaoShu=0;		// 电机1速度小数部分
volatile int32_t SuDu_You_XiaoShu=0;		// 电机2速度小数部分

volatile uint16_t chuanganqi;

extern void (*timer1_callback)(void);

int main(void)
{
	// 硬件初始化
	SysInit();         
	delay_init();  
	uart_init(115200);
	MAP_Interrupt_enableMaster(); 

	LED_ChuShiHua();
	AnNiu_ChuShiHua();
	DianJi_ChuShiHua();

	TA3_BuHuoSheZhi();

	TA0_ChuShiHua(TIMER_A_CLOCKSOURCE_DIVIDER_3,40000);
	TA0_ShuChuPeiZhi(0);

	T32_ChuShiHua(18749, 255);
	TA1_ChuShiHua(5000, TA1_ZhongDuan);
	
	while (1)
	{
		AnNiu_AnXiaQingKuang();
		// T32中断
		if (T32_ZhongDuan_BiaoJi)
		{
			T32_ZhongDuan_BiaoJi = 0;
			AnNiu_JianCe(&S1);
    		AnNiu_JianCe(&S2);
			uart_printf("Zuo= %d.%02d deg/s ; You= %d.%02d deg/s\r\n", SuDu_Zuo_ZhengShu, SuDu_Zuo_XiaoShu, SuDu_You_ZhengShu, SuDu_You_XiaoShu);
			chuanganqi=ChuanGanQi_JianCe(200);
			for(int i = 7; i >= 0; i--) {
			uart_printf("%d", (chuanganqi >> i) & 1);
		}
		// TA1中断
		if (TA1_ZhongDuan_BiaoJi)
		{
			TA1_ZhongDuan_BiaoJi = 0;
			ChuFa_TongJi();
			SuDu_HuanSuan();
			}
		}

	}

}

/* 
这里有四个前面定义没写的函数，AI说是在启动文件里已经声明为弱符号，
只要写一个同名函数就会自动覆盖掉弱符号
（虽然不知道弱符号是什么但是假装看懂了）
*/
// AI和我讲3.0是单独的，不过我在指导书没看到
// 但是在看不懂的文件startup_msp432p401r_uvision.s的第105行
//            	 和startup_msp432p401r_uvision.lst的第182行看到了...算了先写在这，报错再改（嘘）
void TA3_0_IRQHandler(void)	// 右A
{
    TIMER_A3->CCTL[0] &= ~TIMER_A_CCTLN_CCIFG;  // 清捕获标志
    
    if (!!(P10->IN & BIT4) ^ !!(P10->IN & BIT5))  
        You_now++;   // 正转
    else
        You_now--;   // 反转
}
// AI让我数两次，其实我感觉有点多余，但是先放在这里
void TA3_N_IRQHandler(void)
{
    switch (TIMER_A3->IV)
    {
        case 0x02:  // 右B
            if (!!(P10->IN & BIT4) ^ !!(P10->IN & BIT5))
                You_now--;
            else
                You_now++;
            break;
            
        case 0x04:  // 左A
            if (!!(P8->IN & BIT2) ^ !!(P9->IN & BIT2))
                Zuo_now--;
            else
                Zuo_now++;
            break;
            
        case 0x06:  // 左B
            if (!!(P8->IN & BIT2) ^ !!(P9->IN & BIT2))
                Zuo_now++;
            else
                Zuo_now--;
            break;
    }
}
void T32_INT1_IRQHandler(void)	// 定时器2中断函数
{
    MAP_Timer32_clearInterruptFlag(TIMER32_0_BASE);
    T32_ZhongDuan();
}
void TA1_0_IRQHandler(void)
{
    TIMER_A1->CCTL[0] &= ~TIMER_A_CCTLN_CCIFG;
    
    if (timer1_callback != 0)
        timer1_callback();
}


/*
	函数实现
*/
// LED GPIO初始化
void LED_ChuShiHua(void)
{
	MAP_GPIO_setAsOutputPin(PORT_LED1, PIN_LED1); 											// LED1配置为输出
	MAP_GPIO_setOutputHighOnPin(PORT_LED1, PIN_LED1); 							 			// LED1常亮
	// 下面两行是LED2，不过好像不管它也是不亮的，算了写上吧
	MAP_GPIO_setAsOutputPin(PORT_LED2, PIN_LED2_R | PIN_LED2_G | PIN_LED2_B); 				// LED2配置为输出
	MAP_GPIO_setOutputLowOnPin(PORT_LED2, PIN_LED2_R | PIN_LED2_G | PIN_LED2_B); 			// LED2不亮
}

void AnNiu_AnXiaQingKuang(void)
{
	if (S1.DuanAn)
	{
		S1.DuanAn=0;// 清除短按标记
		// 1.1.1 L1初次按下，P2.7输出PWM波形（频率100Hz，占空比50%），电机1正转
		if (DianJi_1==TingZhi)
		{
			Timer_A_setCompareValue(TIMER_A0_BASE,TIMER_A_CAPTURECOMPARE_REGISTER_4, 20000);	// 输出占空比50%的PWM波
			MAP_GPIO_setOutputHighOnPin(PORT_ShiNeng, PIN_ZUO_ShiNeng);							// 电机工作
			DianJi_1=ZhengZhuan;
		}
		// 1.1.2 L1重复按下，P2.7输出PWM波形（频率100Hz，占空比50%），电机1反转
		else
		{
			if (DianJi_1==ZhengZhuan)
			{
				MAP_GPIO_setOutputLowOnPin(PORT_ShiNeng, PIN_ZUO_ShiNeng); 			// 电机停止
				MAP_GPIO_setOutputHighOnPin(PORT_ZhuanXiang, PIN_ZUO_ZhuanXiang); 	// 改为反转
				MAP_GPIO_setOutputHighOnPin(PORT_ShiNeng, PIN_ZUO_ShiNeng);			// 电机工作
				DianJi_1=FanZhuan;
			}
			else if (DianJi_1==FanZhuan)
			{
				MAP_GPIO_setOutputLowOnPin(PORT_ShiNeng, PIN_ZUO_ShiNeng); 			// 电机停止
				MAP_GPIO_setOutputLowOnPin(PORT_ZhuanXiang, PIN_ZUO_ZhuanXiang); 	// 改为正转
				MAP_GPIO_setOutputHighOnPin(PORT_ShiNeng, PIN_ZUO_ShiNeng);			// 电机工作
				DianJi_1=ZhengZhuan;
			}
		}
	}

	// 1.1.3 S1长按3秒，电机1停止工作
	if (S1.ChangAn)
	{
		S1.ChangAn=0;// 清除长按标记
		MAP_GPIO_setOutputLowOnPin(PORT_ShiNeng, PIN_ZUO_ShiNeng); 			// 电机停止
		Timer_A_setCompareValue(TIMER_A0_BASE,TIMER_A_CAPTURECOMPARE_REGISTER_4, 0);
		MAP_GPIO_setOutputLowOnPin(PORT_ZhuanXiang, PIN_ZUO_ZhuanXiang); 	// 改为正转
		DianJi_1=TingZhi;
	}

	if (S2.DuanAn)
	{
		S2.DuanAn=0;// 清除短按标记
		// 1.2.1 L2初次按下，P2.6输出PWM波形（频率100Hz，占空比50%），电机2正转
		if (DianJi_2==TingZhi)
		{
			Timer_A_setCompareValue(TIMER_A0_BASE,TIMER_A_CAPTURECOMPARE_REGISTER_3, 20000);	// 输出占空比30%的PWM波
			MAP_GPIO_setOutputHighOnPin(PORT_ShiNeng, PIN_YOU_ShiNeng);							// 电机工作
			DianJi_2=ZhengZhuan;
		}
		// 1.2.2 L2重复按下，P2.6输出PWM波形（频率100Hz，占空比30%），电机2反转
		else
		{
			if (DianJi_2==ZhengZhuan)
			{
				MAP_GPIO_setOutputLowOnPin(PORT_ShiNeng, PIN_YOU_ShiNeng); 			// 电机停止
				MAP_GPIO_setOutputHighOnPin(PORT_ZhuanXiang, PIN_YOU_ZhuanXiang); 	// 改为反转
				MAP_GPIO_setOutputHighOnPin(PORT_ShiNeng, PIN_YOU_ShiNeng);			// 电机工作
				DianJi_2=FanZhuan;
			}
			else if (DianJi_2==FanZhuan)
			{
				MAP_GPIO_setOutputLowOnPin(PORT_ShiNeng, PIN_YOU_ShiNeng); 			// 电机停止
				MAP_GPIO_setOutputLowOnPin(PORT_ZhuanXiang, PIN_YOU_ZhuanXiang); 	// 改为正转
				MAP_GPIO_setOutputHighOnPin(PORT_ShiNeng, PIN_YOU_ShiNeng);			// 电机工作
				DianJi_2=ZhengZhuan;
			}
		}
	}
	// 1.2.3 S2长按3秒，电机2停止工作
	if (S2.ChangAn)
	{
		S2.ChangAn=0;// 清除长按标记
		MAP_GPIO_setOutputLowOnPin(PORT_ShiNeng, PIN_YOU_ShiNeng); 			// 电机停止
		Timer_A_setCompareValue(TIMER_A0_BASE,TIMER_A_CAPTURECOMPARE_REGISTER_3, 0);
		MAP_GPIO_setOutputLowOnPin(PORT_ZhuanXiang, PIN_YOU_ZhuanXiang); 	// 改为正转
		DianJi_2=TingZhi;
	}
}

void T32_ZhongDuan(void)
{
	T32_ZhongDuan_BiaoJi=1;
}

void TA1_ZhongDuan(void)
{
	TA1_ZhongDuan_BiaoJi=1; // 通知主循环中断
}

void ChuFa_TongJi(void)
{
    int32_t Zuo_temp = Zuo_now;
    int32_t You_temp = You_now;

    ChuFa_Zuo = Zuo_temp - Zuo_before;   // 100ms触发次数增量
    ChuFa_You = You_temp - You_before;

    //Zuo_before = Zuo_temp;
    //You_before = You_temp;
}

void SuDu_HuanSuan(void)
{
	// 计算角速度（这里的1560是每圈脉冲数，我手转了一圈差不多是1500，AI说常见值是1560）
	// 这里不用浮点数是因为复位以后偶尔会乱码...我也不知道为什么
	SuDu_Zuo_ZhengShu = (int)(ChuFa_Zuo * (360.0 / 1560.0) * 100.0); 
	SuDu_Zuo_XiaoShu = (int)(ChuFa_Zuo * (360.0 / 1560.0) * 10000.0) - SuDu_Zuo_ZhengShu*100; 

	SuDu_You_ZhengShu = (int)(ChuFa_You * (360.0 / 1560.0) * 100.0);
	SuDu_You_XiaoShu = (int)(ChuFa_You * (360.0 / 1560.0) * 10000.0) - SuDu_You_ZhengShu*100;

	if (SuDu_You_ZhengShu<0){SuDu_You_XiaoShu*=-1;}
	if (SuDu_Zuo_ZhengShu<0){SuDu_Zuo_XiaoShu*=-1;}
}
