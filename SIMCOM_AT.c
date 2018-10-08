/*************************************************************************************************************
 * 文件名:			SIMCOM_AT.c
 * 功能:			SIMCOM底层AT指令接口
 * 作者:			cp1300@139.com
 * 创建时间:		2015-02-15
 * 最后修改时间:	2018-03-23
 * 详细:			
*************************************************************************************************************/
//#include "system.h"
//#include "usart.h"
#include "SIMCOM_AT.h"
#include "SIMCOM.h"
#include "string.h"
//#include "ucos_ii.h"

//调试开关
#define SIMCOM_DBUG 1
#if SIMCOM_DBUG

#ifndef SIMCOM_debug
#define SIMCOM_debug(format, ...) rt_kprintf("####[SIMCOM_AT %s:%4d] " format "\r\n", __FILE__, __LINE__, ##__VA_ARGS__)
#endif /* SIMCOM_debug(...) */

#else

#define SIMCOM_debug(N, ...)
#endif

/*************************************************************************************************************************
* 函数				:	bool SIMCOM_SendAT(SIMCOM_HANDLE *pHandle, char *pStr)
* 功能				:	发送一个AT指令（会添加结束符\r\n）,不会等待响应
* 参数				:	pHandle:SIMCOM句柄；pStr：指令字符串
* 返回				:	接口发送状态
* 依赖				:	无
* 作者				:	cp1300@139.com
* 时间				:	2018-03-23
* 最后修改时间 		: 	2018-03-23
* 说明				: 	用于底层AT指令发送
*************************************************************************************************************************/
bool SIMCOM_SendAT(SIMCOM_HANDLE *pHandle, char *pStr)
{
    pHandle->pSendData((u8 *)pStr, strlen(pStr)); //发送指令
    return pHandle->pSendData((u8 *)"\r\n", 2);   //发送结束符
}

/*************************************************************************************************************************
* 函数			:	bool SIMCOM_TestAT(SIMCOM_HANDLE *pHandle, u32 retry)
* 功能			:	SIMCOM AT 命令通信测试
* 参数			:	pHandle:SIMCOM句柄；retry:重试次数
* 返回			:	FALSE:通信失败;TRUE:通信成功
* 依赖			:	底层
* 作者			:	cp1300@139.com
* 时间			:	2013-10-20
* 最后修改时间 : 	2018-03-23
* 说明			: 	每隔100ms向SIMCOM通信模块发送一个"AT",等待响应返回
*************************************************************************************************************************/
bool SIMCOM_TestAT(SIMCOM_HANDLE *pHandle, u32 retry)
{
    u32 cnt;
    u8 *pRxBuff;

    //检测模块存在
    do
    {
        SIMCOM_SendAT(pHandle, "AT");                                                 //发送"AT",同步波特率,并且等待应答
        pHandle->pClearRxData();                                                      //清除计数器
        if (AT_RETURN_OK == SIMCOM_GetATResp(pHandle, &pRxBuff, &cnt, "OK", 10, 150)) //等待响应,超时150ms
        {
            pHandle->pDelayMS(100);
            return TRUE;
        }
        retry--;
    } while (retry);

    pHandle->pDelayMS(100);
    return FALSE;
}

/*************************************************************************************************************************
* 函数				:	bool SIMCOM_WaitSleep(SIMCOM_HANDLE *pHandle, u32 TimeOutMs)
* 功能				:	等待模块空闲,并重新唤醒
* 参数				:	pHandle:句柄；TimeOut:等待超时,时间单位ms
* 返回				:	TRUE:成功;FALSE:超时
* 依赖				:	无
* 作者				:	cp1300@139.com
* 时间				:	2013-10-25
* 最后修改时间 		: 	2018-03-24
* 说明				: 	用于等待操作完成,防止快速操作造成模块不响应
*************************************************************************************************************************/
bool SIMCOM_WaitSleep(SIMCOM_HANDLE *pHandle, u32 TimeOutMs)
{
    u32 i;
    u32 cnt;
    u8 *pData;

    if (TimeOutMs < 100)
        TimeOutMs = 100; //最少100ms
    if (pHandle->pSetDTR_Pin != NULL)
        pHandle->pSetDTR_Pin(SIMCOM_H_LEVEL); //等待模块空闲后进入SLEEP模式

    //循环发送命令，直到命令超时了则认为进入了sleep模式
    for (i = 0; i < (TimeOutMs / 100); i++)
    {
        pHandle->pDelayMS(100);                                                           //延时100ms
        SIMCOM_SendAT(pHandle, "AT");                                                     //发送"AT",同步波特率,并且等待应答
        pHandle->pClearRxData();                                                          //清除接收计数器
        if (AT_RETURN_TIME_OUT == SIMCOM_GetATResp(pHandle, &pData, &cnt, "OK", 10, 100)) //等待响应,超时100ms
        {
            break;
        }
    }
    if (pHandle->pSetDTR_Pin != NULL)
        pHandle->pSetDTR_Pin(SIMCOM_L_LEVEL); //唤醒

    if (i == (TimeOutMs / 100))
    {
        SIMCOM_debug("模块进入空闲模式失败!\r\n");
        pHandle->pClearRxData(); //清除接收计数器

        return FALSE;
    }
    pHandle->pDelayMS(100); //延时100ms

    SIMCOM_debug("模块进入空闲模式成功!\r\n");
    SIMCOM_TestAT(pHandle, 10);
    pHandle->pClearRxData(); //清除接收计数器

    return TRUE;
}

/*************************************************************************************************************************
* 函数				:	SIMCOM_AT_ERROR SIMCOM_GetATResp(SIMCOM_HANDLE *pHandle, u8 **pRxBuff, u32 *pLen, const char *pKeyword, u8 ByteTimeOutMs, u16 TimeOutMs)
* 功能				:	获取SIMCOM的AT指令响应
* 参数				:	pHandle:句柄
						pRxBuff:接收缓冲区指针(输出);pLen:接收到的数据大小(输出),
						pKeyword:关键字,为字符串,比如"OK",如果在接收到的字符串中有OK字符,就返回成功,否则失败(输入)
						ByteTimeOutMs:字节超时时间,单位ms最大255ms
						TimeOutMs:等待超时时间,单位毫秒
* 返回				:	SIM900_ERROR
* 依赖				:	无
* 作者				:	cp1300@139.com
* 时间				:	2018-03-24
* 最后修改时间 		: 	2018-03-24
* 说明				: 	本函数会在接收缓冲区字符串结束添加'\0'
						本函数不能清除缓冲区
*************************************************************************************************************************/
SIMCOM_AT_ERROR SIMCOM_GetATResp(SIMCOM_HANDLE *pHandle, u8 **pRxBuff, u32 *pLen, const char *pKeyword, u8 ByteTimeOutMs, u16 TimeOutMs)
{
    int len;
    u16 ReceiveDelay;

    if (ByteTimeOutMs < 1)
        ByteTimeOutMs = 1;
    len = pHandle->pReadData(pRxBuff, ByteTimeOutMs, TimeOutMs, &ReceiveDelay); //调用回调接口，读取数据
    //等待超时
    if (len == 0)
    {
        return AT_RETURN_TIME_OUT; //返回超时错误
    }
    //数据接收完毕
    *pLen = len; //返回接收数据长度
    if ((*pRxBuff)[len - 1] != 0)
    {
        (*pRxBuff)[len] = '\0'; //将数据结尾添加结束字符串
    }

    SIMCOM_debug("\r\nSIMCOM(%dB)->%s\r\n", len, *pRxBuff); //打印返回信息
    if (strstr((const char *)(*pRxBuff), pKeyword) != NULL) //搜索关键字
    {
        SIMCOM_debug("%s 返回成功!\r\n", pKeyword);
        return AT_RETURN_OK;
    }
    else if (strstr((const char *)(*pRxBuff), "ERROR") != NULL)
    {
        SIMCOM_debug("%s 返回错误!\r\n", pKeyword);
        return AT_RETURN_ERROR;
    }
    else
    {
        SIMCOM_debug("%s 返回未知!\r\n", pKeyword);
        return AT_RETURN_UNKNOWN;
    }
}

/*************************************************************************************************************************
* 函数				:	bool SIM900_SetParametersReturnBool(char *pATCom, u8 retry, u16 TimeOutx10MS, const char *pErrorDebug)
* 功能				:	设置SIM900一个参数,返回一个bool状态
* 参数				:	pATCom:AT命令;retry:重试次数;TimeOut:命令超时时间,单位10ms;pErrorDebug:失败后提示的调试信息
* 返回				:	TRUE:执行成功了,返回了OK,FALSE:执行失败了,返回了ERROR或其它
* 依赖				:	SIM900
* 作者				:	cp1300@139.com
* 时间				:	2014-12-19
* 最后修改时间 	: 	2014-12-19
* 说明				: 	用于简化命名发送,防止代码重复
*************************************************************************************************************************/
bool SIMCOM_SetParametersReturnBool(SIMCOM_HANDLE *pHandle, char *pATCom, u8 retry, u16 TimeOutMs, const char *pErrorDebug)
{
    u32 cnt;
    u8 *pData;

    retry += 1; //重试次数
    do
    {
        SIMCOM_SendAT(pHandle, pATCom);                                                   //发送AT命令
        pHandle->pClearRxData();                                                          //清除接收计数器
        if (AT_RETURN_OK == SIMCOM_GetATResp(pHandle, &pData, &cnt, "OK", 10, TimeOutMs)) //等待响应,超时10s
        {
            pHandle->pClearRxData(); //清除接收计数器
            return TRUE;
        }

        SIMCOM_Ready(pHandle); //等待就绪
        retry--;
    } while (retry);

    if (pErrorDebug != NULL)
    {
        uart_printf("%s", pErrorDebug); //输出调试信息
    }
    pHandle->pClearRxData(); //清除接收计数器

    return FALSE;
}

/*************************************************************************************************************************
*函数        	:	u32 GSM_StringToHex(char *pStr, u8 NumDigits)
*功能        	:	将16进制样式字符串转换为16进制整型数(必须保证字符串字母都是大写)
*参数        	:	pStr:字符串起始指针
* 					NumDigits:数字位数,16进制数字位数
*返回        	:	转换后的数字
*依赖        	:	无
*作者        	:	cp1300@139.com
*时间        	:	2013-04-30
*最后修改时间	:	2013-10-17
*说明        	:	比如字符串"A865"转换后为0xA865,位数为4位
					必须保证字符串字母都是大写
*************************************************************************************************************************/
u32 GSM_StringToHex(char *pStr, u8 NumDigits)
{
    u8 temp;
    u32 HEX = 0;
    u8 i;

    NumDigits = (NumDigits > 8) ? 8 : NumDigits; //最大支持8位16进制数

    for (i = 0; i < NumDigits; i++)
    {
        HEX <<= 4;
        temp = pStr[i];
        temp = (temp > '9') ? temp - 'A' + 10 : temp - '0';
        HEX |= temp;
    }
    return HEX;
}

/*************************************************************************************************************************
*函数        	:	void GSM_HexToString(u32 HexNum,c har *pStr, u8 NumDigits)
*功能        	:	将整型数字转换为16进制样式字符串(字母为大写,不带结束符)
*参数        	:	HexNum:16进制数字
					pStr:字符缓冲区指针
* 					NumDigits:数字位数,16进制数字位数
*返回        	:	无
*依赖        	:	无
*作者        	:	cp1300@139.com
*时间        	:	2013-04-30
*最后修改时间	:	2013-04-30
*说明        	:	比如字符串0xA865转换后为"A865",位数为4位
*************************************************************************************************************************/
void GSM_HexToString(u32 HexNum, char *pStr, u8 NumDigits)
{
    u8 temp;
    u8 i;

    NumDigits = (NumDigits > 8) ? 8 : NumDigits; //最大支持8位16进制数

    for (i = 0; i < NumDigits; i++)
    {
        temp = 0x0f & (HexNum >> (4 * (NumDigits - 1 - i)));
        temp = (temp > 0x09) ? (temp - 0x0A + 'A') : (temp + '0');
        pStr[i] = temp;
    }
}

/*************************************************************************************************************************
*函数        	:	u32 GSM_StringToDec(char *pStr, u8 NumDigits)
*功能        	:	将10进制样式字符串转换为整型数(必须保证完全为数字字符)
*参数        	:	pStr:字符串起始指针
* 					NumDigits:数字位数,10进制数字位数
*返回        	:	转换后的数字
*依赖        	:	无
*作者        	:	cp1300@139.com
*时间        	:	2013-04-30
*最后修改时间	:	2013-04-30
*说明        	:	比如字符串"1865"转换后为1865,位数为4位
					必须保证完全为数字字符
*************************************************************************************************************************/
u32 GSM_StringToDec(char *pStr, u8 NumDigits)
{
    u32 temp;
    u32 DEC = 0;
    u8 i;
    u8 j;

    NumDigits = (NumDigits > 10) ? 10 : NumDigits; //最大支持10位10进制数

    for (i = 0; i < NumDigits; i++)
    {
        temp = pStr[i] - '0';
        if (temp > 9) //只能是数字范围
            return 0;
        for (j = 1; j < (NumDigits - i); j++)
        {
            temp *= 10;
        }
        DEC += temp;
    }
    return DEC;
}
