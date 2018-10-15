/**
 ****************************************************************************
 * @Warning :Without permission from the author,Not for commercial use
 * @File    :
 * @Author  :Seblee
 * @date    :2018-10-09 10:05:38
 * @version : V1.0.0
 *************************************************
 * @brief :
 ****************************************************************************
 * @Last Modified by: Seblee
 * @Last Modified time: 2018-10-09 11:26:52
 ****************************************************************************
**/

/* Private include -----------------------------------------------------------*/
#include "SIMCOM.h"
#include "SIMCOM_GPRS.h"
#include "SIMCOM_AT.h"
/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
#ifndef SIMCOM_CCH_log
#define SIMCOM_CCH_log(N, ...) uart_printf("####[SIMCOM_CCH %s:%4d] " N "\r\n", __FILE__, __LINE__, ##__VA_ARGS__)
#endif /* at_log(...) */
/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

/*----------------------------------------------------------------------------*/

/**
 ****************************************************************************
 * @Function : bool SIMCOM_CCH(SIMCOM_HANDLE *pHandle, const char *host, int port)
 * @File     : SIMCOM_GPRS.c
 * @Program  : none
 * @Created  : 2018-10-09 by seblee
 * @Brief    : Using Transparent Mode for Common Channel Service
 * @Version  : V1.0
**/
bool SIMCOM_CCH(SIMCOM_HANDLE *pHandle, const char *host, int port)
{
    bool result;
    char buff[200];
    result = SIMCOM_SetParametersReturnBool(pHandle, "AT+CCHMODE=1", SIMCOM_DEFAULT_RETRY, 200, "\r\n设置SIM7000C CCHMODE 透传模式失败!\r\n");
    if ((bool)result == FALSE)
        return result;
    SIMCOM_CCH_log("设置透传模式成功");

    result = SIM900_CCHSTART(pHandle);
    if ((bool)result == FALSE)
        return result;
    SIMCOM_CCH_log("启动通道成功");

    rt_snprintf(buff, sizeof(buff), "AT+CCHOPEN=0,\"%s\",%d,2", host, port);
    SIMCOM_CCH_log("buff:%s", buff);
    SIMCOM_SendAT(pHandle, buff);
    pHandle->pClearRxData();
    //CONNECT 115200
    if (AT_RETURN_OK == SIMCOM_GetATResp(pHandle, &pData, &cnt, "CONNECT", 10, 200)) //等待响应,超时200MS
    {
        SIMCOM_CCH_log("CONNECT 115200");
        result = TRUE;
    }
    else
        result = FALSE;
    return result;
}
/**
 ****************************************************************************
 * @Function : bool SIM900_CCHSTART(SIMCOM_HANDLE*pHandle)
 * @File     : SIMCOM_GPRS.c
 * @Program  : :handle
 * @Created  : 2018-10-09 by seblee
 * @Brief    : 
 * @Version  : V1.0
**/
bool SIM900_CCHSTART(SIMCOM_HANDLE *pHandle)
{
    u32 cnt;
    char *p;
    u8 retry = SIMCOM_DEFAULT_RETRY; //重试次数
    u8 *pData;
    int status;

    do
    {
        //+CCHSTART: 0
        SIMCOM_SendAT(pHandle, "AT+CCHSTART");                                      //发送"AT+CREG?",获取网络注册状态
        pHandle->pClearRxData();                                                    //清除接收计数器
        if (AT_RETURN_OK == SIMCOM_GetATResp(pHandle, &pData, &cnt, "OK", 20, 200)) //等待响应,超时200MS
        {
            p = strstr((const char *)pData, "+CCHSTART:"); //搜索字符"+CCHSTART:"
            if (p != NULL)                                 //搜索成功
            {
                status = GSM_StringToDec(&p[11], 1);
                SIMCOM_TestAT(pHandle, 1);
                if (status == 0)
                    return TRUE;
            }
            break;
        }

        SIMCOM_Ready(pHandle);   //等待就绪
        pHandle->pDelayMS(1000); //失败延时1秒后重试
        retry--;
    } while (retry);

    SIMCOM_TestAT(pHandle, 2);
    return FALSE;
}