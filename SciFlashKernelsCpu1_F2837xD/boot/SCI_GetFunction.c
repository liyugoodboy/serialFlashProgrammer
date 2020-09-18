/****************************************************************************
 *文件名称：SCI_GetFunction.c
 *文件说明：SCI引导
 *功能说明：
 *
 *
 *
 ****************************************************************************/
#include "c1_bootrom.h"
#include "F2837xD_Examples.h"
#include "F2837xD_Gpio_defines.h"
#include "F2837xD_GlobalPrototypes.h"
#include "Shared_Erase.h"
#include "Shared_Verify.h"
#include "F2837xD_dcsm.h"
#include "device.h"
/******************************操作命令(与上位机相同定义)**************************/
#define DFU_CPU1                    0x0100
#define DFU_CPU2                    0x0200
#define ERASE_CPU1                  0x0300
#define ERASE_CPU2                  0x0400
#define VERIFY_CPU1                 0x0500
#define VERIFY_CPU2                 0x0600
#define CPU1_UNLOCK_Z1              0x000A
#define CPU1_UNLOCK_Z2              0x000B
#define CPU2_UNLOCK_Z1              0x000C
#define CPU2_UNLOCK_Z2              0x000D
#define RUN_CPU1                    0x000E
#define RESET_CPU1                  0x000F
#define RUN_CPU1_BOOT_CPU2          0x0004
#define RESET_CPU1_BOOT_CPU2        0x0007
#define RUN_CPU2                    0x0010
#define RESET_CPU2                  0x0020

/******************************错误(与上位机相同定义)******************************/
#define NO_ERROR                    0x1000
#define BLANK_ERROR                 0x2000
#define VERIFY_ERROR                0x3000
#define PROGRAM_ERROR               0x4000
#define COMMAND_ERROR               0x5000
#define UNLOCK_ERROR                0x6000

/*****************************响应信号(与上位机相同定义)****************************/
#define ACK                         0x2D
#define NAK                         0xA5

/**********************************全局变量************************************/
typedef struct
{
   Uint16 status;
   Uint32 address;
}  StatusCode;
StatusCode statusCode;     //编程状态与应用入口地址
Uint16 checksum;           //和校验码

/*****************************************************************************
 * 函数声明
 *****************************************************************************/
Uint16 SCIA_GetWordData(void);
Uint16 SCIA_GetOnlyWordData(void);
void SendACK(void);
void SendNAK(void);
inline Uint16 SCIA_GetACK(void);
inline void SCIA_Flush(void);
void SCIA_Init(Uint32  BootMode);
void SCIA_AutobaudLock(void);
void SCI_Pinmux_Option1(void);
void SCI_Pinmux_Option2(void);
Uint32 SCI_GetFunction(Uint32  BootMode);
Uint16 SCI_GetPacket(Uint16* length, Uint16* data);
Uint16 SCI_SendPacket(Uint16 command, Uint16 status, Uint16 length,
                      Uint16* data);
void SCI_SendWord(Uint16 word);
void SCI_SendChecksum(void);
extern void Boot_CPU2(Uint32 BootMode);
extern Uint32 SCI_Boot(Uint32 BootMode);
extern void Shared_Erase(Uint32 sectors);
extern void CopyData(void);
extern void VerifyData(void);
extern Uint32 GetLongData(void);
extern void ReadReservedFn(void);
/*****************************************************************************
 * 函数名称：SCI_GetFunction
 * 函数说明：
 *        此函数首先初始化SCIA并执行自动波特锁定。 它包含一个while循环，等待来自主机的命令。 它处理每个
 *     命令并发送响应，但运行和重置命令。 程序退出并分支到入口点。 重置时，内核退出while循环并等待看门狗超
 *     时复位器件。
 * 输入值：
 *        Uint32  BootMode  SCI引导模式(决定使用SCI的引脚)
 * 返回值：
 *        Uint32  应用程序入口指针
 *****************************************************************************/
#pragma CODE_SECTION(SCI_GetFunction,".TI.ramfunc");
Uint32 SCI_GetFunction(Uint32  BootMode)
{
    Uint32 EntryAddr;
    Uint16 command;
    Uint16 data[10];
    Uint16 length;
    //判断是否有SCIA外设
    if((DevCfgRegs.DC8.bit.SCI_A != 0x01))
    {
        return 0xFFFFFFFF;
    }
    //将GetWordData分配给该函数的SCI-A版本。 GetWordData是指向函数的指针。
    GetWordData = SCIA_GetWordData;


    //初始化SCIA端口
    SCIA_Init(BootMode);


    //指示灯亮表示进入boot功能
    GPIO_writePin(DEVICE_GPIO_PIN_LED1, 0);

    //自动波特率锁定
    //SCIA_AutobaudLock();


    //获取包数据
    command = SCI_GetPacket(&length, data);

    //判断命令
    while(command != RESET_CPU1)
    {
        //重置变量
        statusCode.status = NO_ERROR;
        statusCode.address = 0x12345678;
        checksum = 0;

/*******************CPU1_UNLOCK_Z1:解锁CPUZ1区功能******************************/
        if(command == CPU1_UNLOCK_Z1)
        {
            //读取密码
            Uint32 password0 = (Uint32)data[0] | ((Uint32)data[1] << 16);
            Uint32 password1 = (Uint32)data[2] | ((Uint32)data[3] << 16);
            Uint32 password2 = (Uint32)data[4] | ((Uint32)data[5] << 16);
            Uint32 password3 = (Uint32)data[6] | ((Uint32)data[7] << 16);
            //写入密码
            DcsmZ1Regs.Z1_CSMKEY0 = password0;
            DcsmZ1Regs.Z1_CSMKEY1 = password1;
            DcsmZ1Regs.Z1_CSMKEY2 = password2;
            DcsmZ1Regs.Z1_CSMKEY3 = password3;
            //判断是否解锁成功
            if(DcsmZ1Regs.Z1_CR.bit.UNSECURE == 0)
            {
                //解锁失败错误
                statusCode.status = UNLOCK_ERROR;
            }
        }
/********************CPU1_UNLOCK_Z2:解锁CPUZ2区功能*****************************/
        else if(command == CPU1_UNLOCK_Z2)
        {
            //接收密码
            Uint32 password0 = (Uint32)data[0] | ((Uint32)data[1] << 16);
            Uint32 password1 = (Uint32)data[2] | ((Uint32)data[3] << 16);
            Uint32 password2 = (Uint32)data[4] | ((Uint32)data[5] << 16);
            Uint32 password3 = (Uint32)data[6] | ((Uint32)data[7] << 16);
            //解锁设备
            DcsmZ2Regs.Z2_CSMKEY0 = password0;
            DcsmZ2Regs.Z2_CSMKEY1 = password1;
            DcsmZ2Regs.Z2_CSMKEY2 = password2;
            DcsmZ2Regs.Z2_CSMKEY3 = password3;
            //判断是否解锁成功
            if(DcsmZ2Regs.Z2_CR.bit.UNSECURE == 0)
            {
                //解锁失败错误
                statusCode.status = UNLOCK_ERROR;
            }
        }
/***********************DFU_CPU1:CPU1程序升级功能********************************/
        else if(command == DFU_CPU1)
        {
            //从上位机发送的程序文件中接收引导信息并把文件烧写到FLASH中
            EntryAddr = SCI_Boot(BootMode);
            //判断引导信息是否正确
            if(statusCode.status == NO_ERROR)
            {
                //程序入口地址
                statusCode.address = EntryAddr;
            }
        }
/*********************ERASE_CPU1:CPU1扇区擦除功能*********************************/
        else if(command == ERASE_CPU1)
        {
            //获取要擦除的扇区
            Uint32 sectors = (Uint32)(((Uint32)data[1] << 16) |
                                      (Uint32)data[0]);
            //扇区擦除
            Shared_Erase(sectors);
        }
/*********************VERIFY_CPU1:CPU1验证*************************************/
        else if(command == VERIFY_CPU1)
        {
            //验证FLASH中数据是否与发送的应用数据一致
            VerifyData();
        }
/*********************RUN_CPU1:运行CPU1****************************************/
        else if(command == RUN_CPU1)
        {
            //获取程序入口点信息
            EntryAddr = (Uint32)( ((Uint32)data[1] << 16) | (Uint32)data[0]);
            return(EntryAddr);
        }
/****************RUN_CPU1_BOOT_CPU2:引导CPU2后运行CPU1***************************/
        else if(command == RUN_CPU1_BOOT_CPU2)
        {
            Boot_CPU2(BootMode);
            EntryAddr = (Uint32)( ((Uint32)data[1] << 16) | (Uint32)data[0]);
            return(EntryAddr);
        }
/************* RESET_CPU1_BOOT_CPU2:引导CPU2后复位CPU1***************************/
        else if(command == RESET_CPU1_BOOT_CPU2)
        {
            //此函数引导CPU2直到CPU2 bootloader2程序退出
            Boot_CPU2(BootMode);
            break;
        }
/*********************COMMAND_ERROR:错误命令***********************************/
        else
        {
            statusCode.status = COMMAND_ERROR;
        }
        //发送状态包直到接收到ACK信号
        while(SCI_SendPacket(command, statusCode.status, 6,
                             (Uint16*)&statusCode.address))
        {}
        //重新等待接收到新命令包
        command = SCI_GetPacket(&length, data);
    }
/******************************RESET_CPU1:复位CPU1功能*************************/
    //开启看门狗，等待看门狗超时复位
    EALLOW;
    WdRegs.SCSR.all = 0;    //enable WDRST
    WdRegs.WDCR.all = 0x28; //enable WD
    EDIS;
    while(1){}
}
/*****************************************************************************
 * 函数名称：SCI_SendPacket
 * 函数说明：发送数据包
 * 输入值：
 *        Uint16  command 命令
 *        Uint16  status  状态
 *        Uint16  length  数据长度
 *        Uint16* data    数据指针
 * 返回值：
 *        Uint16  应答情况
 *****************************************************************************/
Uint16 SCI_SendPacket(Uint16 command, Uint16 status, Uint16 length,
                      Uint16* data)
{
    int i;

    SCIA_Flush();
    DELAY_US(100000);
    SCI_SendWord(0x1BE4);
    SCI_SendWord(length);

    checksum = 0;
    SCI_SendWord(command);
    SCI_SendWord(status);

    for(i = 0; i < (length-2)/2; i++)
    {
        SCI_SendWord(*(data + i));
    }
    SCI_SendChecksum();
    SCI_SendWord(0xE41B);

    //接收应答信号
    return SCIA_GetACK();
}
/*****************************************************************************
 * 函数名称：SCIA_GetACK
 * 函数说明：获取主机ACK信号
 * 输入值：
 * 返回值：
 *        Uint16  应答情况
 *****************************************************************************/
inline Uint16 SCIA_GetACK()
{
    Uint16 wordData;
    while(SciaRegs.SCIRXST.bit.RXRDY != 1) { }

    wordData =  (Uint16)SciaRegs.SCIRXBUF.bit.SAR;
    if(wordData != ACK)
    {
        return(1);
    }

    return(0);
}
/*****************************************************************************
 * 函数名称：SCI_SendChecksum
 * 函数说明：SCIA发送校验和数据
 * 输入值：
 * 返回值：
 *****************************************************************************/
void SCI_SendChecksum()
{
    SciaRegs.SCITXBUF.bit.TXDT = (checksum & 0xFF); //LSB
    SCIA_Flush();
    SCIA_GetACK();

    SciaRegs.SCITXBUF.bit.TXDT = (checksum >> 8) & 0xFF; //MSB
    SCIA_Flush();
    SCIA_GetACK();
}

/*****************************************************************************
 * 函数名称：SCI_SendWord
 * 函数说明：SCIA发送2字节数据(先LSB后MSB)
 * 输入值：
 *        Uint16 word 数据
 * 返回值：
 *****************************************************************************/
void SCI_SendWord(Uint16 word)
{
    SciaRegs.SCITXBUF.bit.TXDT = (word & 0xFF); //LSB
    checksum += word & 0xFF;
    SCIA_Flush();
    SCIA_GetACK();

    SciaRegs.SCITXBUF.bit.TXDT = (word>>8) & 0xFF; //MSB
    checksum += word>>8 & 0xFF;
    SCIA_Flush();
    SCIA_GetACK();
}
/*****************************************************************************
 * 函数名称：SCIA_Init
 * 函数说明：SCIA初始化
 * 输入值：
 *        Uint32 BootMode SCI引导模式
 * 返回值：
 *****************************************************************************/
void SCIA_Init(Uint32  BootMode)
{
    //延时等待设备稳定
    DEVICE_DELAY_US(1000000);
    //由模式判断使用的引脚
    if((BootMode & 0xF0) == 0x0)
    {
        //SCI_Pinmux_Option1();
    }
    else
    {
        //SCI_Pinmux_Option2();
    }
    GPIO_SetupPinOptions(42, GPIO_OUTPUT, GPIO_ASYNC);
    GPIO_SetupPinMux(42,GPIO_MUX_CPU1,15);
    GPIO_SetupPinOptions(43, GPIO_INPUT, GPIO_ASYNC);
    GPIO_SetupPinMux(43,GPIO_MUX_CPU1,15);
    //失能模块
    SCI_disableModule(SCIA_BASE);
    //复位模块
    SCI_performSoftwareReset(SCIA_BASE);
    //波特率115200 8位数据 1个停止位  无校验
    SCI_setConfig(SCIA_BASE, DEVICE_LSPCLK_FREQ, 115200,
           (SCI_CONFIG_WLEN_8 |SCI_CONFIG_STOP_ONE |
                                   SCI_CONFIG_PAR_NONE));
    //重置通道
    SCI_resetChannels(SCIA_BASE);
    SCI_resetRxFIFO(SCIA_BASE);
    SCI_resetTxFIFO(SCIA_BASE);
    //清除中断标志
    SCI_clearInterruptStatus(SCIA_BASE, SCI_INT_TXFF | SCI_INT_RXFF);
    //启用模块
    SCI_enableModule(SCIA_BASE);
    //复位模块
    SCI_performSoftwareReset(SCIA_BASE);
    //延时等待设备稳定
    DEVICE_DELAY_US(1000000);
    return;
}

//
// SCIA_AutobaudLock - Perform autobaud lock with the host.
//                     Note that if autobaud never occurs
//                     the program will hang in this routine as there
//                     is no timeout mechanism included.
//
void SCIA_AutobaudLock(void)
{
    Uint16 byteData;

    //
    // Must prime baud register with >= 1
    //
    SciaRegs.SCIHBAUD.bit.BAUD = 0;
    SciaRegs.SCILBAUD.bit.BAUD = 1;

    //
    // Prepare for autobaud detection
    // Set the CDC bit to enable autobaud detection
    // and clear the ABD bit
    //
    SciaRegs.SCIFFCT.bit.CDC = 1;
    SciaRegs.SCIFFCT.bit.ABDCLR = 1;

    //
    // Wait until we correctly read an
    // 'A' or 'a' and lock
    //
    while(SciaRegs.SCIFFCT.bit.ABD != 1) {}

    //
    // After autobaud lock, clear the ABD and CDC bits
    //
    SciaRegs.SCIFFCT.bit.ABDCLR = 1;
    SciaRegs.SCIFFCT.bit.CDC = 0;

    while(SciaRegs.SCIRXST.bit.RXRDY != 1) { }
    byteData = SciaRegs.SCIRXBUF.bit.SAR;
    SciaRegs.SCITXBUF.bit.TXDT = byteData;

    return;
}

/*****************************************************************************
 * 函数名称：SCIA_GetWordData
 * 函数说明：从SCIA端口获取两个字节数据(宏定义控制回显)
 *        此函数从SCI-A端口获取两个字节，并将它们放在一起以形成单个16位值。 假定主机以LSB和MSB的顺
 *        序发送数据。
 * 输入值：
 * 返回值：  Uint16 获取到的数据
 *****************************************************************************/
Uint16 SCIA_GetWordData(void)
{
   Uint16 wordData;
   Uint16 byteData;

   wordData = 0x0000;
   byteData = 0x0000;

   //获取LSB数据
   while(SciaRegs.SCIRXST.bit.RXRDY != 1) { }
   wordData =  (Uint16)SciaRegs.SCIRXBUF.bit.SAR;

#if !checksum_enable
   SciaRegs.SCITXBUF.bit.TXDT = wordData;
#endif

   //获取MSB数据
   while(SciaRegs.SCIRXST.bit.RXRDY != 1) { }
   byteData =  (Uint16)SciaRegs.SCIRXBUF.bit.SAR;

#if !checksum_enable
   SciaRegs.SCITXBUF.bit.TXDT = byteData;
#endif
   //校验和
#if checksum_enable
   checksum += wordData + byteData;
#endif

   //组合数据MSB:LSB
   wordData |= (byteData << 8);

   return wordData;
}
/*****************************************************************************
 * 函数名称：SCI_GetPacket
 * 函数说明：从SCIA端口获取两个字节数据
 *        此函数从SCI-A端口获取两个字节，并将它们放在一起以形成单个16位值。 假定主机以LSB和MSB的顺
 *        序发送数据。
 * 输入值：
 * 返回值：  Uint16 获取到的数据
 *****************************************************************************/
Uint16 SCIA_GetOnlyWordData(void)
{
   Uint16 wordData;
   Uint16 byteData;

   wordData = 0x0000;
   byteData = 0x0000;

   //获取LSB数据
   while(SciaRegs.SCIRXST.bit.RXRDY != 1) { }
   wordData = (Uint16)SciaRegs.SCIRXBUF.bit.SAR;
   //数据回显
   //SciaRegs.SCITXBUF.bit.TXDT = wordData;

   //获取MSB数据
   while(SciaRegs.SCIRXST.bit.RXRDY != 1) { }
   byteData =  (Uint16)SciaRegs.SCIRXBUF.bit.SAR;
   //数据回显
   //SciaRegs.SCITXBUF.bit.TXDT = byteData;

   //校验和
   checksum += wordData + byteData;

   //组合数据MSB:LSB
   wordData |= (byteData << 8);

   return wordData;
}
/*****************************************************************************
 * 函数名称：SCI_GetPacket
 * 函数说明：从SCIA端口获取包数据
 * 输入值：
 *        Uint16* length 数据长度指针
 *        Uint16* data   数据指针
 * 返回值：
 *        Uint16 命令
 *****************************************************************************/
Uint16 SCI_GetPacket(Uint16* length, Uint16* data)
{
    //接收数据包头并校验
    if(SCIA_GetOnlyWordData() != 0x1BE4)
    {
        SendNAK();
        return(100);//数据头错误
    }
    //数据长度
    *length = SCIA_GetOnlyWordData();

    //重置和校验，以校验命令和数据
    checksum = 0;
    //命令
    Uint16 command = SCIA_GetOnlyWordData();
    //获取数据
    int i = 0;
    for(i = 0; i < (*length)/2; i++)
    {
        *(data+i) = SCIA_GetOnlyWordData();
    }

    //接收校验数据并校验
    Uint16 dataChecksum = checksum;
    if(dataChecksum != SCIA_GetOnlyWordData())
    {
        //校验失败发送NAK信号
        SendNAK();
        return(101);
    }
    //获取数据包尾并校验
    if(SCIA_GetOnlyWordData() != 0xE41B)
    {
        //包尾数据错误发送NAK信号
        SendNAK();
        return(102);
    }
    //发送成功信号
    SendACK();
    //返回命令
    return(command);
}
/*****************************************************************************
 * 函数名称：SendACK
 * 函数说明：发送ACK信号
 * 输入值：
 * 返回值：
 *****************************************************************************/
void SendACK(void)
{
    while(!SciaRegs.SCICTL2.bit.TXRDY){}
    SciaRegs.SCITXBUF.bit.TXDT = ACK;
    SCIA_Flush();
}
/*****************************************************************************
 * 函数名称：SendNAK
 * 函数说明：发送NAK信号
 * 输入值：
 * 返回值：
 *****************************************************************************/
void SendNAK(void)
{
    while(!SciaRegs.SCICTL2.bit.TXRDY){}
    SciaRegs.SCITXBUF.bit.TXDT = NAK;
    SCIA_Flush();
}
/*****************************************************************************
 * 函数名称：SCIA_Flush
 * 函数说明：等待SCIA发送器清空
 * 输入值：
 * 返回值：
 *****************************************************************************/
inline void SCIA_Flush(void)
{
    //等待SCITXBUF空
    while(!SciaRegs.SCICTL2.bit.TXEMPTY){}
}
/*****************************************************************************
 * 函数名称：SCI_Pinmux_Option1
 * 函数说明：GPIO配置方案1(配置为SCI引导模式1默认引脚)
 *        GPIO84 as SCITXDA (输出引脚)
 *        GPIO85 as SCIRXDA (输入引脚)
 * 输入值：
 * 返回值：
 *****************************************************************************/
void SCI_Pinmux_Option1(void)
{
    EALLOW;
    GPIO_SetupPinOptions(84, GPIO_OUTPUT, GPIO_ASYNC);
    GPIO_SetupPinMux(84,GPIO_MUX_CPU1,5);
    GPIO_SetupPinOptions(85, GPIO_INPUT, GPIO_ASYNC);
    GPIO_SetupPinMux(85,GPIO_MUX_CPU1,5);
    EDIS;
}
/*****************************************************************************
 * 函数名称：SCI_Pinmux_Option2
 * 函数说明：GPIO配置方案2(配置为SCI引导模式2默认引脚)
 *        GPIO29 as SCITXDA (输出引脚)
 *        GPIO28 as SCIRXDA (输入引脚)
 * 输入值：
 * 返回值：
 *****************************************************************************/
void SCI_Pinmux_Option2(void)
{
    EALLOW;
    GPIO_SetupPinOptions(29, GPIO_OUTPUT, GPIO_ASYNC);
    GPIO_SetupPinMux(29,GPIO_MUX_CPU1,1);
    GPIO_SetupPinOptions(28, GPIO_INPUT, GPIO_ASYNC);
    GPIO_SetupPinMux(28,GPIO_MUX_CPU1,1);
    EDIS;
}
/******************* (C) COPYRIGHT 2020 LIYU *****END OF FILE*****************/
