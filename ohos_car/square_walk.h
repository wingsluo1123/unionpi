#ifndef SQUARE_WALK_H
#define SQUARE_WALK_H

#include <stdint.h>
#include <stdbool.h>
#include "pthread_usr.h"

// 方形行走状态
typedef enum {
    SQUARE_STATE_IDLE = 0,
    SQUARE_STATE_STRAIGHT,
    SQUARE_STATE_TURNING,
    SQUARE_STATE_COMPLETED
} SquareWalkState;

// 方形行走参数
typedef struct {
    float target_side_length;     // 目标边长 (米)
    float current_distance;       // 当前已走距离
    float target_yaw;             // 目标航向角 (度)
    float current_yaw;            // 当前航向角 (度)
    int current_side;             // 当前边 (0-3)
    SquareWalkState state;        // 当前状态
    float base_speed;             // 基础速度
    float turn_speed;             // 转向速度
} SquareWalkController;

// 初始化方形行走控制器
void square_walk_init(SquareWalkController *ctrl, float side_length, float speed);

// 开始方形行走
void square_walk_start(SquareWalkController *ctrl);

// 停止方形行走
void square_walk_stop(SquareWalkController *ctrl);

// 更新方形行走状态（每周期调用）
void square_walk_update(SquareWalkController *ctrl, float yaw, float distance_delta);

// 获取控制输出（左右轮速度）
void square_walk_get_control(SquareWalkController *ctrl, int16_t *vel, int16_t *ang);

#endif // SQUARE_WALK_H