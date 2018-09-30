/*************************************************************************************************************
 * 文件名:			SIMCOM_GSM.h
 * 功能:			SIMCOM GSM相关接口
 * 作者:			cp1300@139.com
 * 创建时间:		2015-02-15
 * 最后修改时间:	2018-03-23
 * 详细:			
*************************************************************************************************************/
#ifndef _SIMCOM_GSM_H_
#define _SIMCOM_GSM_H_
#include "system.h"
#include "simcom.h"

bool SIMCOM_NetworkConfig(SIMCOM_HANDLE *pHandle, SIMCOM_MODE_TYPE ModeType, NETWORK_CONFIG_TYPE *pConfig); //SIMCOM网络配置
SIM_CARD_STATUS SIMCOM_GetCPIN(SIMCOM_HANDLE *pHandle);                                                     //获取SIM卡状态
SIMCOM_NETSTATUS SIM900_GetGSMNetworkStatus(SIMCOM_HANDLE *pHandle);                                        //获取GSM网络注册状态
SIMCOM_NETSTATUS SIMCOM_GetDataNetworkStatus(SIMCOM_HANDLE *pHandle);                                       //获取数据网络注册状态
bool SIMCOM_ModuleInit(SIMCOM_HANDLE *pHandle);                                                             //初始化SIMCOM模块基本配置（不允许失败）
bool SIMCOM_GetModuleInfo(SIMCOM_HANDLE *pHandle, SIMCOM_INFO *pInfo);                                      //获取模块的相关信息
bool SIMCOM_COPS(SIMCOM_HANDLE *pHandle, char pCOPS_Buff[SIMCOM_INFO_SIZE]);                                //获取运营商名称
SIMCOM_NETMODE_TYPE SIM7XXX_GetNetworkMode(SIMCOM_HANDLE *pHandle);                                         //获取SIM7XXX系列模块网络制式
bool SIMCOM_HardwarePowerUP(SIMCOM_HANDLE *pHandle, bool isTest);                                           //SIMCOM模块硬件开机
bool SIMCOM_HardwarePowerDOWN(SIMCOM_HANDLE *pHandle, bool isTest);                                         //SIMCOM模块硬件关机
SIMCOM_MODE_TYPE SIMCOM_GetMode(SIMCOM_HANDLE *pHandle);                                                    //获取SIMCOM模块的型号
int SIMCOM_GetSignal(SIMCOM_HANDLE *pHandle);                                                               //获取信号强度
bool SIMCOM_GetBookNumber(SIMCOM_HANDLE *pHandle, u8 index, char pPhoneNumber[16]);                         //从电话簿获取一个电话号码(不能用于SIM7000)
bool SIMCOM_GetPhoneNumber(SIMCOM_HANDLE *pHandle, char pPhoneNumber[16]);                                  //获取本机号码(会去掉前面的86，限制长度15位,不能用于SIM7000，SIM2000以及电信卡)
bool SIMCOM_GetServeNumber(SIMCOM_HANDLE *pHandle, char pPhoneNumber[16]);                                  //获取短信服务中心号码(会去掉前面的86，限制长度15位,不能用于SIM7000，SIM2000以及电信卡)
bool SIMCOM_GetCIMI(SIMCOM_HANDLE *pHandle, char pCIMI[16]);                                                //获取SIM卡CIMI号码(SIM卡唯一id，必须存在)
bool SIM7000C_GetNB_APN(SIMCOM_HANDLE *pHandle, char pAPN[17]);                                             //获取SIM7000C NBIOT 接入点

#endif /*_SIMCOM_GSM_H_*/
