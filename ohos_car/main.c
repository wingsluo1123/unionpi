/**
  ******************************************************************************
  * @file           : main.c
  * @author         : Dylan
  * @brief          : main入口函数
  * @attention      : 必须把串口启动放在main函数部分且不能封装，会乱码
  * @date           : 2023.3.22
  ******************************************************************************
  */


const int c_uiBaud[] = {115200,2400 , 4800 , 9600 , 19200 , 38400 , 57600  , 230400 , 460800 , 921600};

int s_iCurBaud = 115200;
volatile char s_cDataUpdate = 0;  // 添加IMU数据更新标志变量定义

#define ACC_UPDATE		0x01
#define GYRO_UPDATE		0x02
#define ANGLE_UPDATE	0x04
#define MAG_UPDATE		0x08
#define READ_UPDATE		0x80

static void AutoScanSensor(char* dev);
static void SensorDataUpdata(unsigned int uiReg, unsigned int uiRegNum);

#include "main.h"



int fd_chassis;
int fd_imu;
int main(int argc, char **argv)
{
    char *uart_dev = UART_TTL_NAME;
    
    fd_chassis = serial_open(uart_dev, 115200);
    if ( fd_chassis < 0)
    {
        perror("open chassis fail\n");
        return ERR;
    }
    printf("open chassis success\n");
    /**
     *
     * @brief     IMU 初始化
     */
    
    char *uart_imu_dev = IMU_TTL_NAME;

    // fd_imu = serial_open(uart_imu_dev, 115200);
    fd_imu = serial_open(uart_imu_dev, 9600);

    if((fd_imu<0))
	{
	    printf("open imu failed\n");
	    
	}
	else printf("open imu success\n");
    
    WitInit(WIT_PROTOCOL_NORMAL, 0x50);
    WitRegisterCallBack(SensorDataUpdata);
    AutoScanSensor(IMU_TTL_NAME);


    PID_init();



    pthread_t pid_t1;                                                             //底盘进程
    pthread_t pid_t2;                                                             //IMU进程
    pthread_t pid_t3;                                                             //tcp通信进程
    pthread_t pid_t4;                                                             //底盘接收进程
    pthread_create(&pid_t1, NULL, (void *)_serial_output_task, 0);
    pthread_create(&pid_t2, NULL, (void *)_serial_IMU_input_task, 0);
    pthread_create(&pid_t3, NULL, (void *)_tcp_server_socket, 0);
    pthread_create(&pid_t4, NULL, (void *)_serial_input_task, 0);
    while (1)
    {


        sleep(10L);
    }
    close(fd_chassis);
    return 0;
}




void AutoScanSensor(char* dev)
{
	int i, iRetry;
	char cBuff[1];
	
	for(i = 0; i < 10; i++)
	{
		serial_close(fd_imu);
		s_iCurBaud = c_uiBaud[i];
		fd_imu = serial_open(dev , c_uiBaud[i]);
		
		iRetry = 2;
		do
		{
			s_cDataUpdate = 0;
			WitReadReg(AX, 3);
			usleep(200000);
			while(serial_read_data(fd_imu, cBuff, 1))
			{
				WitSerialDataIn(cBuff[0]);
			}
			if(s_cDataUpdate != 0)
			{
				printf("%d baud find sensor\r\n\r\n", c_uiBaud[i]);
                return ;
			}
			iRetry--;
		}while(iRetry);		
	}
	printf("can not find sensor\r\n");
	printf("please check your connection\r\n");
}


void SensorDataUpdata(unsigned int uiReg, unsigned int uiRegNum)
{
    int i;
    for(i = 0; i < uiRegNum; i++)
    {
        switch(uiReg)
        {
//            case AX:
//            case AY:
            case AZ:
				s_cDataUpdate |= ACC_UPDATE;
                   break;
//            case GX:
//            case GY:
            case GZ:
				s_cDataUpdate |= GYRO_UPDATE;
                break;
//            case HX:
//            case HY:
            case HZ:
				s_cDataUpdate |= MAG_UPDATE;
                break;
//            case Roll:
//            case Pitch:
            case Yaw:
				s_cDataUpdate |= ANGLE_UPDATE;
                break;
            default:
				s_cDataUpdate |= READ_UPDATE;
              	break;
        }
		uiReg++;
    }
}