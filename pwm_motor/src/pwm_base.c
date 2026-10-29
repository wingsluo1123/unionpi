/*
 * @Author: WARDEN equinoxf@petalmail.com
 * @Date: 2025-10-30 17:23:58
 * @LastEditors: WARDEN equinoxf@petalmail.com
 * @LastEditTime: 2025-10-30 18:37:45
 * @FilePath: /OpenHarmony/unionpi_whale_demo/pwm_motor/src/pwm_base.c
 * @Description: PWM 驱动实现源文件
 */

#include "../include/pwm_base.h"
#include "securec.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int init_pwm(int pwmChannel)
{
    char pwm_file_name[128] = {0};
    (void)memset_s(pwm_file_name, sizeof(pwm_file_name), 0, sizeof(pwm_file_name));

    if (PWM1 == pwmChannel)
    {
        (void)sprintf_s(pwm_file_name, sizeof(pwm_file_name), "%s/export", PWM1_PEX);
    }
    else if (PWM0 == pwmChannel)
    {
        (void)sprintf_s(pwm_file_name, sizeof(pwm_file_name), "%s/export", PWM0_PEX);
    }
    else
    {
        printf("[%s] PWM WRONG CHANNEL\n", __func__);
        return PWM_WRONOG_CHANNEL;
    }

    if (access(pwm_file_name, F_OK) != 0)
    {
        printf("[%s] PWM EXPORT FILE NOT EXIST\n", __func__);
        return PWM_FILE_NOT_EXIST;
    }

    FILE *fp = NULL;
    fp = fopen(pwm_file_name, "w");
    if (!fp)
    {
        printf("[%s] Failed to open export file!\n", __func__);
        return PWM_FILE_NOT_EXIST;
    }
    (void)fprintf(fp, "%d", 0);
    (void)fclose(fp);
    fp = NULL;
    return 0;
}

int set_pwm_period(int pwmChannel, int period)
{
    char pwm_file_name[128] = {0};
    (void)memset_s(pwm_file_name, sizeof(pwm_file_name), 0, sizeof(pwm_file_name));

    if (PWM1 == pwmChannel)
    {
        (void)sprintf_s(pwm_file_name, sizeof(pwm_file_name), "%s/pwm0/period", PWM1_PEX);
    }
    else if (PWM0 == pwmChannel)
    {
        (void)sprintf_s(pwm_file_name, sizeof(pwm_file_name), "%s/pwm0/period", PWM0_PEX);
    }
    else
    {
        printf("[%s] PWM WRONG CHANNEL\n", __func__);
        return PWM_WRONOG_CHANNEL;
    }

    if (access(pwm_file_name, F_OK) != 0)
    {
        printf("[%s] PWM PERIOD FILE NOT EXIST\n", __func__);
        return PWM_FILE_NOT_EXIST;
    }

    FILE *fp = NULL;
    fp = fopen(pwm_file_name, "r+");
    if (!fp)
    {
        printf("[%s] Failed to open period file!\n", __func__);
        return PWM_FILE_NOT_EXIST;
    }
    if (period)
    {
        fprintf(fp, "%d", period);
    }
    (void)fclose(fp);
    fp = NULL;

    return 0;
}

int set_pwm_dutyCycle(int pwmChannel, int dutyCycle)
{
    char pwm_file_name[128] = {0};
    (void)memset_s(pwm_file_name, sizeof(pwm_file_name), 0, sizeof(pwm_file_name));

    if (PWM1 == pwmChannel)
    {
        (void)sprintf_s(pwm_file_name, sizeof(pwm_file_name), "%s/pwm0/duty_cycle", PWM1_PEX);
    }
    else if (PWM0 == pwmChannel)
    {
        (void)sprintf_s(pwm_file_name, sizeof(pwm_file_name), "%s/pwm0/duty_cycle", PWM0_PEX);
    }
    else
    {
        printf("[%s] PWM WRONG CHANNEL\n", __func__);
        return PWM_WRONOG_CHANNEL;
    }

    if (access(pwm_file_name, F_OK) != 0)
    {
        printf("[%s] PWM DUTY_CYCLE FILE NOT EXIST\n", __func__);
        return PWM_FILE_NOT_EXIST;
    }

    FILE *fp = NULL;
    fp = fopen(pwm_file_name, "r+");
    if (!fp)
    {
        printf("[%s] Failed to open duty_cycle file!\n", __func__);
        return PWM_FILE_NOT_EXIST;
    }
    if (dutyCycle)
    {
        fprintf(fp, "%d", dutyCycle);
    }
    (void)fclose(fp);
    fp = NULL;

    return 0;
}

int set_pwm_polarity(int pwmChannel, int polarity)
{
    char pwm_file_name[128] = {0};
    (void)memset_s(pwm_file_name, sizeof(pwm_file_name), 0, sizeof(pwm_file_name));

    if (PWM1 == pwmChannel)
    {
        (void)sprintf_s(pwm_file_name, sizeof(pwm_file_name), "%s/pwm0/polarity", PWM1_PEX);
    }
    else if (PWM0 == pwmChannel)
    {
        (void)sprintf_s(pwm_file_name, sizeof(pwm_file_name), "%s/pwm0/polarity", PWM0_PEX);
    }
    else
    {
        printf("[%s] PWM WRONG CHANNEL\n", __func__);
        return PWM_WRONOG_CHANNEL;
    }

    if (access(pwm_file_name, F_OK) != 0)
    {
        printf("[%s] PWM POLARITY FILE NOT EXIST\n", __func__);
        return PWM_FILE_NOT_EXIST;
    }

    FILE *fp = NULL;
    fp = fopen(pwm_file_name, "rw+");
    if (!fp)
    {
        printf("[%s] Failed to open polarity file!\n", __func__);
        return PWM_FILE_NOT_EXIST;
    }
    if (polarity == PWM_POLARITY_NORMAL)
    {
        fprintf(fp, "%s", "normal");
    }
    else if (polarity == PWM_POLARITY_INVERSED)
    {
        fprintf(fp, "%s", "inversed");
    }
    (void)fclose(fp);
    fp = NULL;

    return 0;
}

int set_pwm_enable(int pwmChannel, int isEnable)
{
    char buffer[256] = {0};
    char pwm_file_name[128] = {0};
    (void)memset_s(pwm_file_name, sizeof(pwm_file_name), 0, sizeof(pwm_file_name));

    if (PWM1 == pwmChannel)
    {
        (void)sprintf_s(pwm_file_name, sizeof(pwm_file_name), "%s/pwm0/enable", PWM1_PEX);
    }
    else if (PWM0 == pwmChannel)
    {
        (void)sprintf_s(pwm_file_name, sizeof(pwm_file_name), "%s/pwm0/enable", PWM0_PEX);
    }
    else
    {
        printf("[%s] PWM WRONG CHANNEL\n", __func__);
        return PWM_WRONOG_CHANNEL;
    }

    if (access(pwm_file_name, F_OK) != 0)
    {
        printf("[%s] PWM ENABLE FILE NOT EXIST\n", __func__);
        return PWM_FILE_NOT_EXIST;
    }

    (void)snprintf_s(buffer, sizeof(buffer), sizeof(buffer), "echo %d > %s", isEnable, pwm_file_name);
    system(buffer);

    return 0;
}

int get_pwm_period(int pwmChannel)
{
    int ret = 0;
    char pwm_file_name[128] = {0};
    (void)memset_s(pwm_file_name, sizeof(pwm_file_name), 0, sizeof(pwm_file_name));

    if (PWM1 == pwmChannel)
    {
        (void)sprintf_s(pwm_file_name, sizeof(pwm_file_name), "%s/pwm0/period", PWM1_PEX);
    }
    else if (PWM0 == pwmChannel)
    {
        (void)sprintf_s(pwm_file_name, sizeof(pwm_file_name), "%s/pwm0/period", PWM0_PEX);
    }
    else
    {
        printf("[%s] PWM WRONG CHANNEL\n", __func__);
        return PWM_WRONOG_CHANNEL;
    }

    if (access(pwm_file_name, F_OK) != 0)
    {
        printf("[%s] PWM PERIOD FILE NOT EXIST\n", __func__);
        return PWM_FILE_NOT_EXIST;
    }

    FILE *fp = NULL;
    char buffer[32] = {0};
    (void)memset_s(buffer, sizeof(buffer), 0, sizeof(buffer));
    fp = fopen(pwm_file_name, "r");
    if (!fp)
    {
        printf("[%s] Failed to open period file!\n", __func__);
        return PWM_FILE_NOT_EXIST;
    }
    (void)fread(buffer, sizeof(buffer), 1, fp);
    (void)fclose(fp);
    fp = NULL;

    ret = atoi(buffer);
    return ret;
}

int get_pwm_dutyCycle(int pwmChannel)
{
    int ret = 0;
    char pwm_file_name[128] = {0};
    (void)memset_s(pwm_file_name, sizeof(pwm_file_name), 0, sizeof(pwm_file_name));
    if (PWM1 == pwmChannel)
    {
        (void)sprintf_s(pwm_file_name, sizeof(pwm_file_name), "%s/pwm0/duty_cycle", PWM1_PEX);
    }
    else if (PWM0 == pwmChannel)
    {
        (void)sprintf_s(pwm_file_name, sizeof(pwm_file_name), "%s/pwm0/duty_cycle", PWM0_PEX);
    }
    else
    {
        printf("[%s] PWM WRONG CHANNEL\n", __func__);
        return PWM_WRONOG_CHANNEL;
    }

    if (access(pwm_file_name, F_OK) != 0)
    {
        printf("[%s] PWM DUTY_CYCLE FILE NOT EXIST\n", __func__);
        return PWM_FILE_NOT_EXIST;
    }

    FILE *fp = NULL;
    char buffer[32] = {0};
    (void)memset_s(buffer, sizeof(buffer), 0, sizeof(buffer));
    fp = fopen(pwm_file_name, "r");
    if (!fp)
    {
        printf("[%s] Failed to open duty_cycle file!\n", __func__);
        return PWM_FILE_NOT_EXIST;
    }
    (void)fread(buffer, sizeof(buffer), 1, fp);
    (void)fclose(fp);
    fp = NULL;

    ret = atoi(buffer);
    return ret;
}

int get_pwm_polarity(int pwmChannel)
{
    int ret = 0;
    char pwm_file_name[128] = {0};
    (void)memset_s(pwm_file_name, sizeof(pwm_file_name), 0, sizeof(pwm_file_name));

    if (PWM1 == pwmChannel)
    {
        (void)sprintf_s(pwm_file_name, sizeof(pwm_file_name), "%s/pwm0/polarity", PWM1_PEX);
    }
    else if (PWM0 == pwmChannel)
    {
        (void)sprintf_s(pwm_file_name, sizeof(pwm_file_name), "%s/pwm0/polarity", PWM0_PEX);
    }
    else
    {
        printf("[%s] PWM WRONG CHANNEL\n", __func__);
        return PWM_WRONOG_CHANNEL;
    }

    if (access(pwm_file_name, F_OK) != 0)
    {
        printf("[%s] PWM POLARITY FILE NOT EXIST\n", __func__);
        return PWM_FILE_NOT_EXIST;
    }

    FILE *fp = NULL;
    char buffer[32] = {0};
    (void)memset_s(buffer, sizeof(buffer), 0, sizeof(buffer));
    fp = fopen(pwm_file_name, "r");
    if (!fp)
    {
        printf("[%s] Failed to open polarity file!\n", __func__);
        return PWM_FILE_NOT_EXIST;
    }
    (void)fread(buffer, sizeof(buffer), 1, fp);
    (void)fclose(fp);
    fp = NULL;

    if (strstr(buffer, "normal") != NULL)
    {
        ret = PWM_POLARITY_NORMAL;
    }
    else if (strstr(buffer, "inversed") != NULL)
    {
        ret = PWM_POLARITY_INVERSED;
    }
    else
    {
        printf("[%s] PWM ERROR\n", __func__);
        ret = PWM_ERR;
    }

    return ret;
}

int is_pwm_enabled(int pwmChannel)
{
    int ret = 0;
    char pwm_file_name[128] = {0};
    (void)memset_s(pwm_file_name, sizeof(pwm_file_name), 0, sizeof(pwm_file_name));

    if (PWM1 == pwmChannel)
    {
        (void)sprintf_s(pwm_file_name, sizeof(pwm_file_name), "%s/pwm0/enable", PWM1_PEX);
    }
    else if (PWM0 == pwmChannel)
    {
        (void)sprintf_s(pwm_file_name, sizeof(pwm_file_name), "%s/pwm0/enable", PWM0_PEX);
    }
    else
    {
        printf("[%s] PWM WRONG CHANNEL\n", __func__);
        return PWM_WRONOG_CHANNEL;
    }

    if (access(pwm_file_name, F_OK) != 0)
    {
        printf("[%s] PWM ENABLE FILE NOT EXIST\n", __func__);
        return PWM_FILE_NOT_EXIST;
    }

    FILE *fp = NULL;
    char buffer[32] = {0};
    (void)memset_s(buffer, sizeof(buffer), 0, sizeof(buffer));
    fp = fopen(pwm_file_name, "r");
    if (!fp)
    {
        printf("[%s] Failed to open enable file!\n", __func__);
        return PWM_FILE_NOT_EXIST;
    }
    (void)fread(buffer, sizeof(buffer), 1, fp);
    (void)fclose(fp);
    fp = NULL;

    ret = atoi(buffer);
    return ret;
}
