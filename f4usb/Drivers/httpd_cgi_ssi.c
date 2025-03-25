/**
 ****************************************************************************************************
 * @file        httpd_cgi_ssi.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2021-12-02
 * @brief       http cgi代码
 * @license     Copyright (c) 2020-2032, 广州市星翼电子科技有限公司
 ****************************************************************************************************
 * @attention
 *
 * 实验平台:正点原子 探索者 F407开发板
 * 在线视频:www.yuanzige.com
 * 技术论坛:www.openedv.com
 * 公司网址:www.alientek.com
 * 购买地址:openedv.taobao.com
 *
 * 修改说明
 * V1.0 20211202
 * 第一次发布
 *
 ****************************************************************************************************
 */
 
#include "lwip/debug.h"
#include "httpd.h"
#include "lwip/tcp.h"
#include "fs.h"

#include <string.h>
#include <stdlib.h>


#define NUM_CONFIG_CGI_URIS     2       /* CGI的URI数量 */
#define NUM_CONFIG_SSI_TAGS     4       /* SSI的TAG数量 */


/* 控制LED和BEEP的CGI handler */
const char *LEDS_CGI_Handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]);
const char *BEEP_CGI_Handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]);

static const char *ppcTAGs[] = /* SSI的Tag */
{
    "t", /* ADC值 */
    "w", /* 温度值 */
    "h", /* 时间 */
    "y"  /* 日期 */
};

static const tCGI ppcURLs[] = /* cgi程序 */
{
    {"/leds.cgi", LEDS_CGI_Handler},
    {"/beep.cgi", BEEP_CGI_Handler},
};

/**
 * @breif       当web客户端请求浏览器的时候,使用此函数被CGI handler调用
 * @param       pcToFind    :
 * @param       pcParam     :
 * @param       iNumParams  :
 * @retval      无
 */
static int FindCGIParameter(const char *pcToFind, char *pcParam[], int iNumParams)
{
    int iLoop;

    for (iLoop = 0; iLoop < iNumParams; iLoop++)
    {
        if (strcmp(pcToFind, pcParam[iLoop]) == 0)
        {
            return (iLoop);     /* 返回iLOOP */
        }
    }

    return (-1);
}

/**
 * @breif       SSIHandler中需要用到的处理ADC的函数
 * @param       pcInsert    :
 * @retval      无
 */
void ADC_Handler(char *pcInsert)
{
    char Digit1 = 0, Digit2 = 0, Digit3 = 0, Digit4 = 0;
    uint32_t ADCVal = 0;

    /* 获取ADC的值 */
//    ADCVal = adc_get_result_average(5, 10); /* 获取ADC1_CH5的电压值 */

    /* 转换为电压 ADCVval * 0.8mv */
    ADCVal = (uint32_t)(ADCVal * 0.8);

    Digit1 = ADCVal / 1000;
    Digit2 = (ADCVal - (Digit1 * 1000)) / 100 ;
    Digit3 = (ADCVal - ((Digit1 * 1000) + (Digit2 * 100))) / 10;
    Digit4 = ADCVal - ((Digit1 * 1000) + (Digit2 * 100) + (Digit3 * 10));

    /* 准备添加到html中的数据 */
    *pcInsert       = (char)(Digit1 + 0x30);
    *(pcInsert + 1) = (char)(Digit2 + 0x30);
    *(pcInsert + 2) = (char)(Digit3 + 0x30);
    *(pcInsert + 3) = (char)(Digit4 + 0x30);
}

/**
 * @breif       SSIHandler中需要用到的处理内部温度传感器的函数
 * @param       pcInsert    :
 * @retval      无
 */
void Temperate_Handler(char *pcInsert)
{
    char Digit1 = 0, Digit2 = 0, Digit3 = 0, Digit4 = 0, Digit5 = 0;
    short Temperate = 0;

    /* 获取内部温度值 */
//    Temperate = adc_get_temperature(); /* 获取温度值 此处扩大了100倍 */
    Digit1 = Temperate / 10000;
    Digit2 = (Temperate % 10000) / 1000;
    Digit3 = (Temperate % 1000) / 100 ;
    Digit4 = (Temperate % 100) / 10;
    Digit5 = Temperate % 10;

    /* 添加到html中的数据 */
    *pcInsert = (char)(Digit1 + 0x30);
    *(pcInsert + 1) = (char)(Digit2 + 0x30);
    *(pcInsert + 2) = (char)(Digit3 + 0x30);
    *(pcInsert + 3) = '.';
    *(pcInsert + 4) = (char)(Digit4 + 0x30);
    *(pcInsert + 5) = (char)(Digit5 + 0x30);
}

/**
 * @breif       SSIHandler中需要用到的处理RTC时间的函数
 * @param       pcInsert    :
 * @retval      无
 */
void RTCTime_Handler(char *pcInsert)
{
//    RTC_TimeTypeDef RTC_TimeStruct;
    uint8_t hour, min, sec;

//    HAL_RTC_GetTime(&g_rtc_handle, &RTC_TimeStruct, RTC_FORMAT_BIN);
//    hour = RTC_TimeStruct.Hours;
//    min = RTC_TimeStruct.Minutes;
//    sec = RTC_TimeStruct.Seconds;

    *pcInsert = (char)((hour / 10) + 0x30);
    *(pcInsert + 1) = (char)((hour % 10) + 0x30);
    *(pcInsert + 2) = ':';
    *(pcInsert + 3) = (char)((min / 10) + 0x30);
    *(pcInsert + 4) = (char)((min % 10) + 0x30);
    *(pcInsert + 5) = ':';
    *(pcInsert + 6) = (char)((sec / 10) + 0x30);
    *(pcInsert + 7) = (char)((sec % 10) + 0x30);
}

/**
 * @breif       SSIHandler中需要用到的处理RTC日期的函数
 * @param       pcInsert    :
 * @retval      无
 */
void RTCdate_Handler(char *pcInsert)
{
    uint8_t year, month, date, week;
//    RTC_DateTypeDef RTC_DateStruct;

//    HAL_RTC_GetDate(&g_rtc_handle, &RTC_DateStruct, RTC_FORMAT_BIN);
//    year = RTC_DateStruct.Year;
//    month = RTC_DateStruct.Month;
//    date = RTC_DateStruct.Date;
//    week = RTC_DateStruct.WeekDay;

    *pcInsert = '2';
    *(pcInsert + 1) = '0';
    *(pcInsert + 2) = (char)((year / 10) + 0x30);
    *(pcInsert + 3) = (char)((year % 10) + 0x30);
    *(pcInsert + 4) = '-';
    *(pcInsert + 5) = (char)((month / 10) + 0x30);
    *(pcInsert + 6) = (char)((month % 10) + 0x30);
    *(pcInsert + 7) = '-';
    *(pcInsert + 8) = (char)((date / 10) + 0x30);
    *(pcInsert + 9) = (char)((date % 10) + 0x30);
    *(pcInsert + 10) = ' ';
    *(pcInsert + 11) = 'w';
    *(pcInsert + 12) = 'e';
    *(pcInsert + 13) = 'e';
    *(pcInsert + 14) = 'k';
    *(pcInsert + 15) = ':';
    *(pcInsert + 16) = (char)(week + 0x30);
}

/**
 * @breif       SSI的Handler句柄
 * @param       iIndex      :
 * @param       pcInsert    :
 * @param       iInsertLen  :
 * @retval      无
 */
static u16_t SSIHandler(int iIndex, char *pcInsert, int iInsertLen)
{
    switch (iIndex)
    {
        case 0:
            ADC_Handler(pcInsert);
            break;

        case 1:
            Temperate_Handler(pcInsert);
            break;

        case 2:
            RTCTime_Handler(pcInsert);
            break;

        case 3:
            RTCdate_Handler(pcInsert);
            break;
    }

    return strlen(pcInsert);
}

/**
 * @breif       CGI LED控制句柄
 * @param       iIndex      : CGI句柄索引号
 * @param       iNumParams  :
 * @param       pcParam     :
 * @param       pcValue     :
 * @retval      无
 */
const char *LEDS_CGI_Handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
    uint8_t i = 0; /* 注意根据自己的GET的参数的多少来选择i值范围 */

    iIndex = FindCGIParameter("LED1", pcParam, iNumParams);   /* 找到led的索引号 */

    /* 只有一个CGI句柄 iIndex=0 */
    if (iIndex != -1)
    {
//        LED1(1);    /* 关闭所有的LED1灯 */

        for (i = 0; i < iNumParams; i++)                /* 检查CGI参数: example GET /leds.cgi?led=2&led=4 */
        {
            if (strcmp(pcParam[i], "LED1") == 0)        /* 检查参数"led" */
            {
                if (strcmp(pcValue[i], "LED1ON") == 0)  /* 改变LED1状态 */
                {
//                    LED1(0);    /* 打开LED1 */
                }
                else if (strcmp(pcValue[i], "LED1OFF") == 0)
                {
//                    LED1(1);    /* 关闭LED1 */
                }
            }
        }
    }

//    if (READ_LED1 == 0 && READ_BEEP == 0)
//    {
//        return "/STM32F407LED_ON_BEEP_OFF.shtml";   /* LED1开,BEEP关 */
//    }
//    else if (READ_LED1 == 0 && READ_BEEP == 1)
//    {
//        return "/STM32F407LED_ON_BEEP_ON.shtml";    /* LED1开,BEEP开 */
//    }
//    else if (READ_LED1 == 1 && READ_BEEP == 1)
//    {
//        return "/STM32F407LED_OFF_BEEP_ON.shtml";   /* LED1关,BEEP开 */
//    }
//    else
//    {
        return "/STM32F407LED_OFF_BEEP_OFF.shtml";  /*  LED1关,BEEP关 */
//    }
}

/**
 * @breif       BEEP的CGI控制句柄
 * @param       iIndex      : CGI句柄索引号
 * @param       iNumParams  :
 * @param       pcParam     :
 * @param       pcValue     :
 * @retval      无
 */
const char *BEEP_CGI_Handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
    uint8_t i = 0;

    iIndex = FindCGIParameter("BEEP", pcParam, iNumParams);   /* 找到BEEP的索引号 */

    if (iIndex != -1)   /* 找到BEEP索引号 */
    {
//        BEEP(0);        /* 关闭 */

        for (i = 0; i < iNumParams; i++)
        {
            if (strcmp(pcParam[i], "BEEP") == 0)             /* 查找CGI参数 */
            {
                if (strcmp(pcValue[i], "BEEPON") == 0)       /* 打开BEEP */
                {
//                    BEEP(1);
                }
                else if (strcmp(pcValue[i], "BEEPOFF") == 0) /* 关闭BEEP */
                {
//                    BEEP(0);
                }
            }
        }
    }

//    if (READ_LED1 == 0 && READ_BEEP == 0)
//    {
//        return "/STM32F407LED_ON_BEEP_OFF.shtml";   /* LED1开,BEEP关 */
//    }
//    else if (READ_LED1 == 0 && READ_BEEP == 1)
//    {
//        return "/STM32F407LED_ON_BEEP_ON.shtml";    /* LED1开,BEEP开 */
//    }
//    else if (READ_LED1 == 1 && READ_BEEP == 1)
//    {
//        return "/STM32F407LED_OFF_BEEP_ON.shtml";   /* LED1关,BEEP开 */
//    }
//    else
//    {
        return "/STM32F407LED_OFF_BEEP_OFF.shtml";  /* LED1关,BEEP关 */
//    }
}

/**
 * @breif       SSI句柄初始化
 * @param       无
 * @retval      无
 */
void httpd_ssi_init(void)
{
    http_set_ssi_handler(SSIHandler, ppcTAGs, NUM_CONFIG_SSI_TAGS);   /* 配置SSI句柄 */
}

/**
 * @breif       CGI句柄初始化
 * @param       无
 * @retval      无
 */
void httpd_cgi_init(void)
{
    http_set_cgi_handlers(ppcURLs, NUM_CONFIG_CGI_URIS);      /* 配置CGI句柄 */
}


