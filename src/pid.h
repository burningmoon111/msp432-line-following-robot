/**
 * pid.h — PID 速度控制模块
 *
 * 【文件说明】
 * 为 MSP432 循迹小车提供双电机 PID 速度闭环控制。
 * 上层循迹逻辑只需调用 PID_SetTarget(左目标速度, 右目标速度) 即可，
 * PID 模块会自动根据编码器反馈调整 PWM 输出，使实际转速趋近目标。
 *
 * 【数据流】
 *   目标转速(deg/s) ──→ PID ──→ 修正后转速(deg/s) ──→ 转速→PWM ──→ 电机硬件
 *                        ↑                      ↑              ↑
 *                    实际转速(编码器)      左右分别标定     DianJi_SheZhi
 *
 * 【使用示例】
 *   PID_SetTarget(200, 200);   // 直行
 *   PID_SetTarget(250, 150);   // 左转（左快右慢）
 *   PID_SetTarget(0, 0);       // 停车
 */
#ifndef PID_H
#define PID_H

#include <stdint.h>

/* ========================== 固定精度定义 ========================== */

// PID 内部使用 ×100 定点运算（2 位小数精度）
// 例如 Kp=150 表示实际增益 1.50，output_max=30000 表示限幅 ±300.00 deg/s
#define PID_FIXED_SCALE     100

/* ========================== 数据结构 ========================== */

/**
 * PID_Controller — 单个电机的 PID 控制器实例
 *
 * 沿用项目中 AnNiu 结构体的设计模式，将所有状态封装在结构体中。
 * 左右电机各一个实例（pid_left, pid_right），支持独立调参和标定。
 */
typedef struct {
    /* ---- PID 增益（×100 定点，例如 150 = 1.50） ---- */
    int32_t Kp;             // 
    int32_t Ki;             // 
    int32_t Kd;             // 预留，默认0
		int32_t varI_k;  

    /* ---- 输出限幅 ---- */
    int32_t output_max;     // 输出上限
    int32_t output_min;     // （例如 -30000）

    /* ---- 积分抗饱和限幅*/
    int32_t integral_max;   // 积分上限
    int32_t integral_min;   // 积分下限

    /* ---- 转速→PWM 标定系数（左右电机不同，需实测） ----  */
    int32_t rpm_to_pwm_num; // 转速→PWM 分子（默认 40000）
    int32_t rpm_to_pwm_den; // 转速→PWM 分母（默认 300，即假设最大转速约 300 deg/s）

    /* ---- 运行时状态 ---- */
    int32_t target;         // 目标转速
    int32_t actual;         // 实测转速 
    int32_t error;          // 当前误差 
    int32_t integral;       // 积分累加值
    int32_t prev_error;     // 上一次误差
    int32_t output;         // PID 修正后转速（×PID_FIXED_SCALE，例如 15000=150.00 deg/s）
    int32_t pwm;            // 最终 PWM 占空比（-40000 ~ +40000，符号=方向）
} PID_Controller;

/**
 * PID_Mode — PID 工作模式
 */
typedef enum {
    PID_MODE_MANUAL = 0,    // 手动模式（按键控制），PID 不干预电机（上电默认）
    PID_MODE_AUTO   = 1     // 自动模式（PID 控制），调用 PID_SetTarget() 自动切换
} PID_Mode;

/* ========================== 全局实例（extern，定义在 pid.c） ========================== */
extern PID_Controller pid_left;     // 左电机 PID 控制器
extern PID_Controller pid_right;    // 右电机 PID 控制器

/* ========================== API 函数 ========================== */

/**
 * PID_Init — 初始化一个 PID 控制器实例
 *
 * @param pid              控制器指针
 * @param Kp,Ki,Kd         PID 增益（×100 定点，例如 150=1.50, 10=0.10）
 * @param out_max/min      转速输出限幅（×100，例如 30000=300.00 deg/s）
 * @param int_max/min      积分抗饱和限幅（×100）
 * @param pwm_num/den      转速→PWM 标定系数: PWM = speed × num / den
 */
void PID_Init(PID_Controller *pid,
              int32_t Kp, int32_t Ki, int32_t Kd,int32_t varI_k,
              int32_t out_max, int32_t out_min,
              int32_t int_max, int32_t int_min,
              int32_t pwm_num, int32_t pwm_den);

/**
 * PID_SetCalibration — 运行时修改单个电机的转速→PWM 标定系数
 */
void PID_SetCalibration(PID_Controller *pid, int32_t pwm_num, int32_t pwm_den);

/**
 * PID_SetTarget — 设定左右电机目标转速
 *
 * 这是上层循迹逻辑唯一需要调用的函数。
 * 调用后自动切换到 PID_MODE_AUTO，PID 开始控制电机。
 *
 * @param left_target_deg_s   左电机目标转速 (deg/s)，正=前进
 * @param right_target_deg_s  右电机目标转速 (deg/s)，正=前进
 */
void PID_SetTarget(int32_t left_target_deg_s, int32_t right_target_deg_s);

void PID_Compute(void);

void     PID_SetMode(PID_Mode mode);
PID_Mode PID_GetMode(void);

/**
 * PID_Reset — 复位两个控制器
 * 清零积分项、误差、输出、目标值，切回手动模式
 */
void PID_Reset(void);

/* ---- 调试 Getter（用于串口打印调参） ---- */
int32_t PID_GetLeftTarget(void);        // 左电机目标转速 (deg/s)
int32_t PID_GetRightTarget(void);       // 右电机目标转速 (deg/s)
int32_t PID_GetLeftActual(void);        // 左电机实测转速 (deg/s)
int32_t PID_GetRightActual(void);       // 右电机实测转速 (deg/s)
int32_t PID_GetLeftOutput(void);        // 左电机 PID 输出转速 (deg/s, ×100)
int32_t PID_GetRightOutput(void);       // 右电机 PID 输出转速 (deg/s, ×100)
int32_t PID_GetLeftPwm(void);           // 左电机最终 PWM 值（有符号）
int32_t PID_GetRightPwm(void);          // 右电机最终 PWM 值（有符号）

#endif /* PID_H */
