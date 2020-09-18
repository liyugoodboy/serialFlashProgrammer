/****************************************************************************
 * 文件名称：F2837xD_sci_flash_kernels_cpu02.c
 * 文件说明：使用SCI的F2837xD闪存编程解决方案。
 *
 *        此为双核设备CPU2的bootloader2程序,实现SCI接口烧写flash工程。
 *
 * 在例程中需要修改内容：
 *    1.需要修改程序成为FLASH工程
 *    2.在CPU1引导CPU2运行后,进入此bootloader2程序，此时需判断是CPU1的bootloader2引导运行的
 *    还是APP程序引导运行，如果是CPU1的boodloader2引导则进入升级功能，否则直接跳转到APP程序中
 *    3.固定APP程序入口地址:0x84000
 ****************************************************************************/
#include "F28x_Project.h"
#include "F2837xD_Ipc_drivers.h"
#include "Shared_Erase.h"
#include <string.h>
#include "flash_programming_c28.h"
#include "c2_bootrom.h"
#include "F021_F2837xD_C28x.h"
/*********************************函数声明**************************************/
extern void Example_Error(Fapi_StatusType status);
extern Uint32 SCI_GetFunction(void);
extern SignalCPU1(void);
void Init_Flash_Sectors(void);

/**********************************配置信息************************************/
#define PROGRAM_APP_ENTERADDR_ADDR            0x84000//应用程序入口地址
/****************************************************************************
 * 函数名称:main
 * 函数说明:主函数
 *        此函数会在升级程序结束后，返回APP程序的入口地址。
 *
 ****************************************************************************/
uint32_t main(void)
{
    //等待SCI中数据全部发送完成
    //while(!SciaRegs.SCICTL2.bit.TXEMPTY){}
    //设备初始化(宏定义CPU2后将不会操作时钟)
    Device_init();
    //初始化PIE
    DINT;
    Interrupt_initModule();
    Interrupt_initVectorTable();
    EINT;
    ERTM;

    //模拟引导bootloader程序的IPC通信格式：系统进入等待状态，等待CPI下发引导模式
    //IpcRegs.IPCBOOTSTS = C2_BOOTROM_BOOTSTS_SYSTEM_READY;
    //while(IpcRegs.IPCBOOTMODE != C1C2_BROM_BOOTMODE_BOOT_FROM_FLASH){}

    DEVICE_DELAY_US(10000);
    //判断引导方
    //检查IPC_FLAG6标志
    if(IpcRegs.IPCSTS.bit.IPC6 == 0)
    {
        return PROGRAM_APP_ENTERADDR_ADDR;
    }

    //初始化FLASH到可操作状态
    SeizeFlashPump();
    Init_Flash_Sectors();


    //进入功能函数
    uint32_t EntryAddr = SCI_GetFunction();

    //发送信号给CPU1
    SignalCPU1();

    return EntryAddr;
}

/*****************************************************************************
 * 函数名称：Init_Flash_Sectors
 * 函数说明：初始化flash和API
 * 输入值：
 * 返回值：
 *****************************************************************************/
void Init_Flash_Sectors(void)
{
    EALLOW;
    Flash0EccRegs.ECC_ENABLE.bit.ENABLE = 0x0;
    Fapi_StatusType oReturnCheck;
    oReturnCheck = Fapi_initializeAPI(F021_CPU0_BASE_ADDRESS, 150);

    if(oReturnCheck != Fapi_Status_Success)
    {
        Example_Error(oReturnCheck);
    }

    oReturnCheck = Fapi_setActiveFlashBank(Fapi_FlashBank0);

    if(oReturnCheck != Fapi_Status_Success)
    {
        Example_Error(oReturnCheck);
    }
    Flash0EccRegs.ECC_ENABLE.bit.ENABLE = 0xA;
    EDIS;
}

/*****************************************************************************
 * 函数名称：Example_Error
 * 函数说明：程序错误
 * 输入值：  Fapi_StatusType status  Fapi状态
 * 返回值：
 *****************************************************************************/
#ifdef __TI_COMPILER_VERSION__
    #if __TI_COMPILER_VERSION__ >= 15009000
        #pragma CODE_SECTION(Example_Error,".TI.ramfunc");
    #else
        #pragma CODE_SECTION(Example_Error,"ramfuncs");
    #endif
#endif
void Example_Error(Fapi_StatusType status)
{
    __asm("    ESTOP0");
}

//
// End of file
//
