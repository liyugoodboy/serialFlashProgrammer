//###########################################################################
//
//
// FILE:    SCI_GetFunction.c
//
//
/****************************************************************************
 *文件名称：SCI_GetFunction.c
 *文件说明：SCI引导
 *功能说明：
 *
 *
 *
 ****************************************************************************/
#include "c2_bootrom.h"
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
StatusCode statusCode;

Uint16 checksum;

/*****************************************************************************
 * 函数声明
 *****************************************************************************/
Uint16 SCIA_GetWordData(void);
Uint16 SCIA_GetOnlyWordData(void);
void SendACK(void);
void SendNAK(void);
inline Uint16 SCIA_GetACK(void);
inline void SCIA_Flush(void);
void SCIA_Init();
void SCIA_AutobaudLock(void);
Uint32 SCI_GetFunction(void);
Uint16 SCI_GetPacket(Uint16* length, Uint16* data);
Uint16 SCI_SendPacket(Uint16 command, Uint16 status, Uint16 length,
                      Uint16* data);
void SCI_SendWord(Uint16 word);
void SCI_SendChecksum(void);
void SignalCPU1(void);
extern Uint32 SCI_Boot(void);
extern void SCI_Erase(Uint32 sectors);
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
 * 返回值：
 *        Uint32  应用程序入口指针
 *****************************************************************************/
Uint32 SCI_GetFunction(void)
{
    Uint32 EntryAddr;
    Uint16 command;
    Uint16 data[10];
    Uint16 length;

    //
    // Assign GetWordData to the SCI-A version of the
    // function. GetWordData is a pointer to a function.
    //
    GetWordData = SCIA_GetWordData;

    //
    // Initialize the SCI-A port for communications
    // with the host.
    //
    //parameter SCI_BOOT for GPIO84,85; parameter SCI_BOOT_ALTERNATE for
    //GPIO28,29
    //
    SCIA_Init();
    SCIA_AutobaudLock();


    //SignalCPU1(); //CANNOT DO THIS YET

    command = SCI_GetPacket(&length, data);

    while(command != RESET_CPU2)
    {
        //
        //Reset the statusCode.
        //
        statusCode.status = NO_ERROR;
        statusCode.address = 0x12345678;
        checksum = 0;

        if(command == CPU2_UNLOCK_Z1)
        {
            Uint32 password0 = (Uint32)data[0] | ((Uint32)data[1] << 16);
            Uint32 password1 = (Uint32)data[2] | ((Uint32)data[3] << 16);
            Uint32 password2 = (Uint32)data[4] | ((Uint32)data[5] << 16);
            Uint32 password3 = (Uint32)data[6] | ((Uint32)data[7] << 16);

            //
            //Unlock the device
            //
            DcsmZ1Regs.Z1_CSMKEY0 = password0;
            DcsmZ1Regs.Z1_CSMKEY1 = password1;
            DcsmZ1Regs.Z1_CSMKEY2 = password2;
            DcsmZ1Regs.Z1_CSMKEY3 = password3;

            if(DcsmZ1Regs.Z1_CR.bit.UNSECURE == 0) //0 = Locked
            {
                statusCode.status = UNLOCK_ERROR;
            }
        }
        else if(command == CPU2_UNLOCK_Z2)
        {
            Uint32 password0 = (Uint32)data[0] | ((Uint32)data[1] << 16);
            Uint32 password1 = (Uint32)data[2] | ((Uint32)data[3] << 16);
            Uint32 password2 = (Uint32)data[4] | ((Uint32)data[5] << 16);
            Uint32 password3 = (Uint32)data[6] | ((Uint32)data[7] << 16);

            //
            //Unlock the device
            //
            DcsmZ2Regs.Z2_CSMKEY0 = password0;
            DcsmZ2Regs.Z2_CSMKEY1 = password1;
            DcsmZ2Regs.Z2_CSMKEY2 = password2;
            DcsmZ2Regs.Z2_CSMKEY3 = password3;

            if(DcsmZ2Regs.Z2_CR.bit.UNSECURE == 0) //0 = Locked
            {
                statusCode.status = UNLOCK_ERROR;
            }
        }
        else if(command == DFU_CPU2)
        {
            EntryAddr = SCI_Boot();//loads application into CPU1 FLASH

            if(statusCode.status == NO_ERROR)
            {
                statusCode.address = EntryAddr;
            }
       }
        else if(command == ERASE_CPU2)
        {
            Uint32 sectors = (Uint32)(((Uint32)data[1] << 16) |
                                      (Uint32)data[0]);
            SCI_Erase(sectors);
        }
        else if(command == VERIFY_CPU2)
        {
            VerifyData();
        }
        else if(command == RUN_CPU2)
        {
            EntryAddr = (Uint32)( ((Uint32)data[1] << 16) | (Uint32)data[0]);
            return(EntryAddr);
        }
        else
        {
            statusCode.status = COMMAND_ERROR;
        }

        //
        //send the packet and if NAK send again.
        //
        while(SCI_SendPacket(command, statusCode.status, 6,
                             (Uint16*)&statusCode.address)){}

        //
        //get next Packet
        //
        command = SCI_GetPacket(&length, data);

    }

    //
    // Leave control over flash pump
    //
    SignalCPU1();

    //
    //Reset with WatchDog Timeout
    //
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

    //
    // Receive an ACK or NAK
    //
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

    wordData = (Uint16)SciaRegs.SCIRXBUF.bit.SAR;

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
 * 返回值：
 *****************************************************************************/
void SCIA_Init()
{
    //
    // Enable the SCI-A clocks
    //
    EALLOW;
    CpuSysRegs.PCLKCR7.bit.SCI_A = 1;
    //ClkCfgRegs.LOSPCP.all = 0x0007;

    SciaRegs.SCIFFTX.all = 0x8000;

    //
    // 1 stop bit, No parity, 8-bit character
    // No loopback
    //
    SciaRegs.SCICCR.all = 0x0007;

    //
    // Enable TX, RX, Use internal SCICLK
    //
    SciaRegs.SCICTL1.all = 0x0003;

    //
    // Disable RxErr, Sleep, TX Wake,
    // Disable Rx Interrupt, Tx Interrupt
    //
    SciaRegs.SCICTL2.all = 0x0000;

    //
    // Relinquish SCI-A from reset
    //
    SciaRegs.SCICTL1.all = 0x0023;
    EDIS;

    return;
}

/*****************************************************************************
 * 函数名称：SCIA_AutobaudLock
 * 函数说明：自动波特率锁定
 * 输入值：
 * 返回值：
 *****************************************************************************/
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

    SCIA_Flush();
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

   //
   // Fetch the LSB and verify back to the host
   //
   while(SciaRegs.SCIRXST.bit.RXRDY != 1) { }

   wordData =  (Uint16)SciaRegs.SCIRXBUF.bit.SAR;
#if !checksum_enable
   SciaRegs.SCITXBUF.bit.TXDT = wordData;
#endif

   //
   // Fetch the MSB and verify back to the host
   //
   while(SciaRegs.SCIRXST.bit.RXRDY != 1) { }

   byteData =  (Uint16)SciaRegs.SCIRXBUF.bit.SAR;
#if !checksum_enable
   SciaRegs.SCITXBUF.bit.TXDT = byteData;
#endif
#if checksum_enable
   checksum += wordData + byteData;
#endif

   //
   // form the wordData from the MSB:LSB
   //
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

   //
   // Fetch the LSB and verify back to the host
   //
   while(SciaRegs.SCIRXST.bit.RXRDY != 1) { }

   wordData =  (Uint16)SciaRegs.SCIRXBUF.bit.SAR;
   //SciaRegs.SCITXBUF.bit.TXDT = wordData;

   //
   // Fetch the MSB and verify back to the host
   //
   while(SciaRegs.SCIRXST.bit.RXRDY != 1) { }

   byteData =  (Uint16)SciaRegs.SCIRXBUF.bit.SAR;
   //SciaRegs.SCITXBUF.bit.TXDT = byteData;
   checksum += wordData + byteData;

   //
   // form the wordData from the MSB:LSB
   //
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
    if(SCIA_GetOnlyWordData() != 0x1BE4)
    {
        SendNAK();
        return(0x100); //包起始格式错误
    }
    //获取数据长度
    *length = SCIA_GetOnlyWordData();

    checksum = 0;
    //获取命令
    Uint16 command = SCIA_GetOnlyWordData();

    //获取数据
    int i = 0;
    for(i = 0; i < (*length)/2; i++)
    {
        *(data+i) = SCIA_GetOnlyWordData();
    }

    Uint16 dataChecksum = checksum;
    //获取校验数据并校对
    if(dataChecksum != SCIA_GetOnlyWordData())
    {
        SendNAK();
        return(101);//数据校对错误
    }
    //接收帧尾
    if(SCIA_GetOnlyWordData() != 0xE41B)
    {
        SendNAK();
        return(102);//帧尾错误
    }
    //发送ACK信号
    SendACK();
    return(command);
}
/****************************************************************************
 * 函数名称:SendACK
 * 函数说明:SCIA发送ACK信号
 * 输入参数:
 * 输出参数:
 ****************************************************************************/
void SendACK(void)
{
    while(!SciaRegs.SCICTL2.bit.TXRDY) {}
    SciaRegs.SCITXBUF.bit.TXDT = ACK;
    SCIA_Flush();
}
/****************************************************************************
 * 函数名称:SendNAK
 * 函数说明:SCIA发送NAK信号
 * 输入参数:
 * 输出参数:
 ****************************************************************************/
void SendNAK(void)
{
    while(!SciaRegs.SCICTL2.bit.TXRDY){}
    SciaRegs.SCITXBUF.bit.TXDT = NAK;
    SCIA_Flush();
}
/****************************************************************************
 * 函数名称:SCIA_Flush
 * 函数说明:等待SCIA数据发送完成
 * 输入参数:
 * 输出参数:
 ****************************************************************************/
inline void SCIA_Flush(void)
{
    //等待SCI数据发送完成
    while(!SciaRegs.SCICTL2.bit.TXEMPTY){}
}
/****************************************************************************
 * 函数名称:SignalCPU1
 * 函数说明:给CPU1发送信号
 *        默认使用IPC_FLAG5，阻塞型函数，直到CPU1应答
 * 输入参数:
 * 输出参数:
 ****************************************************************************/
void SignalCPU1(void)
{
    //保留对flash控制
    ReleaseFlashPump();
    //设置IPC_FLAG5给CPU1,并等待CPU1响应
    IpcRegs.IPCSET.bit.IPC5 = 1;
    while(IpcRegs.IPCSTS.bit.IPC5 == 1){}
    asm(" NOP");
    asm(" NOP");
}

