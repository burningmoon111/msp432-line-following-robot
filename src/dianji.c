#include "dianji.h"

// 电机初始化
void DianJi_ChuShiHua(void)
{
    MAP_GPIO_setAsOutputPin(PORT_ShiNeng, PIN_ZUO_ShiNeng | PIN_YOU_ShiNeng);				// 电机使能引脚配置为输出
	MAP_GPIO_setOutputLowOnPin(PORT_ShiNeng, PIN_ZUO_ShiNeng | PIN_YOU_ShiNeng); 			// 电机停止
	MAP_GPIO_setAsOutputPin(PORT_ZhuanXiang, PIN_ZUO_ZhuanXiang | PIN_YOU_ZhuanXiang); 		// 电机转向引脚配置为输出
	MAP_GPIO_setOutputLowOnPin(PORT_ZhuanXiang, PIN_ZUO_ZhuanXiang | PIN_YOU_ZhuanXiang); 	// 电机初始正转
}

/**
 * DianJi_Zuo_SheZhi — 设置左电机 PWM
 *
 * @param pwm  PWM 占空比：正=正转，负=反转，0=停止，范围 ±40000
 *              对应 TA0.4 (P2.7)，方向引脚 P1.7，使能引脚 P3.7
 */
void DianJi_Zuo_SheZhi(int32_t pwm)
{
    if (pwm > 0)
    {
        // 正转：方向低 → 使能开 → 设置 PWM
        MAP_GPIO_setOutputLowOnPin(PORT_ZhuanXiang, PIN_ZUO_ZhuanXiang);
        Timer_A_setCompareValue(TIMER_A0_BASE, TIMER_A_CAPTURECOMPARE_REGISTER_4, (uint16_t)pwm);
        MAP_GPIO_setOutputHighOnPin(PORT_ShiNeng, PIN_ZUO_ShiNeng);
    }
    else if (pwm < 0)
    {
        // 反转：方向高 → 使能开 → 设置 PWM（取绝对值）
        MAP_GPIO_setOutputHighOnPin(PORT_ZhuanXiang, PIN_ZUO_ZhuanXiang);
        Timer_A_setCompareValue(TIMER_A0_BASE, TIMER_A_CAPTURECOMPARE_REGISTER_4, (uint16_t)(-pwm));
        MAP_GPIO_setOutputHighOnPin(PORT_ShiNeng, PIN_ZUO_ShiNeng);
    }
    else
    {
        // 停止：关使能 → PWM 清零 → 方向恢复正转
        MAP_GPIO_setOutputLowOnPin(PORT_ShiNeng, PIN_ZUO_ShiNeng);
        Timer_A_setCompareValue(TIMER_A0_BASE, TIMER_A_CAPTURECOMPARE_REGISTER_4, 0);
        MAP_GPIO_setOutputLowOnPin(PORT_ZhuanXiang, PIN_ZUO_ZhuanXiang);
    }
}

/**
 * DianJi_You_SheZhi — 设置右电机 PWM
 *
 * @param pwm  PWM 占空比：正=正转，负=反转，0=停止，范围 ±40000
 *              对应 TA0.3 (P2.6)，方向引脚 P1.6，使能引脚 P3.6
 */
void DianJi_You_SheZhi(int32_t pwm)
{
    if (pwm > 0)
    {
        // 正转：方向低 → 使能开 → 设置 PWM
        MAP_GPIO_setOutputLowOnPin(PORT_ZhuanXiang, PIN_YOU_ZhuanXiang);
        Timer_A_setCompareValue(TIMER_A0_BASE, TIMER_A_CAPTURECOMPARE_REGISTER_3, (uint16_t)pwm);
        MAP_GPIO_setOutputHighOnPin(PORT_ShiNeng, PIN_YOU_ShiNeng);
    }
    else if (pwm < 0)
    {
        // 反转：方向高 → 使能开 → 设置 PWM（取绝对值）
        MAP_GPIO_setOutputHighOnPin(PORT_ZhuanXiang, PIN_YOU_ZhuanXiang);
        Timer_A_setCompareValue(TIMER_A0_BASE, TIMER_A_CAPTURECOMPARE_REGISTER_3, (uint16_t)(-pwm));
        MAP_GPIO_setOutputHighOnPin(PORT_ShiNeng, PIN_YOU_ShiNeng);
    }
    else
    {
        // 停止：关使能 → PWM 清零 → 方向恢复正转
        MAP_GPIO_setOutputLowOnPin(PORT_ShiNeng, PIN_YOU_ShiNeng);
        Timer_A_setCompareValue(TIMER_A0_BASE, TIMER_A_CAPTURECOMPARE_REGISTER_3, 0);
        MAP_GPIO_setOutputLowOnPin(PORT_ZhuanXiang, PIN_YOU_ZhuanXiang);
    }
}
