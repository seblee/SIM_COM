

//发送数据接口
static bool GPRS_UART_SendData(u8 DataBuff[], u16 DataLen)
{
    UARTx_SendData(SIMCOM_UART_CH, DataBuff, DataLen);

    return TRUE;
}

//接收数据接口
static int GPRS_UART_ReadData(u8 **pDataBuff, u8 ByteTimeOutMs, u16 TimeOutMs, u16 *pReceiveDelayMs)
{
    u32 cnt = 0;
    u16 TempTime;

    if (ByteTimeOutMs < 1)
        ByteTimeOutMs = 1; //字节超时时间，2个帧之间的间隔最小时间
    TimeOutMs /= ByteTimeOutMs;
    TimeOutMs += 1;
    TempTime = TimeOutMs;
    while (TimeOutMs--)
    {
        cnt = UARTx_GetRxCnt(SIMCOM_UART_CH);
        OSTimeDlyHMSM(0, 0, 0, ByteTimeOutMs);
        if ((cnt > 0) && (cnt == UARTx_GetRxCnt(SIMCOM_UART_CH)))
        {
            if (pReceiveDelayMs != NULL) //需要返回延时
            {
                *pReceiveDelayMs = (TempTime - TimeOutMs) * ByteTimeOutMs;
            }
            *pDataBuff = g_SIMCOM_Buff; //接收缓冲区

            return cnt;
        }
#if SYS_WDG_EN_
        IWDG_Feed(); //喂狗
#endif
    }

    return 0;
}

//清除接收缓冲区
static void GPRS_UART_ClearData(void)
{
    UARTx_ClearRxCnt(SIMCOM_UART_CH); //清除串口缓冲区
}
