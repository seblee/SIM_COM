SIMCOM_HANDLE g_SIMCOM_Handle;               //SIMCOM通信模块句柄
u8 g_SIMCOM_Buff[SIMCOM_UART_BUFF_SIZE + 1]; //串口接收缓冲区

static bool GPRS_UART_SendData(u8 DataBuff[], u16 DataLen);                                           //发送数据接口
static int GPRS_UART_ReadData(u8 **pDataBuff, u8 ByteTimeOutMs, u16 TimeOutMs, u16 *pReceiveDelayMs); //接收数据接口
static void GPRS_UART_ClearData(void);                                                                //清除接收缓冲区

//GPRS通信任务
void GPRS_Task(void *pdata)
{
    const char *pModeInof;

    UARTx_Init(SIMCOM_UART_CH, SIMCOM_UART_BAUD, ENABLE);                  //初始化串口
    UARTx_SetRxBuff(SIMCOM_UART_CH, g_SIMCOM_Buff, SIMCOM_UART_BUFF_SIZE); //设置串口接收缓冲区
    SIMCOM_IO_Init();                                                      //SIMCOM相关IO初始化
    OSTimeDlyHMSM(0, 0, 0, 20);

    //初始化SIMCOM句柄接口
    SIMCOM_Init(&g_SIMCOM_Handle,
                GPRS_UART_SendData,  //发送数据接口，如果发送失败，返回FALSE,成功返回TRUE;
                GPRS_UART_ReadData,  //接收数据接口，返回数据长度，如果失败返回<=0,成功，返回数据长度
                GPRS_UART_ClearData, //清除接收缓冲区函数，用于清除接收数据缓冲区数据
                SIMCOM_SetDTR,       //DTR引脚电平控制-用于控制sleep模式或者退出透传模式
                SIMCOM_SetPWRKEY,    //PWRKEY开机引脚电平控制-用于开机
                SIMCOM_GetSTATUS,    //获取STATUS引脚电平-用于指示模块上电状态
                SIMCOM_GetDCD,       //DCD-上拉输入，高电平AT指令模式，低电平为透传模式
                SYS_DelayMS,         //系统延时函数
                IWDG_Feed            //清除系统看门狗(可以为空)
    );

    while (1)
    {
        SIMCOM_RegisNetwork(&g_SIMCOM_Handle, 6, 60, &pModeInof); //SIMCOM模块上电初始化并注册网络
        //SIMCOM 初始化获取短信中心号码以及本机号码,信号强度,CIMI（结果存放在句柄pHandle中）
        if (SIMCOM_PhoneMessageNumberInitialize(&g_SIMCOM_Handle, 3) == FALSE)
        {
            uart_printf("\r\n警告：初始化获取相关信息失败！\r\n");
        }

        OSTimeDlyHMSM(0, 5, 0, 20);
    }
}
