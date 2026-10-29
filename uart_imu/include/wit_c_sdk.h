/*
 * @Author: WARDEN equinoxf@petalmail.com
 * @Date: 2025-10-03 20:35:33
 * @LastEditors: WARDEN equinoxf@petalmail.com
 * @LastEditTime: 2025-10-13 14:32:13
 * @FilePath: /OpenHarmony/UnionpiTiger_Demo/11_uart_imu/include/wit_c_sdk.h
 * @Description: WIT传感器SDK头文件
 */

#ifndef __WIT_C_SDK_H
#define __WIT_C_SDK_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "REG.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/**
 * @brief 操作成功
 */
#define WIT_HAL_OK (0)

/**
 * @brief 设备忙
 */
#define WIT_HAL_BUSY (-1)

/**
 * @brief 超时
 */
#define WIT_HAL_TIMEOUT (-2)

/**
 * @brief 一般性错误
 */
#define WIT_HAL_ERROR (-3)

/**
 * @brief 内存不足
 */
#define WIT_HAL_NOMEM (-4)

/**
 * @brief 资源为空
 */
#define WIT_HAL_EMPTY (-5)

/**
 * @brief 无效参数
 */
#define WIT_HAL_INVAL (-6)

/**
 * @brief 数据缓冲区大小
 */
#define WIT_DATA_BUFF_SIZE 256

/**
 * @brief 正常协议
 */
#define WIT_PROTOCOL_NORMAL 0

/**
 * @brief Modbus协议
 */
#define WIT_PROTOCOL_MODBUS 1

/**
 * @brief CAN协议
 */
#define WIT_PROTOCOL_CAN 2

/**
 * @brief I2C协议
 */
#define WIT_PROTOCOL_I2C 3

/**
 * @brief JY61协议
 */
#define WIT_PROTOCOL_JY61 4

/**
 * @brief 905x Modbus协议
 */
#define WIT_PROTOCOL_905x_MODBUS 5

/**
 * @brief 905x CAN协议
 */
#define WIT_PROTOCOL_905x_CAN 6

    /* serial function */

    /**
     * @brief 串口写函数指针类型
     */
    typedef void (*SerialWrite)(uint8_t *p_ucData, uint32_t uiLen);

    /**
     * @brief 注册串口写函数
     *
     * @param write_func 串口写函数指针
     * @return 成功返回WIT_HAL_OK，失败返回错误码
     */
    int32_t WitSerialWriteRegister(SerialWrite write_func);

    /**
     * @brief 串口数据输入处理
     *
     * @param ucData 输入的数据字节
     */
    void WitSerialDataIn(uint8_t ucData);

    /* iic function */

    /*
        i2c write function example

        int32_t WitI2cWrite(uint8_t ucAddr, uint8_t ucReg, uint8_t *p_ucVal, uint32_t uiLen)
        {
            i2c_start();
            i2c_send(ucAddr);
            if(i2c_wait_ask() != SUCCESS)return 0;
            i2c_send(ucReg);
            if(i2c_wait_ask() != SUCCESS)return 0;
            for(uint32_t i = 0; i < uiLen; i++)
            {
                i2c_send(*p_ucVal++);
                if(i2c_wait_ask() != SUCCESS)return 0;
            }
            i2c_stop();
            return 1;
        }
    */

    /**
     * @brief I2C写函数指针类型
     */
    typedef int32_t (*WitI2cWrite)(uint8_t ucAddr, uint8_t ucReg, uint8_t *p_ucVal, uint32_t uiLen);

    /*
        i2c read function example

        int32_t WitI2cRead(uint8_t ucAddr, uint8_t ucReg, uint8_t *p_ucVal, uint32_t uiLen)
        {
            i2c_start();
            i2c_send(ucAddr);
            if(i2c_wait_ask() != SUCCESS)return 0;
            i2c_send(ucReg);
            if(i2c_wait_ask() != SUCCESS)return 0;

            i2c_start();
            i2c_send(ucAddr+1);
            for(uint32_t i = 0; i < uiLen; i++)
            {
                if(i+1 == uiLen)*p_ucVal++ = i2c_read(0);  //last byte no ask
                else *p_ucVal++ = i2c_read(1);  //  ask
            }
            i2c_stop();
            return 1;
        }
    */

    /**
     * @brief I2C读函数指针类型
     */
    typedef int32_t (*WitI2cRead)(uint8_t ucAddr, uint8_t ucReg, uint8_t *p_ucVal, uint32_t uiLen);

    /**
     * @brief 注册I2C读写函数
     *
     * @param write_func I2C写函数指针
     * @param read_func I2C读函数指针
     * @return 成功返回WIT_HAL_OK，失败返回错误码
     */
    int32_t WitI2cFuncRegister(WitI2cWrite write_func, WitI2cRead read_func);

    /* can function */

    /**
     * @brief CAN写函数指针类型
     */
    typedef void (*CanWrite)(uint8_t ucStdId, uint8_t *p_ucData, uint32_t uiLen);

    /**
     * @brief 注册CAN写函数
     *
     * @param write_func CAN写函数指针
     * @return 成功返回WIT_HAL_OK，失败返回错误码
     */
    int32_t WitCanWriteRegister(CanWrite write_func);

    /* Delayms function */

    /**
     * @brief 延时函数指针类型
     */
    typedef void (*DelaymsCb)(uint16_t ucMs);

    /**
     * @brief 注册延时函数
     *
     * @param delayms_func 延时函数指针
     * @return 成功返回WIT_HAL_OK，失败返回错误码
     */
    int32_t WitDelayMsRegister(DelaymsCb delayms_func);

    /**
     * @brief CAN数据输入处理
     *
     * @param ucData 输入的数据数组
     * @param ucLen 数据长度
     */
    void WitCanDataIn(uint8_t ucData[8], uint8_t ucLen);

    /**
     * @brief 寄存器更新回调函数指针类型
     */
    typedef void (*RegUpdateCb)(uint32_t uiReg, uint32_t uiRegNum);

    /**
     * @brief 注册寄存器更新回调函数
     *
     * @param update_func 回调函数指针
     * @return 成功返回WIT_HAL_OK，失败返回错误码
     */
    int32_t WitRegisterCallBack(RegUpdateCb update_func);

    /**
     * @brief 写寄存器
     *
     * @param uiReg 寄存器地址
     * @param usData 要写入的数据
     * @return 成功返回WIT_HAL_OK，失败返回错误码
     */
    int32_t WitWriteReg(uint32_t uiReg, uint16_t usData);

    /**
     * @brief 读寄存器
     *
     * @param uiReg 寄存器起始地址
     * @param uiReadNum 要读取的寄存器数量
     * @return 成功返回WIT_HAL_OK，失败返回错误码
     */
    int32_t WitReadReg(uint32_t uiReg, uint32_t uiReadNum);

    /**
     * @brief 初始化WIT传感器
     *
     * @param uiProtocol 协议类型
     * @param ucAddr 设备地址
     * @return 成功返回WIT_HAL_OK，失败返回错误码
     */
    int32_t WitInit(uint32_t uiProtocol, uint8_t ucAddr);

    /**
     * @brief 反初始化WIT传感器
     */
    void WitDeInit(void);

    /**
     ******************************************************************************
     * @file    wit_c_sdk.h
     * @author  Wit
     * @version V1.0
     * @date    05-May-2022
     * @brief   This file provides all Configure sensor function.
     ******************************************************************************
     * @attention
     *
     *        http://wit-motion.cn/
     *
     ******************************************************************************
     */

    /**
     * @brief 开始加速度校准
     *
     * @return 成功返回WIT_HAL_OK，失败返回错误码
     */
    int32_t WitStartAccCali(void);

    /**
     * @brief 停止加速度校准
     *
     * @return 成功返回WIT_HAL_OK，失败返回错误码
     */
    int32_t WitStopAccCali(void);

    /**
     * @brief 开始磁力计校准
     *
     * @return 成功返回WIT_HAL_OK，失败返回错误码
     */
    int32_t WitStartMagCali(void);

    /**
     * @brief 停止磁力计校准
     *
     * @return 成功返回WIT_HAL_OK，失败返回错误码
     */
    int32_t WitStopMagCali(void);

    /**
     * @brief 设置UART波特率
     *
     * @param uiBaudIndex 波特率索引
     * @return 成功返回WIT_HAL_OK，失败返回错误码
     */
    int32_t WitSetUartBaud(int32_t uiBaudIndex);

    /**
     * @brief 设置带宽
     *
     * @param uiBaudWidth 带宽值
     * @return 成功返回WIT_HAL_OK，失败返回错误码
     */
    int32_t WitSetBandwidth(int32_t uiBaudWidth);

    /**
     * @brief 设置输出速率
     *
     * @param uiRate 输出速率
     * @return 成功返回WIT_HAL_OK，失败返回错误码
     */
    int32_t WitSetOutputRate(int32_t uiRate);

    /**
     * @brief 设置输出内容
     *
     * @param uiRsw 输出内容设置
     * @return 成功返回WIT_HAL_OK，失败返回错误码
     */
    int32_t WitSetContent(int32_t uiRsw);

    /**
     * @brief 设置CAN波特率
     *
     * @param uiBaudIndex 波特率索引
     * @return 成功返回WIT_HAL_OK，失败返回错误码
     */
    int32_t WitSetCanBaud(int32_t uiBaudIndex);

    /**
     * @brief 保存参数
     *
     * @return 成功返回WIT_HAL_OK，失败返回错误码
     */
    int32_t WitSaveParameter(void);

    /**
     * @brief 软件复位
     *
     * @return 成功返回WIT_HAL_OK，失败返回错误码
     */
    int32_t WitSetForReset(void);

    /**
     * @brief 校准参考角度
     *
     * @return 成功返回WIT_HAL_OK，失败返回错误码
     */
    int32_t WitCaliRefAngle(void);

    /**
     * @brief 检查数值范围
     *
     * @param sTemp 要检查的数值
     * @param sMin 最小值
     * @param sMax 最大值
     * @return 在范围内返回1，否则返回0
     */
    char CheckRange(short sTemp, short sMin, short sMax);

    /**
     * @brief 传感器寄存器数组
     */
    extern int16_t sReg[REGSIZE];

#ifdef __cplusplus
}
#endif

#endif /* __WIT_C_SDK_H */