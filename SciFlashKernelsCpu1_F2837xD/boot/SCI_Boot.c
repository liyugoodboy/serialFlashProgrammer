//###########################################################################
//
// FILE:    SCI_Boot.c
//
// TITLE:   SCI Boot mode routines
//
// Functions:
//
//     Uint32 SCI_Boot(void)
//     inline void SCIA_Init(void)
//     inline void SCIA_AutobaudLock(void)
//     Uint32 SCIA_GetWordData(void)
//
// Notes:
//
//###########################################################################
// $TI Release: F2837xD Support Library v3.09.00.00 $
// $Release Date: Thu Mar 19 07:35:24 IST 2020 $
// $Copyright:
// Copyright (C) 2013-2020 Texas Instruments Incorporated - http://www.ti.com/
//
// Redistribution and use in source and binary forms, with or without 
// modification, are permitted provided that the following conditions 
// are met:
// 
//   Redistributions of source code must retain the above copyright 
//   notice, this list of conditions and the following disclaimer.
// 
//   Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimer in the 
//   documentation and/or other materials provided with the   
//   distribution.
// 
//   Neither the name of Texas Instruments Incorporated nor the names of
//   its contributors may be used to endorse or promote products derived
//   from this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// $
//###########################################################################

//
// Included Files
//
#include "c1_bootrom.h"
#include "F2837xD_Gpio_defines.h"
#include "F2837xD_GlobalPrototypes.h"
#include "F2837xD_Ipc_drivers.h"
#include "Types.h"

//
// Defines
//
#define NO_ERROR                            0x1000
#define BLANK_ERROR                         0x2000
#define VERIFY_ERROR                        0x3000
#define PROGRAM_ERROR                       0x4000
#define COMMAND_ERROR                       0x5000
#define C1C2_BROM_IPC_EXECUTE_BOOTMODE_CMD  0x00000013
#define C1C2_BROM_BOOTMODE_BOOT_FROM_SCI    0x00000001
#define C1C2_BROM_BOOTMODE_BOOT_FROM_RAM    0x0000000A
#define C1C2_BROM_BOOTMODE_BOOT_FROM_FLASH  0x0000000B

//
// Globals
//
typedef struct
{
   Uint16 status;
   Uint32 address;
}  StatusCode;
extern StatusCode statusCode;

//
// Function Prototypes
//
extern Uint16 SCIA_GetWordData(void);
extern Uint32 CopyData(void);
Uint32 GetLongData(void);
extern void ReadReservedFn(void);
extern void Example_Error(Fapi_StatusType status);
Uint32 SCI_Boot(Uint32 BootMode);
void Boot_CPU2(Uint32 BootMode);
void assignSharedRAMstoCPU2(void);
void Assign_SCIA_IO_CPU2(Uint32 BootMode);

//
// SCI_Boot - This module is the main SCI boot routine.
//            It will load code via the SCI-A port.
//
//            It will return a entry point address back
//            to the InitBoot routine which in turn calls
//            the ExitBoot routine.
//
Uint32 SCI_Boot(Uint32 BootMode)
{
    statusCode.status = NO_ERROR;
    statusCode.address = 0x12346578;

    Uint32 EntryAddr;

    //
    // Assign GetWordData to the SCI-A version of the
    // function. GetWordData is a pointer to a function.
    //
    GetWordData = SCIA_GetWordData;

    //
    // If the KeyValue was invalid, abort the load
    // and return the flash entry point.
    //
    if (SCIA_GetWordData() != 0x08AA)
    {
        statusCode.status = VERIFY_ERROR;
        statusCode.address = FLASH_ENTRY_POINT;
    }

    ReadReservedFn(); //reads and discards 8 reserved words

    EntryAddr = GetLongData();

    CopyData();

    Uint16 x = 0;
    for(x = 0; x < 32676; x++){}
    for(x = 0; x < 32676; x++){}

    return EntryAddr;
}
/*****************************************************************************
 * 函数名称：Boot_CPU2
 * 函数说明：通过SCI方式引导CPU2,并等待CPU2完成bootloader2
 * 输入值：
 *        Uint32  BootMode  SCI引导模式引脚配置
 * 返回值：
 *****************************************************************************/
void Boot_CPU2(Uint32 BootMode)
{
    //保持对flash的控制
    ReleaseFlashPump();

    EALLOW;
    while(1)
    {
        //获取CPU2的引导状态
        uint32_t bootStatus = IPCGetBootStatus() & 0x0000000F;

        //判断状态
        if (bootStatus == C2_BOOTROM_BOOTSTS_SYSTEM_READY)
        {
            break;
        }
        else if(bootStatus == C2_BOOTROM_BOOTSTS_C2TOC1_BOOT_CMD_ACK)
        {
            Example_Error(Fapi_Error_Fail);
        }
    }

    //等待CPU02控制系统IPC标志1和32可用
    while(IPCLtoRFlagBusy(IPC_FLAG0) | IPCLtoRFlagBusy(IPC_FLAG31)){}

    CpuSysRegs.PCLKCR7.bit.SCI_A = 1;
    DevCfgRegs.SOFTPRES7.bit.SCI_A = 1;

    int a = 0;
    for(a = 0; a < 32676; a++){}

    CpuSysRegs.PCLKCR7.bit.SCI_A = 0;
    DevCfgRegs.SOFTPRES7.bit.SCI_A = 0;
    //SCIA移交控制权
    Assign_SCIA_IO_CPU2(BootMode);
    //分享GSRAM
    assignSharedRAMstoCPU2();

    //CPU1至CPU2 IPC引导模式寄存器，通过bootrom引导CPU2
    if(BootMode == 0x02)
    {
        IpcRegs.IPCBOOTMODE = C1C2_BROM_BOOTMODE_BOOT_FROM_FLASH;
    }
    else
    {
        IpcRegs.IPCBOOTMODE = C1C2_BROM_BOOTMODE_BOOT_FROM_SCI;
    }
    //给CPU2发送引导命令
    IpcRegs.IPCSENDCOM = C1C2_BROM_IPC_EXECUTE_BOOTMODE_CMD;
    //设置标志
    IpcRegs.IPCSET.all = 0x80000001;
    EDIS;

    EALLOW;
    //设置标志通知CPU2为boot功能模式
    IpcRegs.IPCSET.bit.IPC6 = 1;
    //等待CPU2发送bootloader2完成信号
    while(IpcRegs.IPCSTS.bit.IPC5 != 1){}
    //应答CPU2
    IpcRegs.IPCACK.bit.IPC5 = 1;
    EDIS;
}
/*****************************************************************************
 * 函数名称：Assign_SCIA_IO_CPU2
 * 函数说明：将SCIA模块分配给CPU2控制
 * 输入值：
 * 返回值：
 *****************************************************************************/
void Assign_SCIA_IO_CPU2(Uint32 BootMode)
{
    EALLOW;
    DevCfgRegs.CPUSEL5.bit.SCI_A = 1;    //SCIA控制权给CPU2
    ClkCfgRegs.CLKSEM.all = 0xA5A50000;  //允许CPU2 bootrom来控制时钟配置寄存器
    ClkCfgRegs.LOSPCP.all = 0x0007;

    if(BootMode == 0x1)
    {
        //配置1:SCI引导默认使用引脚
        GPIO_SetupPinOptions(84, GPIO_OUTPUT, GPIO_ASYNC);
        GPIO_SetupPinMux(84,GPIO_MUX_CPU2,5);
        GPIO_SetupPinOptions(85, GPIO_INPUT, GPIO_ASYNC);
        GPIO_SetupPinMux(85,GPIO_MUX_CPU2,5);
    }
    else if(BootMode == 0x81)
    {
        //配置2:SCI引导可修改使用引脚
        GPIO_SetupPinOptions(29, GPIO_OUTPUT, GPIO_ASYNC);
        GPIO_SetupPinMux(29,GPIO_MUX_CPU2,1);
        GPIO_SetupPinOptions(28, GPIO_INPUT, GPIO_ASYNC);
        GPIO_SetupPinMux(28,GPIO_MUX_CPU2,1);
    }
    else
    {
        //自定义配置:bootloader2不通过SCI传输
        GPIO_SetupPinOptions(42, GPIO_OUTPUT, GPIO_ASYNC);
        GPIO_SetupPinMux(42,GPIO_MUX_CPU2,15);
        GPIO_SetupPinOptions(43, GPIO_INPUT, GPIO_ASYNC);
        GPIO_SetupPinMux(43,GPIO_MUX_CPU2,15);
    }
    EDIS;
}

/*****************************************************************************
 * 函数名称：assignSharedRAMstoCPU2
 * 函数说明：给CPU2分享GSRAM
 *        默认分享GSRAM2和GSRAM3
 * 输入值：
 * 返回值：
 *****************************************************************************/
void assignSharedRAMstoCPU2(void)
{
//    EALLOW;
//    MemCfgRegs.GSxMSEL.bit.MSEL_GS0 = 1;
//    MemCfgRegs.GSxMSEL.bit.MSEL_GS1 = 1;
//    MemCfgRegs.GSxMSEL.bit.MSEL_GS2 = 1;
//    MemCfgRegs.GSxMSEL.bit.MSEL_GS3 = 1;
//    EDIS;
}
