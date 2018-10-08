/*************************************************************************************************************
 * 文件名:			SIMCOM_USER.c
 * 功能:			SIMCOM用户层函数
 * 作者:			cp1300@139.com
 * 创建时间:		2015-02-15
 * 最后修改时间:	2018-03-23
 * 详细:			
*************************************************************************************************************/
//#include "system.h"
//#include "usart.h"
#include "string.h"
//#include "ucos_ii.h"
#include "SIMCOM_USER.h"
#include "SIMCOM_GSM.h"
//#include "SIMCOM_GPRS.h"
#include "SIMCOM_AT.h"
#include "SIMCOM.h"

#ifndef DEBUG
#define DEBUG(N, ...) rt_kprintf("####[DEBUG %s:%4d] " N "\r\n", __FILE__, __LINE__, ##__VA_ARGS__)
#endif /* DEBUG(...) */

bool g_SIMC0M_USER_Debug = TRUE; //应用层指令调试状态

//调试开关
#define SIMCOM_USER_DBUG 1
#if SIMCOM_USER_DBUG
//#include "system.h"
#define SIMCOM_USER_debug DEBUG
#else
#define SIMCOM_USER_debug(format, ...)
#endif //SIMCOM_USER_DBUG

const char *const SIMCOM_NETWORK_NAME[18] = {
    "未注册",
    "GSM",
    "GPRS",
    "EGPRS (EDGE)",
    "WCDMA",
    "HSDPA only(WCDMA)",
    "HSUPA only(WCDMA)",
    "HSPA (HSDPA and HSUPA, WCDMA)",
    "LTE",
    "TDS-CDMA",
    "TDS-HSDPA only",
    "TDS-HSUPA only",
    "TDS- HSPA (HSDPA and HSUPA)",
    "CDMA",
    "EVDO",
    "HYBRID (CDMA and EVDO)",
    "1XLTE(CDMA and LTE)",
    "未知,错误",
};

/*************************************************************************************************************************
* 函数				:	bool SIMCOM_Init(SIMCOM_HANDLE *pHandle,
							bool (* pSendData)(u8 *pDataBuff, u16 DataLen),											
							int (* pReadData)(u8 **pDataBuff, u8 ByteTimeOutMs, u16 TimeOutMs, u16 *pReceiveDelay),	
							void (*pClearRxData)(void),																
							void (*pSetDTR_Pin)(u8 Level),															
							void (*pSetPWRKEY_Pin)(u8 Level),														
							u8 (*pGetSTATUS_Pin)(void),	
							u8 (*pGetDCD_Pin)(void),
							void (*pDelayMS)(u32 ms),																
							void (*pIWDG_Feed)(void)
* 功能				:	初始化SIMCOM句柄接口
* 参数				:	pSl651_Handle:句柄；
						pSendCallBack：发送回调函数（pDataBuff：发送数据缓冲区，DataLen：发送数据长度）
						pReadCallBack：接收数据回调函数，会等待直到数据被写入到接收缓冲区（pDataBuff:接收数据缓冲区，ByteTimeOut：等待的字节超时时间，单位ms,TimeOut:数据包超时时间，单位ms）
						pClearRxData:清除接收缓冲区函数，用于清除接收数据缓冲区数据
						pSetDTR_Pin:DTR引脚电平控制-用于控制sleep模式或者退出透传模式
						pSetPWRKEY_Pin:PWRKEY开机引脚电平控制-用于开机
						pGetSTATUS_Pin:获取STATUS引脚电平-用于指示模块上电状态
						pGetDCD_Pin:获取DCD引脚电平-高电平AT指令模式，低电平为透传模式
						pDelayMS:系统延时函数
						pIWDG_Feed:清除系统看门狗(可以为空)
* 返回				:	无
* 依赖				:	TRUE:成功，FALSE:失败
* 作者				:	cp1300@139.com
* 时间				:	2018-03-24
* 最后修改时间 		: 	2018-03-24
* 说明				: 	除pIWDG_Feed接口可以为空，其余接口均不能为空，否则程序会崩溃
*************************************************************************************************************************/
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
)
{
    if (pHandle == NULL)
    {
        DEBUG("unusable handle!\r\n");
        return FALSE;
    }
    //所需变量
    pHandle->SimcomModeType = SIMCOM_INVALID;                        //模块型号
    pHandle->TelecomCarr[0] = 0;                                     //运营商名称
    memset(&pHandle->SIMCOM_Info, 0, sizeof(SIMCOM_INFO));           //SIMCOM通信模块相关信息结构体
    memset(&pHandle->NetworkConfig, 0, sizeof(NETWORK_CONFIG_TYPE)); //网络模式设置
    pHandle->NetworkMode = SIMCOM_NETMODE_NULL;                      //当前网络制式

    //底层通信接口
    pHandle->pSendData = pSendData;           //发送数据接口，如果发送失败，返回FALSE,成功返回TRUE;
    pHandle->pReadData = pReadData;           //接收数据接口，返回数据长度，如果失败返回<=0,成功，返回数据长度
    pHandle->pClearRxData = pClearRxData;     //清除接收缓冲区函数，用于清除接收数据缓冲区数据
    pHandle->pSetDTR_Pin = pSetDTR_Pin;       //DTR引脚电平控制-用于控制sleep模式或者退出透传模式
    pHandle->pSetPWRKEY_Pin = pSetPWRKEY_Pin; //PWRKEY开机引脚电平控制-用于开机
    pHandle->pGetSTATUS_Pin = pGetSTATUS_Pin; //获取STATUS引脚电平-用于指示模块上电状态
    pHandle->pGetDCD_Pin = pGetDCD_Pin;       //获取DCD引脚电平-高电平AT指令模式，低电平为透传模式
    //系统接口
    pHandle->pDelayMS = pDelayMS;     //系统延时函数
    pHandle->pIWDG_Feed = pIWDG_Feed; //清除系统看门狗(可以为空)
    //内部状态定义
    pHandle->s_isInitStatus = FALSE; //用于记录模块初始化状态，复位或上电后变为无效
    //检查是否有接口为空
    if (pHandle->pSendData == NULL || pHandle->pReadData == NULL || pHandle->pClearRxData == NULL || pHandle->pSetDTR_Pin == NULL || pHandle->pSetPWRKEY_Pin == NULL ||
        pHandle->pGetSTATUS_Pin == NULL || pHandle->pGetDCD_Pin == NULL || pHandle->pDelayMS == NULL)
    {
        DEBUG("有回调接口为空！\r\n");
        if (pHandle->pSendData == NULL || pHandle->pReadData == NULL || pHandle->pClearRxData == NULL ||
            pHandle->pDelayMS == NULL)
            return FALSE;
    }

    return TRUE;
}

/*************************************************************************************************************************
* 函数			:	void SIMCOM_PrintfModel(SIMCOM_HANDLE *pHandle, SIMCOM_MODE_TYPE ModeType, const char **pModeInof)
* 功能			:	显示并打印模块型号
* 参数			:	pHandle：句柄；ModeType：模块型号；pModeInof:返回模块型号信息(不需要可以为空)
* 返回			:	无
* 依赖			:	底层
* 作者			:	cp1300@139.com
* 时间			:	2017-10-17
* 最后修改时间	: 	2018-03-24
* 说明			: 	
*************************************************************************************************************************/
void SIMCOM_PrintfModel(SIMCOM_HANDLE *pHandle, SIMCOM_MODE_TYPE ModeType, const char **pModeInof)
{
    switch (ModeType)
    {
    case SIMCOM_SIM900: //默认为SIM900
    {
        if (pModeInof != NULL)
            *pModeInof = "SIM900系列"; //通信模块型号
        uart_printf("[DTU]通信模块为SIM900\r\n");
    }
    break;
    case SIMCOM_SIM800: //SIM800
    {
        if (pModeInof != NULL)
            *pModeInof = "SIM800系列"; //通信模块型号
        uart_printf("[DTU]通信模块为SIM800\r\n");
    }
    break;
    case SIMCOM_SIM2000: //SIM2000
    {

        if (pModeInof != NULL)
            *pModeInof = "SIM2000系列"; //通信模块型号
        uart_printf("[DTU]通信模块为SIM2000\r\n");
    }
    break;
    case SIMCOM_SIM7600: //SIM7600
    {
        if (pModeInof != NULL)
            *pModeInof = "SIM7600系列"; //通信模块型号
        uart_printf("[DTU]通信模块为SIM7600\r\n");
    }
    break;
    case SIMCOM_SIM868: //SIM868
    {
        if (pModeInof != NULL)
            *pModeInof = "SIM868系列"; //通信模块型号
        uart_printf("[DTU]通信模块为SIM868模块\r\n");
    }
    break;
    case SIMCOM_SIM7000C: //SIM7000C
    {
        if (pModeInof != NULL)
            *pModeInof = "SIM7000C系列"; //通信模块型号
        uart_printf("[DTU]通信模块为SIM7000C\r\n");
    }
    break;
    case LYNQ_L700: //LYNQ_L700
    {
        if (pModeInof != NULL)
            *pModeInof = "L700系列"; //通信模块型号
        uart_printf("[DTU]通信模块为L700模块\r\n");
    }
    break;
    case SIMCOM_INVALID: //无效则默认
    {
        if (pModeInof != NULL)
            *pModeInof = "未知"; //通信模块型号
        uart_printf("[DTU]通信模块未知!\r\n");
    }
    break;
    }
}

/*************************************************************************************************************************
* 函数			:	SIMCOM_USER_ERROR SIMCOM_RegisNetwork(SIMCOM_HANDLE *pHandle, u16 Retry, u16 NetworkDelay,const char **pModeInof)
* 功能			:	SIMCOM模块上电初始化并注册网络
* 参数			:	pHandle：句柄；Retry:初始化重试次数>0;NetworkDelay:注册网络延时时间,单位S;pModeInof:返回模块型号信息(不需要可以为空)
* 返回			:	SIMCOM_USER_ERROR	
* 依赖			:	底层
* 作者			:	cp1300@139.com
* 时间			:	2014-08-18
* 最后修改时间	: 	2018-03-24
* 说明			: 	用于通信模块上电并初始化操作		
*************************************************************************************************************************/
SIMCOM_USER_ERROR SIMCOM_RegisNetwork(SIMCOM_HANDLE *pHandle, u16 Retry, u16 NetworkDelay, const char **pModeInof)
{
    SIMCOM_NETSTATUS NetStatus;
    SIMCOM_MODE_TYPE ModeType = SIMCOM_INVALID;            //初始化模块型号无效
    SIMCOM_NETMODE_TYPE NetworkMode = SIMCOM_NETMODE_NULL; //初始化为未知模式
    SIMCOM_USER_ERROR Error = SIMCOM_NULL_ERROR;           //初始化状态

    u16 pcnt, cnt;
    u8 NotCnt, SeaCnt, TurCnt, UnkCnt, ErrCnt;
    bool isCart = FALSE;
    u8 SIM_NotReadyCnt = 0; //SIM卡未准备就绪计数器

    Retry += 1; //重试次数至少1次
    SIMCOM_USER_debug("[SIMCOM]:start RegisNetwork\r\n");
    //模块上电
    for (pcnt = 0; pcnt < Retry; pcnt++) //上电循环
    {
        SIM_NotReadyCnt = 0; //SIM卡未准备就绪计数器复位
        if (pHandle->pIWDG_Feed != NULL)
            pHandle->pIWDG_Feed(); //喂狗
        if (pHandle->pGetSTATUS_Pin != NULL)
        {
            if (pHandle->pGetSTATUS_Pin() == SIMCOM_L_LEVEL) //模块没有上电
            {
                pHandle->s_isInitStatus = FALSE; //模块没有初始化
                SIMCOM_USER_debug("[SIMCOM]:模块没有上电!\r\n");
                if (SIMCOM_HardwarePowerUP(pHandle, TRUE) == TRUE) //上电
                {
                    SIMCOM_USER_debug("[SIMCOM]:开机成功!\r\n");
                    if (SIMCOM_TestAT(pHandle, 50) != TRUE) //发送AT测试命令
                    {
                        if (pModeInof != NULL)
                            *pModeInof = "模块未知";
                        SIMCOM_USER_debug("[SIMCOM]:通信错误,串口错误!\r\n");
                    }
                }
                else
                {
                    if (pModeInof != NULL)
                        *pModeInof = "模块未知";
                    SIMCOM_USER_debug("[SIMCOM]:开机失败!\r\n");
                    Error = SIMCOM_POWER_UP_ERROR; //开机失败
                }
            }
        }
        SIMCOM_USER_debug("[DTU]SIMCOM_TestAT\r\n");
        do
        {
        } while ((SIMCOM_TestAT(pHandle, 50) != TRUE));

        //上电完毕后初始化模块
        if (pHandle->pGetSTATUS_Pin == NULL)
            goto model_init;
        if (pHandle->pGetSTATUS_Pin() == SIMCOM_H_LEVEL)
        {
        model_init:
            //模块初始化网络
            if (NetworkDelay == 0)
                NetworkDelay = 0xffff;                      //为0,一直等待
            NotCnt = SeaCnt = TurCnt = UnkCnt = ErrCnt = 0; //初始化注册状态计数器为0

            //获取模块型号
            if (ModeType == SIMCOM_INVALID)
            {
                SIMCOM_SetParametersReturnBool(pHandle, "ATE 0", SIMCOM_DEFAULT_RETRY, 110, "\r\n关闭AT回显模式失败!\r\n");
                pHandle->pDelayMS(500);
                ModeType = SIMCOM_GetMode(pHandle);               //获取模块型号
                SIMCOM_PrintfModel(pHandle, ModeType, pModeInof); //打印显示通信模块型号
            }
            //型号获取成功了，更新信息
            if (ModeType != SIMCOM_INVALID)
            {
                pHandle->SimcomModeType = ModeType; //上电初始化设置通信模块型号
            }
            //根据模块类型进行初始化，如果是SIM7000C，需要选择工作在GSM模式还是NBIOT模式
            if (SIMCOM_NetworkConfig(pHandle, ModeType, &pHandle->NetworkConfig) == FALSE)
            {
                uart_printf("[DTU]初始化通信模块网络模式失败了!\r\n");
            }

            //初始化获取网络信息
            for (cnt = 0; cnt < NetworkDelay; cnt++)
            {
                if (pHandle->pGetSTATUS_Pin != NULL)
                {
                    if (pHandle->pGetSTATUS_Pin() == SIMCOM_L_LEVEL)
                    {
                        Error = SIMCOM_POWER_UP_ERROR; //异常断电
                        uart_printf("[DTU]异常断电了,请检查供电是否稳定!\r\n");
                        break; //模块没有上电,初始化
                    }
                }

                if (ModeType == SIMCOM_INVALID)
                {
                    SIMCOM_SetParametersReturnBool(pHandle, "ATE 0", SIMCOM_DEFAULT_RETRY, 110, "\r\n关闭AT回显模式失败!\r\n");
                    pHandle->pDelayMS(500);
                    ModeType = SIMCOM_GetMode(pHandle);               //获取模块型号
                    pHandle->SimcomModeType = ModeType;               //上电初始化设置通信模块型号
                    SIMCOM_PrintfModel(pHandle, ModeType, pModeInof); //打印显示通信模块型号
                }

                if (isCart == FALSE) //卡未检测到，则一直重新检测
                {
                    if (SIMCOM_GetCPIN(pHandle) == SIM_READY)
                    {
                        isCart = TRUE; //卡检测成功
                        uart_printf("\r\nSIM卡准备就绪!\r\n");
                    }
                    else
                    {
                        uart_printf("\r\nSIM卡未准备就绪!\r\n");
                        Error = SIMCOM_SIM_NOT_REALYE; //卡未就绪
                    }
                }

                //2018-01-18 增加SIM7000C NB模式网络注册状态获取支持
                if ((ModeType == SIMCOM_SIM7000C) && (pHandle->NetworkConfig.NB_EnableMode == 1)) //如果是SIM7000C，并且开启了NB模式
                {
                    NetStatus = SIMCOM_GetDataNetworkStatus(pHandle); //获取数据网络网络注册状态
                }
                else
                {
                    NetStatus = SIM900_GetGSMNetworkStatus(pHandle); //获取GSM网络注册状态
                }

                SIMCOM_USER_debug("[DTU]网络注册状态%d!\r\n", NetStatus);
                //一直出现网络未注册以及注册拒绝,重启,有些卡会先出现拒绝,然后出现注册成功
                switch (NetStatus)
                {
                case SIMCOM_NET_NOT:
                {
                    NotCnt++;
                    //2017-09-09 增加如果连续15次未检测到卡，并且未注册，则直接退出，返回未插卡
                    if (SIMCOM_GetCPIN(pHandle) == SIM_NOT_READY)
                    {
                        Error = SIMCOM_SIM_NOT_REALYE; //卡未就绪
                        SIM_NotReadyCnt++;             //SIM卡未准备就绪计数器增加
                    }
                    else
                    {
                        SIM_NotReadyCnt = 0; //SIM卡未就绪计数器复位
                    }
                }
                break;               //未注册
                case SIMCOM_NET_ROA: //已经注册,但是漫游
                case SIMCOM_NET_YES: //已经注册
                {
                    SIM_NotReadyCnt = 0;                  //SIM卡未就绪计数器复位
                    if (pHandle->s_isInitStatus == FALSE) //模块没有初始化
                    {
                        if (SIMCOM_ModuleInit(pHandle) == FALSE) //上电后初始化模块基本配置
                        {
                            SIMCOM_USER_debug("[DTU]初始化失败!\r\n");
                            if (isCart == TRUE)
                                Error = SIMCOM_INIT_ERROR; //卡初始化成功了，才返回初始化错误
                            goto reset;
                        }
                    }

                    //获取模块信息
                    if (SIMCOM_GetModuleInfo(pHandle, &pHandle->SIMCOM_Info) == TRUE) //获取模块的相关信息
                    {
                        SIMCOM_USER_debug("打印模块信息!\r\n");
                        uart_printf("\r\n制造商:%s\r\n", pHandle->SIMCOM_Info.Manu);
                        uart_printf("模块型号:%s\r\n", pHandle->SIMCOM_Info.Model);
                        uart_printf("软件版本:%s\r\n", pHandle->SIMCOM_Info.Ver);
                        uart_printf("模块序列号:%s\r\n", pHandle->SIMCOM_Info.IMEI);
                    }
                    else
                    {
                        SIMCOM_USER_debug("\r\n获取模块信息失败!\r\n");
                    }
                    //获取运营商信息
                    if (SIMCOM_COPS(pHandle, pHandle->TelecomCarr) == TRUE) //获取运营商名称
                    {
                        SIMCOM_USER_debug("运营商信息:%s\r\n", pHandle->TelecomCarr);
                    }
                    else
                    {
                        SIMCOM_USER_debug("获取运营商信息失败!\r\n");
                    }

                    //如果是SIM7600 SIM7000C模块,则需要获取网络制式
                    switch (ModeType)
                    {
                    case SIMCOM_SIM7600:
                    case SIMCOM_SIM7000C:
                    {
                        NetworkMode = SIM7XXX_GetNetworkMode(pHandle); //获取SIM7XXX系列模块网络制式
                        pHandle->NetworkMode = NetworkMode;            //记录全局网络制式，必须在获取短信中心号码之前进行获取，因为电信卡无短信中心号码用于初始化
                        if (NetworkMode > 16)
                        {
                            SIMCOM_USER_debug("网络制式:获取失败\r\n");
                        }
                        else
                        {
                            SIMCOM_USER_debug("网络制式:%s\r\n", SIMCOM_NETWORK_NAME[NetworkMode]);
                        }
                    }
                    break;
                    default: //2G
                    {
                        pHandle->NetworkMode = SIMCOM_NETMODE_GSM; //其它2G模块默认GSM网络
                    }
                    break;
                    }

                    Error = SIMCOM_INIT_OK; //初始化成功
                    return Error;           //已经注册并初始化了
                }
                case SIMCOM_NET_SEA:
                    SeaCnt++;
                    break; //未注册,正在搜索
                case SIMCOM_NET_TUR:
                    TurCnt++;
                    break; //注册被拒绝
                case SIMCOM_NET_UNK:
                    UnkCnt++;
                    break; //未知
                default:
                    ErrCnt++;
                    break; //错误
                }

                if ((TurCnt > 60) || (UnkCnt > 20) || (ErrCnt > 20)) //注册被拒绝,或者错误
                {
                    if (isCart == TRUE)
                        Error = SIMCOM_REG_ERROR; //卡初始化成功了，才返回初注册失败
                    SIMCOM_USER_debug("[DTU]模块重启!\r\n");
                    SIMCPM_RESET_ME(pHandle);
                    // if (SIMCOM_HardwarePowerDOWN(pHandle, TRUE) == FALSE)
                    // {
                    //     SIMCOM_USER_debug("[DTU]:关机失败!\r\n");
                    // }
                    break;
                }

                //SIM卡未就绪次数过多
                if (SIM_NotReadyCnt > 16)
                {
                    uart_printf("[DTU]:多次检测到卡未就绪!模块重启!\r\n");
                    SIMCPM_RESET_ME(pHandle);
                    // if (SIMCOM_HardwarePowerDOWN(pHandle, TRUE) == FALSE)
                    // {
                    //     SIMCOM_USER_debug("[DTU]:关机失败!\r\n");
                    // }
                    break;
                }

                //延时
                pHandle->pDelayMS(800);
                if (pHandle->pIWDG_Feed != NULL)
                    pHandle->pIWDG_Feed(); //喂狗
            }
            //网络注册失败或模块初始化失败

        reset:
            if (SIMCOM_HardwarePowerDOWN(pHandle, TRUE) == TRUE) //关闭电源
            {
                uart_printf("[DTU]:网络初始化失败,模块关机成功!\r\n");
            }
            else
            {
                uart_printf("[DTU]:网络初始化失败,模块关机失败!\r\n");
            }
        }
        else
        {
            Error = SIMCOM_POWER_UP_ERROR; //开机失败
        }
    }

    //显示错误信息
    switch (Error)
    {
    case SIMCOM_INIT_OK: //初始化成功
    {
        uart_printf("[DTU]:模块上电或初始成功!\r\n");
    }
    break;
    case SIMCOM_POWER_UP_ERROR: //上电错误
    {
        uart_printf("[DTU]:模块上电错误!\r\n");
    }
    break;
    case SIMCOM_REG_ERROR: //注册出错（超时）
    {
        uart_printf("[DTU]:模块注册网络出错（超时）!\r\n");
    }
    break;

    case SIMCOM_INIT_ERROR: //初始化配置错误
    {
        uart_printf("[DTU]:模块初始化配置出错!\r\n");
    }
    break;
    case SIMCOM_SIM_NOT_REALYE: //SIM卡未就绪导致上电失败
    {
        uart_printf("[DTU]:模块SIM卡未就绪!\r\n");
    }
    break;
    default:
    {
        uart_printf("[DTU]:未知的错误!\r\n");
    }
    break;
    }

    return Error;
}

/*************************************************************************************************************************
* 函数			:	bool SIMCOM_PhoneMessageNumberInitialize(SIMCOM_HANDLE *pHandle, u8 retry)
* 功能			:	SIMCOM 初始化获取短信中心号码以及本机号码,信号强度,CIMI（结果存放在句柄pHandle中）
* 参数			:	pHandle:句柄；Retry:初始化重试次数>0
* 返回			:	TRUE成功,FALSE:失败	
* 依赖			:	底层
* 作者			:	cp1300@139.com
* 时间			:	2014-08-18
* 最后修改时间	: 	2018-03-24
* 说明			: 	用于网络初始化之后进行初始化获取相关信息,需要提前初始化网络模块型号以及网络制式
					短信中心号码存放在:pHandle->ServiceCenterPhoneNumber;	
					电话号码存放在:pHandle->LocalPhoneNumber;	
					信号强度存放在:pHandle->Singal；
					SIM卡CIMI号码存放在pHandle->SIM_CIMI；
					2016-06-15:SIM2000C使用电信SIM卡时,过快读取本机号码会失败,增加延时,确保读取成功
					2016-11-16:增加获取短信中心号码延时
					2017-12-05:NBIO 模块不读取电话号码
*************************************************************************************************************************/
bool SIMCOM_PhoneMessageNumberInitialize(SIMCOM_HANDLE *pHandle, u8 retry)
{
    u8 i; //重试计数器
    int Singal;
    bool status;
    bool isReadServerNumber = TRUE; //是否需要获取短信中心号码，电信卡无需获取

    //测试AT指令
    SIMCOM_Ready(pHandle);
    if (SIMCOM_TestAT(pHandle, 20) != TRUE)
    {

        return FALSE;
    }

    retry += 1;
    //获取短信中心号码
    if (pHandle->SimcomModeType == SIMCOM_SIM2000) //电信模块无需获取短信中心号码
    {
        isReadServerNumber = FALSE;
    }
    else if (pHandle->SimcomModeType == LYNQ_L700) //L700 NBIOT 无需获取短信中心号码
    {
        isReadServerNumber = FALSE;
    }
    else if (pHandle->SimcomModeType == SIMCOM_SIM7000C) //SIM7000C
    {
        isReadServerNumber = FALSE; //sim7000不支持
    }
    else if (pHandle->SimcomModeType == SIMCOM_SIM7600) //SIM7600
    {
        switch (pHandle->NetworkMode) //查看网络制式，如果是CDMA(电信卡)则不需要获取短信中心号码
        {
        case SIMCOM_NETMODE_CDMA:   //CDMA
        case SIMCOM_NETMODE_EVDO:   //EVDO
        case SIMCOM_NETMODE_HYBRID: //HYBRID (CDMA and EVDO)
        case SIMCOM_NETMODE_1XLTE:  //1XLTE(CDMA and LTE)
        {
            isReadServerNumber = FALSE;
        }
        break;
        }
    }

    if (isReadServerNumber) //需要获取短信中心号码
    {
        for (i = 0; i < retry; i++)
        {
            if (SIMCOM_GetServeNumber(pHandle, pHandle->ServiceCenterPhoneNumber) == TRUE) //获取短信服务中心号码成功
            {
                if (pHandle->ServiceCenterPhoneNumber[0] == 0)
                {
                    SIMCOM_USER_debug("短信中心号码为空\r\n");
                }
                else
                {
                    SIMCOM_USER_debug("短信中心号码:%s\r\n", pHandle->ServiceCenterPhoneNumber);
                }
                break;
            }
            else
            {
                SIMCOM_WaitSleep(pHandle, 100);
                SIMCOM_Ready(pHandle);
                pHandle->pDelayMS(1000);
            }
        }
        if (i == retry)
            return FALSE;
    }
    else
    {
        pHandle->pDelayMS(2000);                                  //等待2s钟
        strcpy(pHandle->ServiceCenterPhoneNumber, "13800818500"); //短信中心号码-固定值，防止为空
        SIMCOM_USER_debug("[缺省]短信中心号码:%s\r\n", pHandle->ServiceCenterPhoneNumber);
    }

    //获取信号强度
    for (i = 0; i < retry; i++)
    {
        Singal = SIMCOM_GetSignal(pHandle); //获取信号强度
        if ((Singal > 0) && (Singal != 99))
        {
            pHandle->Singal = (u8)Singal;
            SIMCOM_USER_debug("信号强度 <%02d>!\r\n", Singal);
            break;
        }
        else
        {
            pHandle->Singal = 0;     //没有读取到，延时2秒
            pHandle->pDelayMS(2000); //等待2s钟
        }
        SIMCOM_WaitSleep(pHandle, 100);
        SIMCOM_Ready(pHandle);
        pHandle->pDelayMS(1000); //等待1s钟
    }

    //获取本机号码
    for (i = 0; i < retry; i++)
    {
        if (pHandle->NetworkMode == LYNQ_L700) //L700 NBIOT 无法获取
        {
            pHandle->pDelayMS(1000); //等待1s钟
            pHandle->LocalPhoneNumber[0] = 0;

            return TRUE;
        }
        else if (pHandle->NetworkMode == SIMCOM_SIM7000C) //SIM7000C 无法获取
        {
            pHandle->pDelayMS(1000); //等待1s钟
            pHandle->LocalPhoneNumber[0] = 0;

            return TRUE;
        }
        else if (pHandle->SimcomModeType == SIMCOM_SIM7600) //SIM7600，SIM7000C
        {
            switch (pHandle->NetworkMode) //查看网络制式，如果是CDMA(电信卡)则不需要获取短信中心号码
            {
            case SIMCOM_NETMODE_CDMA:   //CDMA
            case SIMCOM_NETMODE_EVDO:   //EVDO
            case SIMCOM_NETMODE_HYBRID: //HYBRID (CDMA and EVDO)
            case SIMCOM_NETMODE_1XLTE:  //1XLTE(CDMA and LTE)
            {
                status = SIMCOM_GetBookNumber(pHandle, 1, pHandle->LocalPhoneNumber); //从电话簿中读取一个电话号码
            }
            break;
            default: //其他制式可以读取
            {
                status = SIMCOM_GetPhoneNumber(pHandle, pHandle->LocalPhoneNumber); //读取本机号码
            }
            break;
            }
        }
        else if (pHandle->SimcomModeType == SIMCOM_SIM2000) //SIM2000
        {
            status = SIMCOM_GetBookNumber(pHandle, 1, pHandle->LocalPhoneNumber); //从电话簿中读取一个电话号码
        }

        else //其它模块
        {
            status = SIMCOM_GetPhoneNumber(pHandle, pHandle->LocalPhoneNumber); //读取本机号码
        }

        if (status == TRUE) //获取本机号码成功
        {
            pHandle->LocalPhoneNumber[15] = 0;
            if (pHandle->LocalPhoneNumber[0] == 0)
            {
                SIMCOM_USER_debug("本机号码为空\r\n");
            }
            else
            {
                SIMCOM_USER_debug("本机号码:%s\r\n", pHandle->LocalPhoneNumber);
            }
            break;
        }
        else
        {
            SIMCOM_WaitSleep(pHandle, 100);
            SIMCOM_Ready(pHandle);
            pHandle->pDelayMS(2000); //等待2s钟
        }
    }
    if (i == retry)
        return FALSE;

    //获取SIM卡CIMI号码
    for (i = 0; i < retry; i++)
    {
        //获取SIM卡CIMI号码(SIM卡唯一id，必须存在)
        if (SIMCOM_GetCIMI(pHandle, pHandle->SIM_CIMI) == TRUE)
            break;

        SIMCOM_WaitSleep(pHandle, 100);
        SIMCOM_Ready(pHandle);
        pHandle->pDelayMS(1000); //等待1s钟
    }
    if (i == retry)
        return FALSE;

    return TRUE;
}
