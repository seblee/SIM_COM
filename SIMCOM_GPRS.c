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
    result = SIMCOM_SetParametersReturnBool(pHandle, "AT+CCHMODE=1", SIMCOM_DEFAULT_RETRY, 200, "\r\n设置SIM7000C CCHMODE 透传模式失败!\r\n");
    if (result == FALSE)
        return result;
    result = SIMCOM_SetParametersReturnBool(pHandle, "AT+CCHSTART", SIMCOM_DEFAULT_RETRY, 200, "\r\n设置SIM7000C CCH 启动失败!\r\n");
    if (result == FALSE)
        return result;

    SIMCOM_SendAT(pHandle, "AT+CGMM");
    pHandle->pClearRxData();
    if (AT_RETURN_OK == SIMCOM_GetATResp(pHandle, &pData, &cnt, "OK", 10, 200)) //等待响应,超时200MS
    {
        if (strstr((char *)pData, "SIM900") != NULL)
            return SIMCOM_SIM900;
        else if (strstr((char *)pData, "SIM800") != NULL)
            return SIMCOM_SIM800;
        else if (strstr((char *)pData, "SIM2000") != NULL)
            return SIMCOM_SIM2000;
        else if (strstr((char *)pData, "SIM7600") != NULL)
            return SIMCOM_SIM7600;
        else if (strstr((char *)pData, "SIMCOM_SIM868") != NULL)
            return SIMCOM_SIM868;
        else if (strstr((char *)pData, "SIMCOM_SIM7000C") != NULL)
            return SIMCOM_SIM7000C;
        else if (strstr((char *)pData, "LYNQ_L700") != NULL)
            return LYNQ_L700;
        else
        {
            uart_printf("未知通信模块：%s\r\n", pData);
            return SIMCOM_INVALID;
        }
    }
}

/*************************************************************************************************************************
* 函数				:	SIMCOM_NETSTATUS SIM900_SetGSM_CCHMODE(SIMCOM_HANDLE *pHandle)
* 功能				:	获取GSM网络注册状态
* 参数				:	pHandle:句柄
* 返回				:	bool
* 依赖				:	底层
* 作者				:	cp1300@139.com
* 时间				:	2013-10-29
* 最后修改时间 		: 	2018-03-24
* 说明				: 	当网络注册后,可能被拒绝,如果被拒绝,获取网络注册状态会提示
						注册成功的,但是通过发送AT 后再去查询,会发现网络注册失败
*************************************************************************************************************************/

/**
 ****************************************************************************
 * @Function : void SIM900_CCHSTART(SIMCOM_HANDLE*pHandle)
 * @File     : SIMCOM_GPRS.c
 * @Program  : :handle
 * @Created  : 2018-10-09 by seblee
 * @Brief    : 
 * @Version  : V1.0
**/
void SIM900_CCHSTART(SIMCOM_HANDLE *pHandle)
{
}

bool SIM900_CCHSTART(SIMCOM_HANDLE *pHandle)
{
    u32 cnt;
    char *p;
    u8 retry = SIMCOM_DEFAULT_RETRY; //重试次数
    u8 *pData;
    SIMCOM_NETSTATUS status;

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
                status = (SIMCOM_NETSTATUS)GSM_StringToDec(&p[11], 1);
                SIMCOM_TestAT(pHandle, 1);
                return status;
            }
            break;
        }

        SIMCOM_Ready(pHandle);   //等待就绪
        pHandle->pDelayMS(1000); //失败延时1秒后重试
        retry--;
    } while (retry);

    SIMCOM_TestAT(pHandle, 2);
    return SIMCOM_NET_ERROR;
}