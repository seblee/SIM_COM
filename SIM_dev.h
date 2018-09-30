/////////////////////////////////////////////////////////////////////////////////////////////
//SIM900/SIM800通信支持
//GSM模块相关定义
#define SIMCOM_UART_CH UART_CH3          //串口3
#define SIMCOM_UART_BAUD 115200          //波特率
#define SIMCOM_UART_BUFF_SIZE (1024 * 4) //接收缓冲区大小

//相关控制引脚
__inline void SIMCOM_SetDTR(u8 Level) { (PGout(4) = Level); }    //DTR
__inline void SIMCOM_SetPWRKEY(u8 Level) { (PGout(3) = Level); } //PWRKEY
__inline u8 SIMCOM_GetSTATUS(void) { return PGin(5) ? 1 : 0; }   //STATUS
__inline u8 SIMCOM_GetDCD(void) { return PDin(11) ? 1 : 0; }     //DCD-上拉输入，高电平AT指令模式，低电平为透传模式

//引脚初始化
__inline void SIMCOM_IO_Init(void)
{
    SYS_DeviceClockEnable(DEV_GPIOD, TRUE);               //使能GPIOD时钟
    SYS_DeviceClockEnable(DEV_GPIOG, TRUE);               //使能GPIOG时钟
    SYS_GPIOx_Init(GPIOG, BIT3 | BIT4, OUT_PP, SPEED_2M); //推挽输出
    SYS_GPIOx_OneInit(GPIOD, 11, IN_IPU, IN_NONE);        //DCD 上拉输入
    SYS_GPIOx_OneInit(GPIOG, 5, IN_IPD, IN_NONE);         //STATUS 下拉输入
}
//喂独立看门狗
__inline void IWDG_Feed(void) { IWDG->KR = 0XAAAA; }
