#include "pid.h"
#include "dianji.h"
#include "dingshiqi.h"
#include "msp.h"
#include <stdint.h>
#include <math.h>
/* ========================== 常量 ========================== */

#define ENCODER_PPR         1500    // 编码器每圈脉冲数
#define PID_PERIOD_MS       20    // PID 控制周期（ms）
#define DEG_S_NUMERATOR     150  // 速度换算：脉冲 × 150 / 13 → deg/s
#define DEG_S_DENOMINATOR   13

PID_Controller pid_left;    // 左电机 PID 控制器
PID_Controller pid_right;   // 右电机 PID 控制器

static PID_Mode pid_mode = PID_MODE_AUTO;  //自动进入pid控制

// 引用 main.c 的全局脉冲累加值
extern volatile int32_t Zuo_now;
extern volatile int32_t You_now;

// 引用 main.c 的 历史脉冲值
extern volatile int32_t Zuo_before;
extern volatile int32_t You_before;


static void PID_ComputeOne(PID_Controller *pid);

void PID_Init(PID_Controller *pid,int32_t Kp, int32_t Ki, int32_t Kd,int32_t varI_k,int32_t out_max, int32_t out_min,
              int32_t int_max, int32_t int_min,int32_t pwm_num, int32_t pwm_den)
{
		pid->Kp           = Kp;
    pid->Ki           = Ki;
    pid->Kd           = Kd;
		pid->varI_k       = varI_k;

    /* ---- 输出限幅 ---- */
    pid->output_max   = out_max;
    pid->output_min   = out_min;

    /* ---- 积分抗饱和限幅 ---- */
    pid->integral_max = int_max;
    pid->integral_min = int_min;

    /* ---- 转速→PWM 标定系数 ---- */
    pid->rpm_to_pwm_num = pwm_num;//最大PWM
    pid->rpm_to_pwm_den = pwm_den;//最大转速

    /* ---- 运行时状态清零 ---- */
    pid->target       = 0;
    pid->actual       = 0;
    pid->error        = 0;
    pid->integral     = 0;// 积分累加值
    pid->prev_error   = 0;
    pid->output       = 0;
    pid->pwm          = 0;
		
}

void PID_SetTarget(int32_t left_target_deg_s, int32_t right_target_deg_s)
{
    pid_left.target  = left_target_deg_s;
    pid_right.target = right_target_deg_s;
   
}
void PID_Compute(void)
{
    int32_t pulses_left  = Zuo_now- Zuo_before;
    int32_t pulses_right = You_now -You_before;
    Zuo_before=Zuo_now;
		You_before=You_now;
		

    //换算为 deg/s
    pid_left.actual  = (pulses_left  * DEG_S_NUMERATOR) / DEG_S_DENOMINATOR;
    pid_right.actual = (pulses_right * DEG_S_NUMERATOR) / DEG_S_DENOMINATOR;

    //对左右电机分别执行 PID 运算 
    PID_ComputeOne(&pid_left);
    PID_ComputeOne(&pid_right);

    /* ---- 5. 将 PWM 值写入硬件 ---- */
    DianJi_Zuo_SheZhi(pid_left.pwm);
    DianJi_You_SheZhi(pid_right.pwm);
}

/**
 * PID_SetCalibration — 运行时修改转速→PWM 标定系数
 */
void PID_SetCalibration(PID_Controller *pid, int32_t pwm_num, int32_t pwm_den)
{
    pid->rpm_to_pwm_num = pwm_num;
    pid->rpm_to_pwm_den = pwm_den;
}

/**
 * PID_SetTarget — 设定目标转速
 */

void PID_ComputeOne(PID_Controller *pid){
		pid->error=pid->target - pid->actual;
		
		int32_t p_term = pid->Kp * pid->error;//P部分
			
		float abs_err = fabs(pid->error);  
		float C = 1.0f / (pid->varI_k * abs_err + 1.0f);// 变速积分系数
		pid->integral += (int32_t)(pid->Ki * C * pid->error);//I部分
	
		int32_t d_term = pid->Kd * (pid->error - pid->prev_error);
    pid->prev_error = pid->error;//d部分
		
	
		if (pid->integral > pid->integral_max) {
        pid->integral = pid->integral_max;
    }
    if (pid->integral < pid->integral_min) {
        pid->integral = pid->integral_min;
    }//积分限幅
		
    if (pid->target == 0) {
        pid->integral = 0;
    }// 目标为 0 时清积分防止停车时积分累积导致下次启动抖动
		int32_t speed_output = p_term + pid->integral + d_term;

    // 转速域输出限幅
    if (speed_output > pid->output_max)
    {
        speed_output = pid->output_max;
    }
    if (speed_output < pid->output_min)
    {
        speed_output = pid->output_min;
    }
    pid->output = speed_output;
		
		
		//转速转换成PWM
		
    // PWM = speed_actual × num / den
		int32_t pwm = (int32_t)(((int64_t)speed_output * pid->rpm_to_pwm_num) / (pid->rpm_to_pwm_den * 100));//这个100是前面为了精度放大了100倍

    /* ---- PWM 限幅 ---- */
    if (pwm > 40000)  pwm = 40000;
    if (pwm < -40000) pwm = -40000;

    pid->pwm = pwm;
}

void PID_SetMode(PID_Mode mode) { pid_mode = mode; }
PID_Mode PID_GetMode(void)          { return pid_mode; }//工作模式但是好像没啥用

//PID_Reset — 复位两个控制器
void PID_Reset(void)
{
    pid_left.integral    = 0;
    pid_left.prev_error  = 0;
    pid_left.output      = 0;
    pid_left.pwm         = 0;
    pid_left.target      = 0;

    pid_right.integral   = 0;
    pid_right.prev_error = 0;
    pid_right.output     = 0;
    pid_right.pwm        = 0;
    pid_right.target     = 0;

    pid_mode = PID_MODE_MANUAL;
}

/* ---- 读取函数 ---- */
int32_t PID_GetLeftTarget(void)   { return pid_left.target; }
int32_t PID_GetRightTarget(void)  { return pid_right.target; }
int32_t PID_GetLeftActual(void)   { return pid_left.actual; }
int32_t PID_GetRightActual(void)  { return pid_right.actual; }
int32_t PID_GetLeftOutput(void)   { return pid_left.output; }    // ×100 定点转速
int32_t PID_GetRightOutput(void)  { return pid_right.output; }
int32_t PID_GetLeftPwm(void)      { return pid_left.pwm; }       // 最终 PWM 值
int32_t PID_GetRightPwm(void)     { return pid_right.pwm; }
