/*************************************************************************************************************
 * 文件名:			SIMCOM_USER.h
 * 功能:			SIMCOM用户层函数
 * 作者:			cp1300@139.com
 * 创建时间:		2015-02-15
 * 最后修改时间:	2018-03-23
 * 详细:			
*************************************************************************************************************/
#ifndef _SIMCOM_SUER_H_
#define _SIMCOM_SUER_H_
#include "system.h"
#include "simcom.h"

//SIMCOM 初始化错误
typedef enum
{
    SIMCOM_INIT_OK = 0, //初始化成功

    SIMCOM_POWER_UP_ERROR = 1, //上电错误
    SIMCOM_REG_ERROR = 2,      //注册出错（超时）
    SIMCOM_INIT_ERROR = 3,     //初始化配置错误
    SIMCOM_SIM_NOT_REALYE = 4, //SIM卡未就绪导致上电失败
    SIMCOM_NULL_ERROR = 255    //状态无效
} SIMCOM_USER_ERROR;

//API
//初始化SIMCOM句柄接口
bool SIMCOM_Init(SIMCOM_HANDLE *pHandle,
                 bool (*pSendData)(u8 *pDataBuff, u16 DataLen),                                         //发送数据接口，如果发送失败，返回FALSE,成功返回TRUE;
                 int (*pReadData)(u8 **pDataBuff, u8 ByteTimeOutMs, u16 TimeOutMs, u16 *pReceiveDelay), //接收数据接口，返回数据长度，如果失败返回<=0,成功，返回数据长度
                 void (*pClearRxData)(void),                                                            //清除接收缓冲区函数，用于清除接收数据缓冲区数据
                 void (*pSetDTR_Pin)(u8 Level),                                                         //DTR引脚电平控制-用于控制sleep模式或者退出透传模式
                 void (*pSetPWRKEY_Pin)(u8 Level),                                                      //PWRKEY开机引脚电平控制-用于开机
                 u8 (*pGetSTATUS_Pin)(void),                                                            //获取STATUS引脚电平-用于指示模块上电状态
                 u8 (*pGetDCD_Pin)(void),                                                               //获取DCD引脚电平-高电平AT指令模式，低电平为透传模式
                 void (*pDelayMS)(u32 ms),                                                              //系统延时函数
                 void (*pIWDG_Feed)(void)                                                               //清除系统看门狗(可以为空)
);

SIMCOM_USER_ERROR SIMCOM_RegisNetwork(SIMCOM_HANDLE *pHandle, u16 Retry, u16 NetworkDelay, const char **pModeInof); //SIMCOM模块上电初始化并注册网络
bool SIMCOM_PhoneMessageNumberInitialize(SIMCOM_HANDLE *pHandle, u8 retry);                                         //SIMCOM 初始化获取短信中心号码以及本机号码,信号强度,CIMI（结果存放在句柄pHandle中）

#endif /*_SIMCOM_SUER_H_*/
