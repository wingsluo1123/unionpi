#include "circle_walk.h"
#include "controller.h"
#include <math.h>
#include <stdio.h>

static pid_res yaw_pid;

static float normalize_angle(float angle) {
    while (angle > 180.0f) angle -= 360.0f;
    while (angle < -180.0f) angle += 360.0f;
    return angle;
}

void circle_walk_init(CircleWalkController *ctrl, float radius, float speed) {
    if (ctrl == NULL) return;
    
    ctrl->circle_radius = radius;
    ctrl->current_angle = 0.0f;
    ctrl->target_angle = 0.0f;
    ctrl->current_yaw = 0.0f;
    ctrl->target_yaw = 0.0f;
    ctrl->state = CIRCLE_STATE_IDLE;
    ctrl->base_speed = speed;
    
    ctrl->circumference = 2.0f * M_PI * radius;
    ctrl->total_distance = ctrl->circumference;
    ctrl->traveled_distance = 0.0f;
    
    if (radius > 0.01f) {
        float angular_velocity = speed / radius;
        float angular_velocity_deg_s = angular_velocity * 180.0f / M_PI;
        ctrl->turn_speed = (int16_t)(angular_velocity_deg_s * 100.0f);
    } else {
        ctrl->turn_speed = 5000;
    }
    
    yaw_pid.Kp = 0.8f;
    yaw_pid.Ki = 0.005f;
    yaw_pid.Kd = 0.1f;
    yaw_pid.integral = 0.0f;
    yaw_pid.err_last = 0.0f;
    
    printf("[CircleWalk] Initialized: radius=%.2fm, speed=%.0fmm/s, turn_speed=%d (%.1f°/s)\n",
           radius, speed, (int)ctrl->turn_speed, ctrl->turn_speed / 100.0f);
}

void circle_walk_start(CircleWalkController *ctrl) {
    if (ctrl == NULL) return;
    
    extern float fAngle[3];
    ctrl->current_yaw = fAngle[2];
    ctrl->target_yaw = ctrl->current_yaw;
    ctrl->current_angle = 0.0f;
    ctrl->traveled_distance = 0.0f;
    ctrl->state = CIRCLE_STATE_WALKING;
    
    printf("[CircleWalk] Started! radius=%.2fm, yaw=%.1f°\n", 
           ctrl->circle_radius, ctrl->current_yaw);
}

void circle_walk_stop(CircleWalkController *ctrl) {
    if (ctrl == NULL) return;
    
    ctrl->state = CIRCLE_STATE_IDLE;
    printf("[CircleWalk] Stopped!\n");
}

void circle_walk_update(CircleWalkController *ctrl, float yaw, float distance_delta) {
    if (ctrl == NULL) return;
    
    ctrl->current_yaw = yaw;
    
    switch (ctrl->state) {
        case CIRCLE_STATE_WALKING:
            ctrl->traveled_distance += distance_delta;
            ctrl->current_angle = (ctrl->traveled_distance / ctrl->circumference) * 360.0f;
            
            float delta_angle = (distance_delta / ctrl->circumference) * 360.0f;
            ctrl->target_yaw = normalize_angle(ctrl->target_yaw + delta_angle);
            
            static int print_counter = 0;
            if (++print_counter % 50 == 0) {
                printf("[CircleWalk] Walking: %.2f/%.2fm (%.1f°), target_yaw=%.1f°, current_yaw=%.1f°\n", 
                       ctrl->traveled_distance, ctrl->circumference, ctrl->current_angle,
                       ctrl->target_yaw, ctrl->current_yaw);
            }
            
            break;
            
        default:
            break;
    }
}

void circle_walk_get_control(CircleWalkController *ctrl, int16_t *vel, int16_t *ang) {
    if (ctrl == NULL || vel == NULL || ang == NULL) {
        *vel = 0;
        *ang = 0;
        return;
    }
    
    switch (ctrl->state) {
        case CIRCLE_STATE_WALKING: {
            float yaw_error = normalize_angle(ctrl->target_yaw - ctrl->current_yaw);
            
            if (fabs(yaw_error) < 3.0f) {
                *vel = (int16_t)ctrl->base_speed;
                *ang = ctrl->turn_speed;
                yaw_pid.integral = 0.0f;
                yaw_pid.err_last = yaw_error;
                break;
            }
            
            yaw_pid.ActualAngle = ctrl->current_yaw;
            yaw_pid.SetAngle = ctrl->target_yaw;
            yaw_pid.err = yaw_error;
            
            float pid_output = yaw_pid.Kp * yaw_pid.err + 
                               yaw_pid.Ki * yaw_pid.integral + 
                               yaw_pid.Kd * (yaw_pid.err - yaw_pid.err_last);
            
            yaw_pid.err_last = yaw_pid.err;
            yaw_pid.integral += yaw_pid.err;
            
            if (yaw_pid.integral > 100.0f) yaw_pid.integral = 100.0f;
            if (yaw_pid.integral < -100.0f) yaw_pid.integral = -100.0f;
            
            if (pid_output > 10.0f) pid_output = 10.0f;
            if (pid_output < -10.0f) pid_output = -10.0f;
            
            *vel = (int16_t)ctrl->base_speed;
            *ang = (int16_t)(ctrl->turn_speed + pid_output * 100.0f);
            break;
        }
            
        default:
            *vel = 0;
            *ang = 0;
            break;
    }
}