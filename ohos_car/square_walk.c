#include "square_walk.h"
#include "controller.h"
#include <math.h>
#include <stdio.h>

// PID 控制器实例（航向保持用）
static pid_res yaw_pid;

// 归一化角度到 [-180, 180]
static float normalize_angle(float angle)
{
    while (angle > 180.0f) angle -= 360.0f;
    while (angle < -180.0f) angle += 360.0f;
    return angle;
}

// 初始化方形行走控制器
void square_walk_init(SquareWalkController *ctrl, float side_length, float speed)
{
    if (ctrl == NULL) return;
    
    // 初始化参数
    ctrl->target_side_length = side_length;
    ctrl->current_distance = 0.0f;
    ctrl->target_yaw = 0.0f;
    ctrl->current_yaw = 0.0f;
    ctrl->current_side = 0;
    ctrl->state = SQUARE_STATE_STRAIGHT;
    ctrl->base_speed = speed;
    ctrl->turn_speed = 100; // 10°/s (100 * 0.01°/s)
    
    // 初始化航向保持 PID
    yaw_pid.Kp = 0.6f;    // 调整为更稳健的参数
    yaw_pid.Ki = 0.001f;
    yaw_pid.Kd = 0.05f;
    yaw_pid.integral = 0.0f;
    yaw_pid.err_last = 0.0f;
    
    printf("[SquareWalk] Initialized: side=%.2fm, speed=%.0f\n", side_length, speed);
}

// 开始方形行走
void square_walk_start(SquareWalkController *ctrl)
{
    if (ctrl == NULL) return;

    ctrl->state = SQUARE_STATE_STRAIGHT;
    ctrl->current_side = 0;
    ctrl->current_distance = 0.0f;
    // 直接读取 IMU 当前航向作为基准，避免使用 init 中初始化的 0
    extern float fAngle[3];
    ctrl->current_yaw = fAngle[2];  // 从 IMU 读取真实 Yaw 角
    ctrl->target_yaw = ctrl->current_yaw;

    printf("[SquareWalk] Started! current yaw=%.1f°, target yaw=%.1f°\n", 
           ctrl->current_yaw, ctrl->target_yaw);
}

// 停止方形行走
void square_walk_stop(SquareWalkController *ctrl)
{
    if (ctrl == NULL) return;
    
    ctrl->state = SQUARE_STATE_IDLE;
    printf("[SquareWalk] Stopped!\n");
}

// 更新方形行走状态
void square_walk_update(SquareWalkController *ctrl, float yaw, float distance_delta)
{
    if (ctrl == NULL) return;
    
    ctrl->current_yaw = yaw;
    
    switch (ctrl->state) {
        case SQUARE_STATE_STRAIGHT:
            ctrl->current_distance += distance_delta;
            printf("[SquareWalk] Straight: %.2f/%.2fm\n", ctrl->current_distance, ctrl->target_side_length);
            
            // 检查是否走完一条边
            if (ctrl->current_distance >= ctrl->target_side_length) {
                // 开始转向
                ctrl->state = SQUARE_STATE_TURNING;
                ctrl->target_yaw = normalize_angle(ctrl->target_yaw + 90.0f);
                printf("[SquareWalk] Side %d done, turning to %.1f°\n", ctrl->current_side + 1, ctrl->target_yaw);
            }
            break;
            
        case SQUARE_STATE_TURNING: {
            float yaw_error = normalize_angle(ctrl->target_yaw - ctrl->current_yaw);
            printf("[SquareWalk] Turning: %.1f -> %.1f (err=%.1f)\n", ctrl->current_yaw, ctrl->target_yaw, yaw_error);
            
            // 检查转向是否完成（误差小于 2 度）
            if (fabs(yaw_error) < 40.0f) {
                ctrl->current_side++;
                
                if (ctrl->current_side >= 4) {
                    // 完成一个方形，继续循环走下一个
                    ctrl->current_side = 0;
                    ctrl->current_distance = 0.0f;
                    ctrl->state = SQUARE_STATE_STRAIGHT;
                    printf("[SquareWalk] Square done, looping...\n");
                } else {
                    // 开始走下一条边
                    ctrl->state = SQUARE_STATE_STRAIGHT;
                    ctrl->current_distance = 0.0f;
                }
            }
            break;
        }
            
        default:
            break;
    }
}

// 获取控制输出
void square_walk_get_control(SquareWalkController *ctrl, int16_t *vel, int16_t *ang)
{
    if (ctrl == NULL || vel == NULL || ang == NULL) {
        *vel = 0;
        *ang = 0;
        return;
    }
    
    switch (ctrl->state) {
        case SQUARE_STATE_STRAIGHT: {
            // 直线行走：用 PID 保持航向
            float yaw_error = normalize_angle(ctrl->target_yaw - ctrl->current_yaw);
            
            // 死区：误差小于 3° 不纠偏，避免底盘来回抖动
            if (fabs(yaw_error) < 100.0f) {
                *vel = (int16_t)ctrl->base_speed;
                *ang = 0;
                yaw_pid.integral = 0.0f;  // 死区内清零积分，防止累积
                yaw_pid.err_last = yaw_error;
                break;
            }
            
            // PID 计算
            yaw_pid.ActualAngle = ctrl->current_yaw;
            yaw_pid.SetAngle = ctrl->target_yaw;
            yaw_pid.err = yaw_error;
            
            float pid_output = yaw_pid.Kp * yaw_pid.err + 
                               yaw_pid.Ki * yaw_pid.integral + 
                               yaw_pid.Kd * (yaw_pid.err - yaw_pid.err_last);
            
            yaw_pid.err_last = yaw_pid.err;
            yaw_pid.integral += yaw_pid.err;
            
            // 积分限幅，防止积分饱和
            if (yaw_pid.integral > 50.0f) yaw_pid.integral = 50.0f;
            if (yaw_pid.integral < -50.0f) yaw_pid.integral = -50.0f;
            
            // 限幅 (最大允许 5°/s 的纠偏速度)
            if (pid_output > 5.0f) pid_output = 5.0f;
            if (pid_output < -5.0f) pid_output = -5.0f;
            
            *vel = (int16_t)ctrl->base_speed;
            *ang = (int16_t)(pid_output * 5.0f); // 转换为 0.01°/s 单位
            break;
        }
            
        case SQUARE_STATE_TURNING: {
            float yaw_error = normalize_angle(ctrl->target_yaw - ctrl->current_yaw);
            
            // 原地转向
            *vel = 0;
            
            // 根据误差方向设置转向速度
            if (yaw_error > 0) {
                *ang = ctrl->turn_speed;
            } else {
                *ang = -ctrl->turn_speed;
            }
            break;
        }
            
        default:
            *vel = 0;
            *ang = 0;
            break;
    }
}