#include "controller.h" // 包含头文件，声明相关结构体和函数

pid_res pid; // 定义全局PID控制器结构体变量
extern PC_SendType send_data; // 声明外部变量，发送数据结构体
extern float fAcc[3], fGyro[3], fAngle[3]; // 声明外部IMU数据数组
extern float cumulativeZAngle; // 声明外部累计z轴角度
integral_acc integral_length; // 定义全局积分结构体变量

// PID初始化函数
void PID_init(){
    printf("PID_init begin \n");
    pid.SetAngle = 0.0;        // 设置目标角度
    pid.ActualAngle = 0.0;     // 实际角度
    pid.err = 0.0;             // 当前误差
    pid.err_last = 0.0;        // 上一次误差
    pid.speed = 20.0;          // 初始速度
    pid.integral = 0.0;        // 误差积分
    pid.Kp = 0.6;              // 比例系数（增加响应速度）
    pid.Ki = 0.001;            // 积分系数（减少稳态误差）
    pid.Kd = 0.05;             // 微分系数（减少超调）
    pid.limits = 250;          // 输出限幅
    pid.out = 20;              // 输出初值

    integral_length.Setlength = 0.3;  // 设置目标距离（30cm）
    printf("PID_init end \n");
}

// PID位置式更新函数，返回控制输出
float pid_pos_update(pid_res *p){
    float out = 0;
    
    // 获取当前角度
    p->ActualAngle = cumulativeZAngle;
    
    // 计算角度误差，考虑角度的周期性
    p->err = p->SetAngle - p->ActualAngle;
    
    // 处理角度跨越±180度的情况
    if (p->err > 180.0f) {
        p->err -= 360.0f;
    } else if (p->err < -180.0f) {
        p->err += 360.0f;
    }
    
    // 误差积分（带积分限幅）
    p->integral += p->err;
    if (p->integral > 100.0f) p->integral = 100.0f;
    if (p->integral < -100.0f) p->integral = -100.0f;
    
    // PID公式计算
    p->speed = p->Kp * p->err + p->Ki * p->integral + p->Kd * (p->err - p->err_last);
    
    // 误差迭代
    p->err_last = p->err;
    
    // 计算输出
    out = p->speed;
    
    // 添加最小输出阈值，确保有足够的力矩
    if (fabs(out) > 5.0f) {
        if (out >= 0) {
            out += 90;
        } else {
            out -= 90;
        }
    }
    
    // 限幅处理
    out = range(out, -p->limits, p->limits);
    p->out = out;
    
    return out;
}

// 存储速度和位移
double velocityx[2] = {0.0f, 0.0f}; // 速度数组，前后时刻
double positionX[2] = {0.0f, 0.0f}; // 位移数组，前后时刻
double ay_0 = 0; // 上一时刻加速度
double ay_1 = 0; // 当前加速度

// 双重积分计算位移
float double_integrating_acc(float ay_0, float ay_1, float deltaTime) {
    // 一次积分，计算速度
    velocityx[1] = velocityx[0] + (ay_0 + (ay_1 - ay_0) / 2.0f) * deltaTime;
    // 二次积分，计算位移
    positionX[1] = positionX[0] + (velocityx[0] + (velocityx[1] - velocityx[0]) / 2.0f) * deltaTime;
    // 更新前一时刻数据
    velocityx[0] = velocityx[1];
    positionX[0] = positionX[1];
    return positionX[1]; // 返回当前位移
}

// 模式1：角度控制
void mode_1(){
    // turn;
   // printf("setangle=%lf",pid.SetAngle); // 打印目标角度
    pid_pos_update(&pid); // 更新PID
  //  printf("PID out dead at %f\n", pid.out); // 打印PID输出
    send_data = ChangeSendType(0, 0, pid.out); // 生成发送数据
    write(fd_imu, (unsigned char *) send_data.send_data, 12); // 发送数据到IMU
}

// 模式2：前进控制
void mode_2(){
    // forward move
    ay_1=integral_length.Actacc; // 获取当前加速度
    // integral_length.Actuallength=double_integrating_acc(ay_1,ay_0);
    ay_0=ay_1; // 更新上一时刻加速度
    send_data = ChangeSendType(20, 0, 0); // 生成发送数据
    write(fd_imu, (unsigned char *) send_data.send_data, 12); // 发送数据到IMU
}