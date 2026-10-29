#ifndef CIRCLE_WALK_H
#define CIRCLE_WALK_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    CIRCLE_STATE_IDLE = 0,
    CIRCLE_STATE_WALKING,
    CIRCLE_STATE_COMPLETED
} CircleWalkState;

typedef struct {
    float circle_radius;      
    float current_angle;      
    float target_angle;       
    float current_yaw;        
    float target_yaw;         
    CircleWalkState state;    
    float base_speed;         
    float turn_speed;         
    float circumference;      
    float total_distance;     
    float traveled_distance;  
} CircleWalkController;

void circle_walk_init(CircleWalkController *ctrl, float radius, float speed);
void circle_walk_start(CircleWalkController *ctrl);
void circle_walk_stop(CircleWalkController *ctrl);
void circle_walk_update(CircleWalkController *ctrl, float yaw, float distance_delta);
void circle_walk_get_control(CircleWalkController *ctrl, int16_t *vel, int16_t *ang);

#endif // CIRCLE_WALK_H