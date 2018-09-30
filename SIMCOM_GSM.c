/*************************************************************************************************************
 * 文件名:			SIMCOM_GSM.c
 * 功能:			SIMCOM GSM相关接口
 * 作者:			cp1300@139.com
 * 创建时间:		2015-02-15
 * 最后修改时间:	2018-03-23
 * 详细:			
*************************************************************************************************************/
#include "system.h"
#include "usart.h"
#include "SIMCOM_GSM.h"
#include "SIMCOM_AT.h"
#include "string.h"
#include "SIMCOM.h"
#include <stdlib.h>

bool g_SIMC0M_GSM_Debug = TRUE; //底层AT指令调试状态

//调试开关
#define SIMCOM_GSM_DBUG 1
#if SIMCOM_GSM_DBUG
#include "system.h"
#define SIMCOM_GSM_debug(format, ...)           \
    {                                           \
        if (g_SIMC0M_GSM_Debug)                 \
        {                                       \
            uart_printf(format, ##__VA_ARGS__); \
        }                                       \
    }
#else
#define SIMCOM_GSM_debug(format, ...) /\
/
#endif                                //SIMCOM_GSM_DBUG

/*************************************************************************************************************************
* 函数			:	bool SIMCOM_NetworkConfig(SIMCOM_HANDLE *pHandle, SIMCOM_MODE_TYPE ModeType, SIMCOM_SIM_SELECT SIM_Select)
* 功能			:	SIMCOM网络配置
* 参数			:	pHandle：句柄；ModeType：通信模块型号；SIM_Select:SIM卡选择；
* 返回			:	TRUE:成功,FALSE:失败	
* 依赖			:	底层
* 作者			:	cp1300@139.com
* 时间			:	2018-01-17
* 最后修改时间	: 	2018-03-24
 * 详细			:
*************************************************************************************************************************/
bool SIMCOM_NetworkConfig(SIMCOM_HANDLE *pHandle, SIMCOM_MODE_TYPE ModeType, NETWORK_CONFIG_TYPE *pConfig)
{
    char buff[16];

    pConfig->ModeType = ModeType;    //记录通信模块型号
    if (ModeType == SIMCOM_SIM7000C) //SIM7000C需要选择工作模式
    {
        switch (pConfig->NB_EnableMode)
        {
        case 0: //GSM模式
        {
            uart_printf("[DTU]设置GSM网络模式!\r\n");
            if (SIMCOM_SetParametersReturnBool(pHandle, "AT+CNMP=13", SIMCOM_DEFAULT_RETRY, 200, "\r\n设置SIM7000C GSM模式失败!\r\n") == FALSE)
                return FALSE; //GSM模式
        }
        break;
        case 1: //NB模式
        {
            uart_printf("[DTU]设置NBIOT网络模式!\r\n");
            if (SIMCOM_SetParametersReturnBool(pHandle, "AT+CNMP=38", SIMCOM_DEFAULT_RETRY, 200, "\r\n设置SIM7000C LTE NB模式失败!\r\n") == FALSE)
                return FALSE; //LTE only（使用NB-IOT网络时CNMP需要设置为38）
            //CAT NB模式设置
            switch (pConfig->ModeType)
            {
            case CAT_M_MODE: //CAT模式
            {
                sprintf(buff, "AT+CMNB=%d", 1); //cat模式
            }
            break;
            default:
            {
                sprintf(buff, "AT+CMNB=%d", 2); //NBIOT模式
            }
            break;
            }
            if (SIMCOM_SetParametersReturnBool(pHandle, buff, SIMCOM_DEFAULT_RETRY, 200, "\r\n设置SIM7000C CAT NB模式失败!\r\n") == FALSE)
                return FALSE; //1: CAT-M 2: NB-IOT
            //扰码设置
            if (pConfig->isNB_ScarEnable) //开启扰码
            {
                sprintf(buff, "AT+NBSC=%d", 1);
            }
            else
            {
                sprintf(buff, "AT+NBSC=%d", 0);
            }
            if (SIMCOM_SetParametersReturnBool(pHandle, buff, SIMCOM_DEFAULT_RETRY, 200, "\r\n设置SIM7000C NB 扰码模式失败!\r\n") == FALSE)
                return FALSE;
        }
        break;
        default:
            return TRUE; //忽略，无需设置
        }
    }
    return TRUE;
}

/*************************************************************************************************************************
* 函数				:	SIM_CARD_STATUS SIMCOM_GetCPIN(SIMCOM_HANDLE *pHandle)
* 功能				:	获取SIM卡状态
* 参数				:	无
* 返回				:	FALSE:失败;TRUE:成功
* 依赖				:	底层
* 作者				:	cp1300@139.com
* 时间				:	2014-06-26
* 最后修改时间 		: 	2014-06-26
* 说明				: 	2017-09-05 ： 增加SIM卡状态为3种状态
*************************************************************************************************************************/
SIM_CARD_STATUS SIMCOM_GetCPIN(SIMCOM_HANDLE *pHandle)
{
    u32 cnt;
    char *p;
    u8 retry = SIMCOM_DEFAULT_RETRY; //重试次数
    int status;
    u8 *pData;

    do
    {
        //+CPIN: READY
        SIMCOM_SendAT(pHandle, "AT+CPIN?");
        pHandle->pClearRxData();                                         //清除接收计数器
        status = SIMCOM_GetATResp(pHandle, &pData, &cnt, "OK", 20, 200); //等待响应,超时200MS
        if (AT_RETURN_OK == status)                                      //返回OK
        {
            p = strstr((const char *)pData, "+CPIN: READY"); //搜索字符"+CPIN: READY"
            if (p != NULL)                                   //搜索成功
            {
                return SIM_READY; //SIM卡就绪
            }
            break;
        }
        else if (AT_RETURN_ERROR == status) //返回ERROR
        {
            p = strstr((const char *)pData, "ERROR"); //搜索卡未准备就绪标志
            if (p != NULL)                            //搜索成功
            {
                return SIM_NOT_READY; //SIM卡未就绪
            }
            break;
        }

        SIMCOM_Ready(pHandle);   //等待就绪
        pHandle->pDelayMS(1000); //失败延时1秒后重试
        retry--;
    } while (retry);

    return SIM_UNKNOWN; //SIM卡未知
}

/*************************************************************************************************************************
* 函数				:	SIMCOM_NETSTATUS SIM900_GetGSMNetworkStatus(SIMCOM_HANDLE *pHandle)
* 功能				:	获取GSM网络注册状态
* 参数				:	pHandle:句柄
* 返回				:	SIMCOM_NETSTATUS
* 依赖				:	底层
* 作者				:	cp1300@139.com
* 时间				:	2013-10-29
* 最后修改时间 		: 	2018-03-24
* 说明				: 	当网络注册后,可能被拒绝,如果被拒绝,获取网络注册状态会提示
						注册成功的,但是通过发送AT 后再去查询,会发现网络注册失败
*************************************************************************************************************************/
SIMCOM_NETSTATUS SIM900_GetGSMNetworkStatus(SIMCOM_HANDLE *pHandle)
{
    u32 cnt;
    char *p;
    u8 retry = SIMCOM_DEFAULT_RETRY; //重试次数
    u8 *pData;

    do
    {
        //+CREG: 0,1
        SIMCOM_SendAT(pHandle, "AT+CREG?");                                         //发送"AT+CREG?",获取网络注册状态
        pHandle->pClearRxData();                                                    //清除接收计数器
        if (AT_RETURN_OK == SIMCOM_GetATResp(pHandle, &pData, &cnt, "OK", 20, 200)) //等待响应,超时200MS
        {
            p = strstr((const char *)pData, "+CREG:"); //搜索字符"+CREG:"
            if (p != NULL)                             //搜索成功
            {
                SIMCOM_TestAT(pHandle, 1);
                return (SIMCOM_NETSTATUS)GSM_StringToDec(&p[9], 1);
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

/*************************************************************************************************************************
* 函数				:	SIMCOM_NETSTATUS SIMCOM_GetDataNetworkStatus(SIMCOM_HANDLE *pHandle)
* 功能				:	获取数据网络注册状态
* 参数				:	pHandle:句柄
* 返回				:	SIMCOM_NETSTATUS
* 依赖				:	底层
* 作者				:	cp1300@139.com
* 时间				:	2013-10-29
* 最后修改时间 		: 	2018-03-24
* 说明				: 	用于获取NB数据网络或GPRS数据网络注册状态
*************************************************************************************************************************/
SIMCOM_NETSTATUS SIMCOM_GetDataNetworkStatus(SIMCOM_HANDLE *pHandle)
{
    u32 cnt;
    char *p;
    u8 retry = SIMCOM_DEFAULT_RETRY; //重试次数
    u8 *pData;

    do
    {
        //+CGREG: 0,1
        SIMCOM_SendAT(pHandle, "AT+CGREG?");                                        //发送"AT+CGREG?",获取网络注册状态
        pHandle->pClearRxData();                                                    //清除接收计数器
        if (AT_RETURN_OK == SIMCOM_GetATResp(pHandle, &pData, &cnt, "OK", 20, 200)) //等待响应,超时200MS
        {
            p = strstr((const char *)pData, "+CGREG:"); //搜索字符"+CGREG:"
            if (p != NULL)                              //搜索成功
            {
                SIMCOM_TestAT(pHandle, 1);
                return (SIMCOM_NETSTATUS)GSM_StringToDec(&p[10], 1);
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

/*************************************************************************************************************************
* 函数				:	bool SIM900_SetGPRS_PackDatatSize(SIMCOM_HANDLE *pHandle)
* 功能				:	设置SIM900/SIM800 GPRS发送数据缓冲区
* 参数				:	pHandle：句柄
* 返回				:	FALSE:失败;TRUE:成功
* 依赖				:	底层
* 作者				:	cp1300@139.com
* 时间				:	2014-09-11
* 最后修改时间 		: 	2014-09-11
* 说明				: 	按照最大数据包1460B设置
*************************************************************************************************************************/
bool SIM900_SetGPRS_PackDatatSize(SIMCOM_HANDLE *pHandle)
{
    char buff[36];

    //先开启透传模式才能设置
    SIMCOM_SetParametersReturnBool(pHandle, "AT+CIPMODE=1", SIMCOM_DEFAULT_RETRY, 2000, "开启透传模式失败!\r\n"); //开启透传模式

    //设置GPRS传输数据包大小
    //AT+CIPCCFG=3,2,1024,1 //设置透传参数 //3-重传次数为3次,2-等待数据输入时间为 //2*200ms,1024-数据缓冲区为1024个字节 //1-支持转义退出透传
    sprintf(buff, "AT+CIPCCFG=3,2,%d,1", 1460);
    return SIMCOM_SetParametersReturnBool(pHandle, buff, SIMCOM_DEFAULT_RETRY, 200, "GPRS发送数据缓冲区设置失败!\r\n"); //发送
}

/*************************************************************************************************************************
* 函数				:	bool SIMCOM_ModuleInit(SIMCOM_HANDLE *pHandle)
* 功能				:	初始化SIMCOM模块基本配置（不允许失败）
* 参数				:	pHandle:句柄
* 返回				:	FALSE:初始化失败;TRUE:初始化成功
* 依赖				:	底层
* 作者				:	cp1300@139.com
* 时间				:	2013-10-16
* 最后修改时间 		: 	2018-03-24
* 说明				: 	必须先上电，并获取模块型号，根据不同的型号模块分别进行初始化
*************************************************************************************************************************/
bool SIMCOM_ModuleInit(SIMCOM_HANDLE *pHandle)
{
    u8 retry = 5; //重试次数

    pHandle->pSetDTR_Pin(SIMCOM_L_LEVEL); //DTR=0,退出低功耗模式
    //检测模块存在,并保证通信正常
    SIMCOM_Ready(pHandle);
    SIMCOM_TestAT(pHandle, 20);

    switch (pHandle->SimcomModeType) //不同的芯片存在不一样的初始化
    {
    case SIMCOM_SIM2000: //SIM2000需要先关闭URC,否则会提示Call Ready
    {
        SIMCOM_SetParametersReturnBool(pHandle, "AT+CIURC=0", SIMCOM_DEFAULT_RETRY, 110, "\r\n关闭Call Ready显示失败!\r\n");
    }
    break;
    default:
        break;
    }
    //设置关闭回显
    if (SIMCOM_SetParametersReturnBool(pHandle, "ATE 0", SIMCOM_DEFAULT_RETRY, 110, "\r\n关闭AT回显模式失败!\r\n") == FALSE)
    {
        return FALSE;
    }
    //设置短消息格式为PDU格式
    if (SIMCOM_SetParametersReturnBool(pHandle, "AT+CMGF=0", SIMCOM_DEFAULT_RETRY, 110, "\r\n设置短消息格式为PDU格式失败!\r\n") == FALSE)
    {
        uart_printf("\r\n设置DCD功能模式失败!\r\n");
        return FALSE;
    }
    //设置DCD功能模式，DCD线只在数据载波存在时为ON。
    if (SIMCOM_SetParametersReturnBool(pHandle, "AT&C1", SIMCOM_DEFAULT_RETRY, 110, "\r\n设置DCD功能模式失败!\r\n") == FALSE)
    {
        uart_printf("\r\n设置DCD功能模式失败!\r\n");
        //return FALSE;
    }
    //设置 DTR 功能模式,DTR 由ON至OFF：TA在保持当前数据通话的同时，切换至命令模式
    if (SIMCOM_SetParametersReturnBool(pHandle, "AT&D1", SIMCOM_DEFAULT_RETRY, 110, "\r\n设置DTR功能模式失败!\r\n") == FALSE)
    {
        uart_printf("\r\n设置DTR功能模式失败!\r\n");
        //return FALSE;
    }

    //	//使能RI引脚提示
    //	if(SIM900_SetParametersReturnBool("AT+CFGRI=1", SIMCOM_DEFAULT_RETRY, 11, "\r\n启动RI引脚提示失败!\r\n") == FALSE)
    //	{
    //		return FALSE;
    //	}

    //设置模块sleep模式使能//发送"AT+CSCLK",启动SLEEP模式;0:关闭;1:手动;2:自动空闲5S钟后休眠
    if (SIMCOM_SetParametersReturnBool(pHandle, "AT+CSCLK=1", SIMCOM_DEFAULT_RETRY, 110, "\r\n设置SLEEP失败!\r\n") == FALSE)
    {
        return FALSE;
    }

    //检查卡是否就绪
    retry = 8; //重试次数
    do
    {
        if (SIMCOM_GetCPIN(pHandle) == SIM_READY)
        {
            uart_printf("\r\nSIM卡准备就绪!\r\n");
            break;
        }
        else
        {
            uart_printf("\r\nSIM卡未准备就绪!\r\n");
        }

        SIMCOM_Ready(pHandle);   //等待就绪
        pHandle->pDelayMS(1000); //失败延时1秒后重试
        retry--;
    } while (retry);
    if (retry == 0)
    {
        uart_printf("\r\nSIM卡未准备就绪!\r\n");
        pHandle->pClearRxData(); //清除接收计数器
        return FALSE;
    }

    //	//上电删除所有短信
    //	retry = SIMCOM_DEFAULT_RETRY;						//重试次数
    //	do
    //	{
    //		if(SIM900_DelMultiSMS(DelSMS) == TRUE)//删除短信
    //		{
    //			//uart_printf("上电删除短信成功!\r\n");
    //			break;
    //		}
    //		SIM900_Ready();	//等待就绪
    //		retry --;
    //	}while(retry);
    //	if(retry == 0)
    //	{
    //		uart_printf("上电删除短信失败!\r\n");
    //		SIM900_ClearRxCnt();				//清除计数器
    //		return FALSE;
    //	}

    //2016-09-20：设置等待消息上报超时时间为1分钟，因为西宁项目卡出现超时情况
    switch (pHandle->SimcomModeType) //不同的芯片存在不一样的初始化
    {
    case SIMCOM_SIM800: //SIM800需要等待就绪时间长一些
    {
        retry = 65;
    }
    break;
    default:
        retry = 35;
        break;
    }

    //关闭新消息自动上报
    while (retry)
    {
        if (SIMCOM_SetParametersReturnBool(pHandle, "AT+CNMI=0", SIMCOM_DEFAULT_RETRY, 110, "\r\n关闭新消息自动上报失败!\r\n") == FALSE)
        {
            //return FALSE;
        }
        else
            break;
        pHandle->pDelayMS(1000); //延时1秒
        retry--;
    }
    if (retry == 0)
        return FALSE;

    switch (pHandle->SimcomModeType) //不同的芯片存在不一样的初始化
    {
    case LYNQ_L700:
        break;
    case SIMCOM_SIM7600:
    {
        //设置TCP收发相关
        retry = SIMCOM_DEFAULT_RETRY; //重试次数
        while (retry)
        {
            //设置重试次数为3次，并且发送延时为120ms
            if (SIMCOM_SetParametersReturnBool(pHandle, "AT+CIPCCFG=3,100,,1,0,,1000", SIMCOM_DEFAULT_RETRY, 110, "\r\n配置TCP/IP失败!\r\n") == FALSE)
            {
                //return FALSE;
            }
            else
                break;
            pHandle->pDelayMS(1000); //延时1秒
            retry--;
        }
        if (retry == 0)
        {
            uart_printf("\r\n设置TCP重发次数以及发送延时失败!\r\n");
            pHandle->pClearRxData(); //清除接收计数器
            return FALSE;
        }
        //设置不用等到发送响应
        retry = SIMCOM_DEFAULT_RETRY; //重试次数
        while (retry)
        {
            //设置重试次数为3次，并且发送延时为120ms
            if (SIMCOM_SetParametersReturnBool(pHandle, "AT+CIPSENDMODE=0", SIMCOM_DEFAULT_RETRY, 110, "\r\n不用等待发送ACK设置失败!\r\n") == FALSE)
            {
                //return FALSE;
            }
            else
                break;
            pHandle->pDelayMS(1000); //延时1秒
            retry--;
        }
        if (retry == 0)
        {
            uart_printf("\r\n设置不用等待发送ACK失败!\r\n");
            pHandle->pClearRxData(); //清除接收计数器
        }

        //显示接收数据长度
        retry = SIMCOM_DEFAULT_RETRY; //重试次数
        while (retry)
        {
            //设置重试次数为3次，并且发送延时为120ms
            if (SIMCOM_SetParametersReturnBool(pHandle, "AT+CIPHEAD=1", SIMCOM_DEFAULT_RETRY, 110, "\r\n显示接收数据长度设置失败!\r\n") == FALSE)
            {
                //return FALSE;
            }
            else
                break;
            pHandle->pDelayMS(1000); //延时1秒
            retry--;
        }
        if (retry == 0)
        {
            uart_printf("\r\n设置显示接收数据长度失败!\r\n");
            pHandle->pClearRxData(); //清除接收计数器
            return FALSE;
        }

        //不显示接收数据IP头
        retry = SIMCOM_DEFAULT_RETRY; //重试次数
        while (retry)
        {
            //设置重试次数为3次，并且发送延时为120ms
            if (SIMCOM_SetParametersReturnBool(pHandle, "AT+CIPSRIP=0", SIMCOM_DEFAULT_RETRY, 110, "\r\n不显示接收数据IP头设置失败!\r\n") == FALSE)
            {
                //return FALSE;
            }
            else
                break;
            pHandle->pDelayMS(1000); //延时1秒
            retry--;
        }
        if (retry == 0)
        {
            uart_printf("\r\n不显示接收数据IP头失败!\r\n");
            pHandle->pClearRxData(); //清除接收计数器
            return FALSE;
        }
    }
    break;
    default: //2G模块均需要进行设置的
    {
        //设置GPRS发送数据缓冲区大小
        retry = SIMCOM_DEFAULT_RETRY; //重试次数
        do
        {
            if (SIM900_SetGPRS_PackDatatSize(pHandle) == TRUE)
            {
                break;
            }
            retry--;
        } while (retry);
        if (retry == 0)
        {
            uart_printf("\r\n设置GPRS传输大小失败!\r\n");
            pHandle->pClearRxData(); //清除接收计数器
            return FALSE;
        }
    }
    break;
    }

    pHandle->s_isInitStatus = TRUE; //模块成功初始化
    pHandle->pClearRxData();        //清除接收计数器

    return TRUE;
}

/*************************************************************************************************************************
* 函数				:	bool SIMCOM_GetModuleInfo(SIMCOM_HANDLE *pHandle, SIMCOM_INFO *pInfo)
* 功能				:	获取模块的相关信息
* 参数				:	pHandle:句柄；pInfo:信息结构体指针
* 返回				:	FALSE:失败;TRUE:成功
* 依赖				:	底层
* 作者				:	cp1300@139.com
* 时间				:	2014-07-29
* 最后修改时间 		: 	2014-10-08
* 说明				: 	SIMCOM_INFO_SIZE:限制最大长度
						SIMCOM_VER_SIZE:软件版本长度限制
						2014-10-08：在个别模块上面遇到发送AT+GMI后返回了AT+GMI，导致获取失败，如果发现返回了AT+则重新获取，可以避免此问题
						2016-12-07:修改获取模块型号指令为AT+CGMM,用于兼容SIM7600
*************************************************************************************************************************/
bool SIMCOM_GetModuleInfo(SIMCOM_HANDLE *pHandle, SIMCOM_INFO *pInfo)
{
    u32 i, cnt;
    u8 retry = SIMCOM_DEFAULT_RETRY; //重试次数
    char *p;
    u8 *pData;

    //清空缓冲区
    pInfo->Manu[0] = 0;
    pInfo->Model[0] = 0;
    pInfo->Ver[0] = 0;
    pInfo->IMEI[0] = 0;

    retry = SIMCOM_DEFAULT_RETRY; //重试次数
    //获取制造商信息
    do
    {
        SIMCOM_TestAT(pHandle, 10);
        SIMCOM_SendAT(pHandle, "AT+GMI"); //请求制造商身份

        if (AT_RETURN_OK == SIMCOM_GetATResp(pHandle, &pData, &cnt, "OK", 10, 200)) //等待响应,超时200MS
        {
            //uart_printf("%s\r\n",pData);
            if (strstr((const char *)pData, "AT+") == NULL) //搜索关键字
            {
                for (i = 0; i < (SIMCOM_INFO_SIZE - 1); i++)
                {
                    if ((pData[2 + i] == '\r') || (pData[2 + i] == '\n') || (pData[2 + i] == '\0'))
                        break;
                    pInfo->Manu[i] = pData[2 + i];
                }
                pInfo->Manu[i] = 0;
                break;
            }
        }
        SIMCOM_Ready(pHandle);   //等待就绪
        pHandle->pDelayMS(1000); //失败延时1秒后重试
        retry--;
    } while (retry);
    if (retry == 0)
        return FALSE;

    retry = SIMCOM_DEFAULT_RETRY; //重试次数
    //获取型号
    do
    {
        SIMCOM_SendAT(pHandle, "AT+CGMM");
        pHandle->pClearRxData();                                                    //清除接收计数器
        if (AT_RETURN_OK == SIMCOM_GetATResp(pHandle, &pData, &cnt, "OK", 10, 200)) //等待响应,超时200MS
        {
            for (i = 0; i < (SIMCOM_INFO_SIZE - 1); i++)
            {
                if ((pData[2 + i] == '\r') || (pData[2 + i] == '\n') || (pData[2 + i] == '\0'))
                    break;
                pInfo->Model[i] = pData[2 + i];
            }
            pInfo->Model[i] = 0;
            break;
        }
        SIMCOM_Ready(pHandle);   //等待就绪
        pHandle->pDelayMS(1000); //失败延时1秒后重试
        retry--;
    } while (retry);
    if (retry == 0)
        return FALSE;

    retry = SIMCOM_DEFAULT_RETRY; //重试次数
    //获取软件版本
    do
    {
        SIMCOM_SendAT(pHandle, "AT+GMR");
        pHandle->pClearRxData();                                                    //清除接收计数器
        if (AT_RETURN_OK == SIMCOM_GetATResp(pHandle, &pData, &cnt, "OK", 10, 200)) //等待响应,超时200MS
        {
            p = strstr((char *)pData, "+GMR: ");
            if (p != NULL)
            {
                p += strlen("+GMR: "); //SIM7600前面会有 +GMR:  ，跳过即可
                for (i = 0; i < (SIMCOM_VER_SIZE - 1); i++)
                {
                    if ((p[i] == '\r') || (p[i] == '\n') || (p[i] == '\0'))
                        break;
                    pInfo->Ver[i] = p[i];
                }
                pInfo->Ver[i] = 0;
            }
            else
            {
                for (i = 0; i < (SIMCOM_VER_SIZE - 1); i++)
                {
                    if ((pData[2 + i] == '\r') || (pData[2 + i] == '\n') || (pData[2 + i] == '\0'))
                        break;
                    pInfo->Ver[i] = pData[2 + i];
                }
                pInfo->Ver[i] = 0;
            }

            break;
        }
        SIMCOM_Ready(pHandle);   //等待就绪
        pHandle->pDelayMS(1000); //失败延时1秒后重试
        retry--;
    } while (retry);
    if (retry == 0)
        return FALSE;

    retry = SIMCOM_DEFAULT_RETRY; //重试次数
    //获取序列号
    do
    {
        SIMCOM_SendAT(pHandle, "AT+GSN");
        pHandle->pClearRxData();                                                    //清除接收计数器
        if (AT_RETURN_OK == SIMCOM_GetATResp(pHandle, &pData, &cnt, "OK", 10, 200)) //等待响应,超时200MS
        {
            for (i = 0; i < (SIMCOM_INFO_SIZE - 1); i++)
            {
                if ((pData[2 + i] == '\r') || (pData[2 + i] == '\n') || (pData[2 + i] == '\0'))
                    break;
                pInfo->IMEI[i] = pData[2 + i];
            }
            pInfo->IMEI[i] = 0;
            break;
        }
        SIMCOM_Ready(pHandle);   //等待就绪
        pHandle->pDelayMS(1000); //失败延时1秒后重试
        retry--;
    } while (retry);

    return TRUE;
}

/*************************************************************************************************************************
* 函数				:	bool SIMCOM_COPS(SIMCOM_HANDLE *pHandle, char pCOPS_Buff[SIMCOM_INFO_SIZE])
* 功能				:	获取运营商名称
* 参数				:	pHandle:句柄；pCOPS_Buff:运营商名称
* 返回				:	FALSE:失败;TRUE:成功
* 依赖				:	底层
* 作者				:	cp1300@139.com
* 时间				:	2014-07-29
* 最后修改时间 		: 	2018-03-24
* 说明				: 	SIMCOM_INFO_SIZE 限制最大长度
*************************************************************************************************************************/
bool SIMCOM_COPS(SIMCOM_HANDLE *pHandle, char pCOPS_Buff[SIMCOM_INFO_SIZE])
{
    u32 i, cnt;
    u8 retry = 5; //重试次数
    char *p;
    u8 *pData;

    //清空缓冲区
    pCOPS_Buff[0] = 0;

    switch (pHandle->SimcomModeType) //不同的芯片存在不一样的初始化
    {
    case SIMCOM_SIM2000: //SIM2000需要多次读取,等待的时间比较长
    {
        retry = 28;
    }
    break;
    default:
        break;
    }

    //获取运营商
    do
    {
        SIMCOM_SendAT(pHandle, "AT+COPS?");                                         //显示模块当前注册的网络运营商
        pHandle->pClearRxData();                                                    //清除接收计数器
        if (AT_RETURN_OK == SIMCOM_GetATResp(pHandle, &pData, &cnt, "OK", 20, 150)) //等待响应,超时300MS
        {
            p = strstr((const char *)pData, "\"");
            if (p != NULL)
            {
                p++;
                for (i = 0; i < (SIMCOM_INFO_SIZE - 1); i++)
                {
                    if ((p[i] == '\r') || (p[i] == '\n') || (p[i] == '\0') || (p[i] == '\"'))
                        break;
                    pCOPS_Buff[i] = p[i];
                }
                pCOPS_Buff[i] = 0;
                return TRUE;
            }
        }

        SIMCOM_Ready(pHandle);   //等待就绪
        pHandle->pDelayMS(1000); //延时1秒
        retry--;
    } while (retry);

    return FALSE; //超时//错误
}

/*************************************************************************************************************************
* 函数				:	SIMCOM_NETMODE_TYPE SIM7XXX_GetNetworkMode(SIMCOM_HANDLE *pHandle)
* 功能				:	获取SIM7XXX系列模块网络制式
* 参数				:	pHandle:句柄
* 返回				:	SIMCOM_NETMODE_TYPE
* 依赖				:	底层
* 作者				:	cp1300@139.com
* 时间				:	2016-12-29
* 最后修改时间 		: 	2018-03-24
* 说明				: 	用于从SIM7600模块网络制式
						必须在网络注册成功后进行获取，正常返回
						+CNSMOD: 0,15
						用于SIM7000系列获取网制式
*************************************************************************************************************************/
SIMCOM_NETMODE_TYPE SIM7XXX_GetNetworkMode(SIMCOM_HANDLE *pHandle)
{
    u32 cnt;
    u8 retry = 3;
    char *p;
    int temp;
    u8 *pData;

    //获取型号
    do
    {
        SIMCOM_SendAT(pHandle, "AT+CNSMOD?");
        pHandle->pClearRxData();                                                    //清除接收计数器
        if (AT_RETURN_OK == SIMCOM_GetATResp(pHandle, &pData, &cnt, "OK", 10, 200)) //等待响应,超时200MS
        {
            p = strstr((char *)pData, "+CNSMOD: 0,");
            if (p == NULL)
                p = strstr((char *)pData, "+CNSMOD: 1,");
            p += strlen("+CNSMOD: 0,");
            temp = atoi(p);
            if (temp > 16)
                continue;
            else
                return (SIMCOM_NETMODE_TYPE)temp;
        }

        SIMCOM_Ready(pHandle);   //等待就绪
        pHandle->pDelayMS(1000); //失败延时1秒后重试
        retry--;
    } while (retry);

    return SIMCOM_NETMODE_NULL;
}

/*************************************************************************************************************************
* 函数				:	bool SIMCOM_HardwarePowerUP(SIMCOM_HANDLE *pHandle, bool isTest)
* 功能				:	SIMCOM模块硬件开机
* 参数				:	pHandle：句柄；isTest:是否检测开机是否成功,通过STATUS脚变为高电平可以检测是否开机成功
* 返回				:	开机是否成功
* 依赖				:	无
* 作者				:	cp1300@139.com
* 时间				:	2013-10-29
* 最后修改时间 		: 	2018-03-24
* 说明				: 	用于SIM900模块开机,拉低PWR
						2016-12-07:修改动态监测是否上电成功，增加SIM7600CE兼容
*************************************************************************************************************************/
bool SIMCOM_HardwarePowerUP(SIMCOM_HANDLE *pHandle, bool isTest)
{
    u8 i, j;

    pHandle->pSetDTR_Pin(SIMCOM_L_LEVEL); //DTR=0,退出低功耗模式
    pHandle->s_isInitStatus = FALSE;      //模块没有初始化,需要重新初始化
    if (isTest)                           //需要检测是否开机成功
    {
        if (pHandle->pGetSTATUS_Pin() == SIMCOM_H_LEVEL) //开机脚已经是高电平了
        {
            return TRUE;
        }
        for (i = 0; i < 2; i++)
        {
            pHandle->pSetPWRKEY_Pin(SIMCOM_L_LEVEL); //拉低1200ms开机
            pHandle->pDelayMS(1200);
            pHandle->pSetPWRKEY_Pin(SIMCOM_H_LEVEL); //恢复高电平
            for (j = 0; j < 6; j++)
            {
                pHandle->pDelayMS(1000);
                if (pHandle->pGetSTATUS_Pin() == SIMCOM_H_LEVEL) //开机脚已经是高电平了
                {
                    return TRUE;
                }
            }
        }

        return FALSE;
    }
    else //无需检测是否开机成功
    {
        pHandle->pSetPWRKEY_Pin(SIMCOM_L_LEVEL); //拉低1200ms开机
        pHandle->pDelayMS(1200);
        pHandle->pSetPWRKEY_Pin(SIMCOM_H_LEVEL); //恢复高电平
        pHandle->pDelayMS(3000);
        ; //延时3S等待开机完毕

        return TRUE;
    }
}

/*************************************************************************************************************************
* 函数				:	bool SIMCOM_HardwarePowerDOWN(SIMCOM_HANDLE *pHandle, bool isTest)
* 功能				:	SIMCOM模块硬件关机
* 参数				:	pHandle：句柄；isTest:是否检测开机是否成功,通过STATUS脚变为高电平可以检测是否关机成功
* 返回				:	关机是否成功
* 依赖				:	无
* 作者				:	cp1300@139.com
* 时间				:	2013-10-29
* 最后修改时间 		: 	2018-03-24
* 说明				: 	用于SIM900模块关机机,拉低PWR大于1S小于5S
						2016-12-07:优化关机，兼容SIM7600
						一定要先获取模块型号，不同模块关机时间不一样
*************************************************************************************************************************/
bool SIMCOM_HardwarePowerDOWN(SIMCOM_HANDLE *pHandle, bool isTest)
{
    u8 i, j;

    pHandle->s_isInitStatus = FALSE; //模块没有初始化,需要重新初始化
    if (isTest)                      //需要检测是否开机成功
    {
        if (pHandle->pGetSTATUS_Pin() == SIMCOM_L_LEVEL) //开机脚已经是低电平了
        {
            return TRUE;
        }
        for (i = 0; i < 2; i++)
        {
            pHandle->pSetPWRKEY_Pin(SIMCOM_L_LEVEL); //拉低1200ms关机
            switch (pHandle->SimcomModeType)
            {
            case SIMCOM_SIM7600:
            {
                pHandle->pDelayMS(3000); //SIM7600关机至少2.5S
            }
            break;
            default:
            {
                pHandle->pDelayMS(1000);
            }
            break;
            }

            pHandle->pDelayMS(200);
            pHandle->pSetPWRKEY_Pin(SIMCOM_H_LEVEL); //恢复高电平
            for (j = 0; j < 5; j++)
            {
                pHandle->pDelayMS(3000);                         //延时3S等待关机完毕
                if (pHandle->pGetSTATUS_Pin() == SIMCOM_L_LEVEL) //开机脚已经是低电平了
                {
                    return TRUE;
                }
            }
        }

        return FALSE;
    }
    else //无需检测是否开机成功
    {
        pHandle->pSetPWRKEY_Pin(SIMCOM_L_LEVEL); //拉低1200ms关机
        pHandle->pDelayMS(1200);
        pHandle->pSetPWRKEY_Pin(SIMCOM_H_LEVEL); //恢复高电平
        pHandle->pDelayMS(3000);
        ; //延时3S等待关机完毕

        return TRUE;
    }
}

/*************************************************************************************************************************
* 函数				:	SIMCOM_MODE_TYPE SIMCOM_GetMode(SIMCOM_HANDLE *pHandle)
* 功能				:	获取SIMCOM模块的型号
* 参数				:	pHandle:句柄
* 返回				:	型号,见SIMCOM_MODE_TYPE
* 依赖				:	底层
* 作者				:	cp1300@139.com
* 时间				:	2015-09-12
* 最后修改时间 		: 	2015-09-12
* 说明				: 	用于识别型号,对应初始化
						2016-12-07:修改指令为AT+CGMM，兼容SIM7600，(旧指令:AT+GOI)
*************************************************************************************************************************/
SIMCOM_MODE_TYPE SIMCOM_GetMode(SIMCOM_HANDLE *pHandle)
{
    u32 cnt;
    u8 retry = SIMCOM_DEFAULT_RETRY + 1;
    u8 *pData;

    //获取型号
    do
    {
        SIMCOM_SendAT(pHandle, "AT+CGMM");
        pHandle->pClearRxData();                                                    //清除接收计数器
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

        SIMCOM_Ready(pHandle);   //等待就绪
        pHandle->pDelayMS(1000); //失败延时1秒后重试
        retry--;
    } while (retry);

    return SIMCOM_INVALID;
}

/*************************************************************************************************************************
* 函数			:	bool SIMCOM_GetServeNumber(PHONE_NUMBER *pServeNumber)
* 功能			:	获取短信服务中心号码(会去掉前面的86，限制长度15位,不能用于SIM7000，SIM2000以及电信卡)
* 参数			:	pServeNumber:电话号码存储缓冲区指针
* 返回			:	FALSE:通信失败;TRUE:通信成功
* 依赖			:	底层
* 作者			:	cp1300@139.com
* 时间			:	2013-10-20
* 最后修改时间 	: 	2013-10-20
* 说明			: 	获取SIM卡内部的短信服务中心号码,一般在办理SIM卡的时候已经进行了设置.
					如果没有预置短信中心号码需要使用手机进行设置
					2014-07-12:只要返回OK则认为成功,因为有可能没有设置短信中心号码
					2016-01-26:自动选择是否跳过+86
*************************************************************************************************************************/
bool SIMCOM_GetServeNumber(SIMCOM_HANDLE *pHandle, char pPhoneNumber[16])
{
    u8 i, n;
    u32 cnt;
    char *p, *p1;
    u8 retry = SIMCOM_DEFAULT_RETRY; //重试次数
    u8 *pData;

    do
    {
        //+CSCA: "+8613800270500",145
        SIMCOM_SendAT(pHandle, "AT+CSCA?");                                         //发送"AT+CSCA",获取短信服务中心号码
        pHandle->pClearRxData();                                                    //清除接收计数器
        if (AT_RETURN_OK == SIMCOM_GetATResp(pHandle, &pData, &cnt, "OK", 30, 600)) //等待响应,超时600MS
        {
            p = strstr((const char *)pData, "+CSCA:"); //搜索字符"+CSCA:"
            if (p != NULL)                             //搜索成功
            {
                p = strstr(p + 1, "+"); //搜索"+"
                if (p != NULL)
                {
                    p1 = strstr(p + 1, "\""); //搜索"\""
                    if (p1 != NULL)
                    {
                        if (p[0] == '+') //如果是+开头，则跳过加号
                        {
                            p += 3; //跳过+86
                        }
                        else if (p[0] == '8' && p[1] == '6') //跳过86
                        {
                            p += 2;
                        }
                        n = p1 - p; //计算电话号码长度
                        if (n > 15)
                            n = 15; //限制号码长度为15字节
                        for (i = 0; i < n; i++)
                        {
                            pPhoneNumber[i] = p[i]; //复制电话号码
                        }
                        pPhoneNumber[i] = '\0'; //添加结束符

                        SIMCOM_GSM_debug("短信中心号码:%s\r\n", pPhoneNumber);

                        return TRUE;
                    }
                }
            }
            else
            {
                pPhoneNumber[0] = '\0';
                SIMCOM_GSM_debug("短信中心号码:为空,没有设置\r\n");

                return TRUE;
            }
        }
        SIMCOM_Ready(pHandle);   //等待就绪
        pHandle->pDelayMS(1000); //失败延时1秒后重试
        retry--;
    } while (retry);

    return FALSE;
}

/*************************************************************************************************************************
* 函数			:	bool SIMCOM_GetPhoneNumber(SIMCOM_HANDLE *pHandle, char pPhoneNumber[16])
* 功能			:	获取本机号码(会去掉前面的86，限制长度15位,不能用于SIM7000，SIM2000以及电信卡)
* 参数			:	pHandle:句柄；pPhoneNumber：号码缓冲区
* 返回			:	FALSE:通信失败;TRUE:通信成功
* 依赖			:	底层
* 作者			:	cp1300@139.com
* 时间			:	2013-10-20
* 最后修改时间 	: 	2018-03-24
* 说明			: 	通常会预存本机号码到SIM卡,也可能没有（不能用于SIM7000，SIM2000C,不能用于电信卡）
					2014-07-12:只要返回OK则认为成功,因为有可能没有设置电话号码
					2016-01-26:修改字节超时，否则某些卡会出现超时，没有收到OK
								自动选择是否跳过+86
*************************************************************************************************************************/
bool SIMCOM_GetPhoneNumber(SIMCOM_HANDLE *pHandle, char pPhoneNumber[16])
{
    u8 n;
    u8 i;
    u32 cnt;
    char *p, *p1;
    u8 retry = SIMCOM_DEFAULT_RETRY; //重试次数
    u8 *pData;

    do
    {
        //+CNUM: "","15871750634",129,7,4
        SIMCOM_SendAT(pHandle, "AT+CNUM");                                          //发送"AT++CNUM",获取号码
        pHandle->pClearRxData();                                                    //清除接收计数器
        if (AT_RETURN_OK == SIMCOM_GetATResp(pHandle, &pData, &cnt, "OK", 30, 600)) //等待响应,超时600MS
        {
            p = strstr((const char *)pData, "+CNUM:"); //搜索字符"+CNUM:"
            if (p != NULL)                             //搜索成功
            {
                p = strstr(p + 1, "\",\""); //搜索","	//开始
                if (p != NULL)
                {
                    p1 = strstr(p + 3, "\","); //搜索",//结束
                    if (p1 != NULL)
                    {
                        p += 3;          //跳过","
                        if (p[0] == '+') //如果是+开头，则跳过加号
                        {
                            p += 3; //跳过+86
                        }
                        else if (p[0] == '8' && p[1] == '6') //跳过86
                        {
                            p += 2;
                        }

                        n = p1 - p; //计算电话号码长度
                        if (n > 15)
                            n = 15; //限制号码长度为15字节
                        for (i = 0; i < n; i++)
                        {
                            pPhoneNumber[i] = p[i]; //复制电话号码
                        }
                        pPhoneNumber[i] = '\0'; //添加结束符
                        SIMCOM_GSM_debug("本机号码:%s\r\n", pPhoneNumber);

                        return TRUE;
                    }
                }
            }
            else
            {
                pPhoneNumber[0] = '\0';

                SIMCOM_GSM_debug("本机号码:为空,没有设置\r\n");
                return TRUE;
            }
        }

        SIMCOM_Ready(pHandle);   //等待就绪
        pHandle->pDelayMS(1000); //失败延时1秒后重试
        retry--;
    } while (retry);

    return FALSE;
}

/*************************************************************************************************************************
* 函数			:	bool SIMCOM_GetBookNumber(SIMCOM_HANDLE *pHandle, u8 index, char pPhoneNumber[16])
* 功能			:	从电话簿获取一个电话号码(不能用于SIM7000)
* 参数			:	pHandle:句柄；index:电话号码所有,1-255;CenterPhone:电话号码存储缓冲区指针
* 返回			:	FALSE:通信失败;TRUE:通信成功
* 依赖			:	底层
* 作者			:	cp1300@139.com
* 时间			:	2013-10-20
* 最后修改时间 	: 	2016-06-15
* 说明			: 	用于从电话簿读取一个电话号码，常用语电信卡SIM2000C模块存储本机号码到第一个索引
*************************************************************************************************************************/
bool SIMCOM_GetBookNumber(SIMCOM_HANDLE *pHandle, u8 index, char pPhoneNumber[16])
{
    u8 i, n;
    u32 cnt;
    char *p, *p1;
    u8 retry = SIMCOM_DEFAULT_RETRY; //重试次数
    u8 *pData;
    char buff[32];

    if (index < 1)
        return FALSE; //索引从1开始
    do
    {
        //+CPBR: 1,"152778787878",129,"Phone"
        sprintf(buff, "AT+CPBR=%d", index);
        SIMCOM_SendAT(pHandle, buff);                                               //发送"AT+CPBR=1",获取号码
        pHandle->pClearRxData();                                                    //清除接收计数器
        if (AT_RETURN_OK == SIMCOM_GetATResp(pHandle, &pData, &cnt, "OK", 30, 600)) //等待响应,超时600MS
        {
            p = strstr((const char *)pData, "+CPBR:"); //搜索字符"+CPBR:"
            if (p != NULL)                             //搜索成功
            {
                p = strstr(p + 1, ",\""); //搜索,"	//开始
                if (p != NULL)
                {
                    p1 = strstr(p + 2, "\","); //搜索",//结束
                    if (p1 != NULL)
                    {
                        p += 2;          //跳过,"
                        if (p[0] == '+') //如果是+开头，则跳过加号
                        {
                            p += 3; //跳过+86
                        }
                        else if (p[0] == '8' && p[1] == '6') //跳过86
                        {
                            p += 2;
                        }

                        n = p1 - p; //计算电话号码长度
                        if (n > 15)
                            n = 15; //限制号码长度为15字节
                        for (i = 0; i < n; i++)
                        {
                            pPhoneNumber[i] = p[i]; //复制电话号码
                        }
                        pPhoneNumber[i] = '\0'; //添加结束符

                        SIMCOM_GSM_debug("号码:%s\r\n", pPhoneNumber);
                        return TRUE;
                    }
                }
            }
            else
            {
                pPhoneNumber[0] = '\0';
                SIMCOM_GSM_debug("号码:为空\r\n");

                return TRUE;
            }
        }
        SIMCOM_Ready(pHandle);   //等待就绪
        pHandle->pDelayMS(1000); //失败延时1秒后重试
        retry--;
    } while (retry);

    return FALSE;
}

/*************************************************************************************************************************
* 函数				:	int SIMCOM_GetSignal(SIMCOM_HANDLE *pHandle)
* 功能				:	获取信号强度
* 参数				:	pHandle:句柄
* 返回				:	<0:获取失败;0-31:信号强度;
* 依赖				:	底层
* 作者				:	cp1300@139.com
* 时间				:	2013-10-21
* 最后修改时间 		: 	2013-10-21
* 说明				: 	无
*************************************************************************************************************************/
int SIMCOM_GetSignal(SIMCOM_HANDLE *pHandle)
{
    u8 temp;
    u32 cnt;
    char *p;
    u8 retry = SIMCOM_DEFAULT_RETRY; //重试次数
    u8 *pData;

    do
    {
        //+CSQ: 27,0
        //+CSQ: 5,0
        //+CSQ: 16,99
        SIMCOM_SendAT(pHandle, "AT+CSQ");                                           //发送"AT++CSQ",获取号码
        pHandle->pClearRxData();                                                    //清除接收计数器
        if (AT_RETURN_OK == SIMCOM_GetATResp(pHandle, &pData, &cnt, "OK", 20, 400)) //等待响应,超时400MS
        {
            p = strstr((const char *)pData, "+CSQ:"); //搜索字符"+CSQ:"
            if (p != NULL)                            //搜索成功
            {
                if (p[7] != ',' && p[8] != ',')
                    p[8] = '\0';
                temp = atoi(&p[6]);

                return temp;
            }
            break;
        }
        SIMCOM_Ready(pHandle);   //等待就绪
        pHandle->pDelayMS(1000); //失败延时1秒后重试
        retry--;
    } while (retry);

    return -1;
}

/*************************************************************************************************************************
* 函数			:	bool SIMCOM_GetCIMI(SIMCOM_HANDLE *pHandle, char pCIMI[16])
* 功能			:	获取SIM卡CIMI号码(SIM卡唯一id，必须存在)
* 参数			:	pHandle：句柄；pCIMI:CIMI缓冲区，长15字节
* 返回			:	FALSE:通信失败;TRUE:通信成功
* 依赖			:	底层
* 作者			:	cp1300@139.com
* 时间			:	2017-11-16
* 最后修改时间 	: 	2018-03-24
* 说明			: 	用于获取卡唯一CIMI编号，防止某些卡无法读取本机号码，这个与卡号一一对应
*************************************************************************************************************************/
bool SIMCOM_GetCIMI(SIMCOM_HANDLE *pHandle, char pCIMI[16])
{
    u32 cnt;
    u8 retry = SIMCOM_DEFAULT_RETRY; //重试次数
    u8 *pData;

    do
    {
        //460041037206894
        SIMCOM_SendAT(pHandle, "AT+CIMI");                                          //发送"AT+CIMI"
        pHandle->pClearRxData();                                                    //清除接收计数器
        if (AT_RETURN_OK == SIMCOM_GetATResp(pHandle, &pData, &cnt, "OK", 20, 200)) //等待响应,超时200MS
        {
            if (pData[0] != '\r' || pData[1] != '\n')
                continue;
            memcpy(pCIMI, &pData[2], 15); //跳过前面\r\n
            pCIMI[15] = 0;                //添加结束符

            SIMCOM_GSM_debug("获取CIMI成功：%s\r\n", pCIMI);
            return TRUE;
        }

        SIMCOM_Ready(pHandle);   //等待就绪
        pHandle->pDelayMS(1000); //失败延时1秒后重试
        retry--;
    } while (retry);

    return FALSE;
}

/*************************************************************************************************************************
* 函数				:	bool SIM7000C_GetNB_APN(SIMCOM_HANDLE *pHandle, char pAPN[17])
* 功能				:	获取SIM7000C NBIOT 接入点
* 参数				:	pHandle：句柄；pAPN：接入点缓冲区
* 返回				:	TRUE:成功；FALSE:失败
* 依赖				:	底层
* 作者				:	cp1300@139.com
* 时间				:	2018-01-16
* 最后修改时间 		: 	2018-01-16
* 说明				: 	必须是NBIOT模式才能使用
*************************************************************************************************************************/
bool SIM7000C_GetNB_APN(SIMCOM_HANDLE *pHandle, char pAPN[17])
{
    u32 cnt;
    char *p;
    u8 retry = SIMCOM_DEFAULT_RETRY; //重试次数
    u8 *pData;
    u8 i;

    do
    {
        //+CGNAPN: 1,"ctnb"
        SIMCOM_SendAT(pHandle, "AT+CGNAPN");                                         //发送AT指令
        pHandle->pClearRxData();                                                     //清除接收计数器
        if (AT_RETURN_OK == SIMCOM_GetATResp(pHandle, &pData, &cnt, "OK", 10, 2000)) //等待响应,超时2000MS
        {
            p = strstr((const char *)pData, "+CGNAPN: 1,\""); //搜索字符+CGNAPN: 1,"
            if (p != NULL)                                    //搜索成功
            {
                p += strlen("+CGNAPN: 1,\"");
                for (i = 0; i < 17; i++)
                {
                    if (p[i] == '\"') //结束符号位 ”
                    {
                        pAPN[i] = 0;
                        if (i == 0)
                            break; //太短了
                        return TRUE;
                    }
                    pAPN[i] = p[i];
                }
            }
        }

        SIMCOM_Ready(pHandle);   //等待就绪
        pHandle->pDelayMS(1000); //失败延时1秒后重试
        retry--;
    } while (retry);

    return FALSE;
}
