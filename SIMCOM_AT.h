/*************************************************************************************************************
 * 文件名:			SIMCOM_AT.h
 * 功能:			SIMCOM底层AT指令接口
 * 作者:			cp1300@139.com
 * 创建时间:		2015-02-15
 * 最后修改时间:	2018-03-23
 * 详细:			
*************************************************************************************************************/
#ifndef _SIMCOM_AT_H_
#define _SIMCOM_AT_H_ #include <rtthread.h>
//#include "system.h"
#include "SIMCOM.h"

extern bool g_SIMC0M_AT_Debug; //底层AT指令调试状态

//SIM900返回错误
typedef enum
{
    AT_RETURN_OK = 0,         //返回成功
    AT_RETURN_ERROR = 1,      //返回错误
    AT_RETURN_UNKNOWN = 2,    //返回结果未知
    AT_RETURN_TIME_OUT = 0xf, //等待返回超时
} SIMCOM_AT_ERROR;

//相关接口
bool SIMCOM_SendAT(SIMCOM_HANDLE *pHandle, char *pStr);                                                                                   //发送一个AT指令（会添加结束符\r\n）,不会等待响应
SIMCOM_AT_ERROR SIMCOM_GetATResp(SIMCOM_HANDLE *pHandle, u8 **pRxBuff, u32 *pLen, const char *pKeyword, u8 ByteTimeOutMs, u16 TimeOutMs); //获取SIMCOM的AT指令响应
bool SIMCOM_SendAT(SIMCOM_HANDLE *pHandle, char *pStr);                                                                                   //发送一个AT指令（会添加结束符\r\n）,不会等待响应
bool SIMCOM_WaitSleep(SIMCOM_HANDLE *pHandle, u32 TimeOutMs);                                                                             //等待模块空闲,并重新唤醒
bool SIMCOM_TestAT(SIMCOM_HANDLE *pHandle, u32 retry);                                                                                    //SIMCOM AT 命令通信测试
#define SIMCOM_Ready(pHandle)               \
    if (SIMCOM_TestAT(pHandle, 5) == FALSE) \
    {                                       \
        SIMCOM_WaitSleep(pHandle, 1000);    \
    }                                                                                                                        //让SIMCOM就绪,防止卡住//串口同步失败,等待上一个操作完成
bool SIMCOM_SetParametersReturnBool(SIMCOM_HANDLE *pHandle, char *pATCom, u8 retry, u16 TimeOutMs, const char *pErrorDebug); //设置SIM900一个参数,返回一个bool状态

//通用工具
u32 GSM_StringToHex(char *pStr, u8 NumDigits);              //将16进制样式字符串转换为16进制整型数(必须保证字符串字母都是大写)
u32 GSM_StringToDec(char *pStr, u8 NumDigits);              //将10进制样式字符串转换为整型数(必须保证完全为数字字符)
void GSM_HexToString(u32 HexNum, char *pStr, u8 NumDigits); //将整型数字转换为16进制样式字符串(字母为大写,不带结束符)

#endif /*SIMCOM_AT*/
