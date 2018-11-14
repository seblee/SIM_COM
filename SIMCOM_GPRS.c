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
#ifndef SIMCOM_NET_log
#define SIMCOM_NET_log(N, ...) uart_printf("####[SIMCOM_CCH %s:%4d] " N "\r\n", __FILE__, __LINE__, ##__VA_ARGS__)
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
    u8 *pData;
    u32 cnt;
    result = SIMCOM_SetParametersReturnBool(pHandle, "AT+CCHMODE=1", SIMCOM_DEFAULT_RETRY, 200, "\r\n设置SIM7000C CCHMODE 透传模式失败!\r\n");
    if ((bool)result == FALSE)
        return result;
    SIMCOM_NET_log("设置透传模式成功");

    result = SIMCOM_CCHSTART(pHandle);
    if ((bool)result == FALSE)
        return result;
    SIMCOM_NET_log("启动通道成功");

    rt_snprintf(buff, sizeof(buff), "AT+CCHOPEN=0,\"%s\",%d,2", host, port);
    SIMCOM_NET_log("buff:%s", buff);
    SIMCOM_SendAT(pHandle, buff);
    pHandle->pClearRxData();

    //CONNECT 115200
    if (AT_RETURN_OK == SIMCOM_GetATResp(pHandle, &pData, &cnt, "CONNECT", 10, 200)) //等待响应,超时200MS
    {
        SIMCOM_NET_log("CONNECT 115200");
        result = TRUE;
    }
    else
        result = FALSE;
    return result;
}
/**
 ****************************************************************************
 * @Function : bool SIMCOM_CCHSTART(SIMCOM_HANDLE*pHandle)
 * @File     : SIMCOM_GPRS.c
 * @Program  : :handle
 * @Created  : 2018-10-09 by seblee
 * @Brief    : 
 * @Version  : V1.0
**/
bool SIMCOM_CCHSTART(SIMCOM_HANDLE *pHandle)
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

/**
 ****************************************************************************
 * @Function : bool SIMCOM_NETOPEN(SIMCOM_HANDLE *pHandle)
 * @File     : SIMCOM_GPRS.c
 * @Program  : none
 * @Created  : 2018-10-16 by seblee
 * @Brief    : 
 * @Version  : V1.0
**/
bool SIMCOM_NETOPEN(SIMCOM_HANDLE *pHandle)
{
    u32 cnt;
    char *p;
    u8 retry = SIMCOM_DEFAULT_RETRY; //重试次数
    u8 *pData;
    int status;

    do
    {
        //+CREG: 0,1
        SIMCOM_SendAT(pHandle, "AT+NETOPEN");                                       //发送"AT+NETOPEN",获取网络注册状态
        pHandle->pClearRxData();                                                    //清除接收计数器
        if (AT_RETURN_OK == SIMCOM_GetATResp(pHandle, &pData, &cnt, "OK", 20, 200)) //等待响应,超时200MS
        {
            // "+NETOPEN: 1"
            p = strstr((const char *)pData, "+NETOPEN:"); //搜索字符"+NETOPEN:"
            if (p != NULL)                                //搜索成功
            {
                status = GSM_StringToDec(&p[10], 1);
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

/**
 ****************************************************************************
 * @Function : bool SIMCOM_IPADDR(SIMCOM_HANDLE *pHandle)
 * @File     : SIMCOM_GPRS.c
 * @Program  : handle
 * @Created  : 2018-10-16 by seblee
 * @Brief    : get moudle ip address
 * @Version  : V1.0
**/
bool SIMCOM_IPADDR(SIMCOM_HANDLE *pHandle)
{
    u32 cnt;
    char *p;
    u8 retry = SIMCOM_DEFAULT_RETRY; //重试次数
    u8 *pData;
    do
    {
        //+IPADDR: 100.106.141.83
        SIMCOM_SendAT(pHandle, "AT+IPADDR");                                        //发送"AT+IPADDR",获取网络注册状态
        pHandle->pClearRxData();                                                    //清除接收计数器
        if (AT_RETURN_OK == SIMCOM_GetATResp(pHandle, &pData, &cnt, "OK", 20, 200)) //等待响应,超时200MS
        {
            // +IPADDR: 100.106.141.83
            p = strstr((const char *)pData, "+IPADDR:"); //搜索字符"+IPADDR:"
            if (p != NULL)                               //搜索成功
                SIMCOM_NET_log("+IPADDR:%s", p + 8);
            return TRUE;
        }
        SIMCOM_Ready(pHandle);   //等待就绪
        pHandle->pDelayMS(1000); //失败延时1秒后重试
        retry--;
    } while (retry);
    SIMCOM_TestAT(pHandle, 2);
    return FALSE;
}

/**
 ****************************************************************************
 * @Function : bool SIMCOM_CIPNETWORK(SIMCOM_HANDLE *pHandle, bool Transparent_mode, const char *host, int port)
 * @File     : SIMCOM_GPRS.c
 * @Program  : none
 * @Created  : 2018-10-16 by seblee
 * @Brief    : 
 * @Version  : V1.0
**/
bool SIMCOM_CIPNETWORK(SIMCOM_HANDLE *pHandle, bool Transparent_mode, const char *host, int port)
{
    u32 cnt;
    char buff[200];
    u8 retry = SIMCOM_DEFAULT_RETRY; //重试次数
    u8 *pData;
    bool result;
    if (pHandle->s_isInitStatus == FALSE) //模块没有初始化
        return FALSE;
    if (Transparent_mode == FALSE)
        result = SIMCOM_SetParametersReturnBool(pHandle, "AT+CIPMODE=0", SIMCOM_DEFAULT_RETRY, 110, "\r\n关闭透传模式失败!\r\n");
    else if (Transparent_mode == TRUE)
        result = SIMCOM_SetParametersReturnBool(pHandle, "AT+CIPMODE=1", SIMCOM_DEFAULT_RETRY, 110, "\r\n设置透传模式失败!\r\n");
    if (result == FALSE)
    {
        SIMCOM_NET_log("CIPMODE设置失败");
        return result;
    }
    result = SIMCOM_NETOPEN(pHandle);
    if (result == FALSE)
    {
        SIMCOM_NET_log("NETOPEN设置失败");
        return result;
    }
    result = SIMCOM_IPADDR(pHandle);
    if (result == FALSE)
    {
        SIMCOM_NET_log("IPADDR获取失败");
        return result;
    }
    //only <link_num>=0 is allowed to operate with transparent mode.
    rt_snprintf(buff, sizeof(buff), "AT+CIPOPEN=0,\"TCP\",\"%s\",%d", host, port);
    SIMCOM_NET_log("buff:%s", buff);
    SIMCOM_SendAT(pHandle, buff);
    pHandle->pClearRxData();
    //CONNECT 115200
    do
    {
        if (AT_RETURN_OK == SIMCOM_GetATResp(pHandle, &pData, &cnt, "CONNECT", 10, 500)) //等待响应,超时200MS
        {
            //CONNECT 115200
            return TRUE;
        }
        pHandle->pDelayMS(100); //失败延时1秒后重试
        retry--;
    } while (retry);

    result = FALSE;
    return result;
}

/**
 ****************************************************************************
 * @Function : bool SIMCOM_CHTTPSSTART(SIMCOM_HANDLE *pHandle)
 * @File     : SIMCOM_GPRS.c
 * @Program  : handle
 * @Created  : 2018-10-18 by seblee
 * @Brief    : Acquire HTTPS stack
 * @Version  : V1.0
**/
bool SIMCOM_CHTTPSSTART(SIMCOM_HANDLE *pHandle)
{
    u32 cnt;
    char *p;
    u8 retry = SIMCOM_DEFAULT_RETRY; //重试次数
    u8 *pData;
    int status;

    do
    {
        //+CHTTPSSTART: 0
        SIMCOM_SendAT(pHandle, "AT+CHTTPSSTART");                                   //发送"AT+CGREG?",获取网络注册状态
        pHandle->pClearRxData();                                                    //清除接收计数器
        if (AT_RETURN_OK == SIMCOM_GetATResp(pHandle, &pData, &cnt, "OK", 20, 200)) //等待响应,超时200MS
        {
            //+CHTTPSSTART: 0
            p = strstr((const char *)pData, "+CHTTPSSTART: "); //搜索字符"+CHTTPSSTART:"
            if (p != NULL)                                     //搜索成功
            {
                status = GSM_StringToDec(&p[rt_strlen("+CHTTPSSTART: ")], 1);
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
    return SIMCOM_NET_ERROR;
}

/**
 ****************************************************************************
 * @Function : bool SIMCOM_COMMAND_ACK(SIMCOM_HANDLE *pHandle, const char *AT_command,
 *                                     const char *pKeyword, const char *pParaword, int *err)
 * @File     : SIMCOM_GPRS.c
 * @Program  : pHandle:module info handle
 *             AT_command:the command to send
 *             pKeyword:back key word
 *             pParaword:back Para key word
 * @Created  : 2018-10-18 by seblee
 * @Brief    : send cmd back ok like this below
 * [OK]
 * []
 * [+CNETSTART: <err>]
 * @Version  : V1.0
**/
bool SIMCOM_COMMAND_ACK(SIMCOM_HANDLE *pHandle, const char *AT_command,
                        const char *pKeyword, const char *pParaword, int *err)
{
    u32 cnt;
    char *p;
    u8 retry = SIMCOM_DEFAULT_RETRY; //重试次数
    u8 *pData;
    *err = -1;
    do
    {
        //+CHTTPSSTART: 0
        SIMCOM_SendAT(pHandle, (char *)AT_command);                                     //发送"AT+CGREG?",获取网络注册状态
        pHandle->pClearRxData();                                                        //清除接收计数器
        if (AT_RETURN_OK == SIMCOM_GetATResp(pHandle, &pData, &cnt, pKeyword, 20, 200)) //等待响应,超时200MS
        {
            //+CHTTPSSTART: 0
            if (pParaword)
            {
                p = strstr((const char *)pData, pParaword); //搜索字符"+CHTTPSSTART:"
                if (p != NULL)                              //搜索成功
                {
                    *err = GSM_StringToDec(&p[rt_strlen(pParaword)], 1);
                    SIMCOM_TestAT(pHandle, 1);
                }
            }
            return TRUE;
        }

        SIMCOM_Ready(pHandle);   //等待就绪
        pHandle->pDelayMS(1000); //失败延时1秒后重试
        retry--;
    } while (retry);

    SIMCOM_TestAT(pHandle, 2);
    return FALSE;
}
