/****************************************************************************
 * 文件名称：F2837xD_sci_flash_kernels_cpu01.c
 * 文件说明：使用SCI的F2837xD闪存编程解决方案。
 *
 * 功能说明: 用于单核或双核的SCI_Flash编程解决方案
 * 在此示例中，我们使用SCI与主机建立UART连接，接收CPU1执行的命令，然后在接收并完成
 * 任务后将ACK，NAK和状态数据包发送回主机。 该内核具有将CPU2编程，验证，解锁，重置，
 * 运行和引导到SCI引导加载程序的能力。每个命令都不希望来自命令包的数据或相对于该命令
 * 的特定数据。在本示例中，我们使用SCI与主机建立UART连接，接收以-sci8 ascii格式在
 * CPU01上运行的应用程序，并在设备上运行该命令。 将其编程到Flash中。
 *
 * 在例程中需要修改内容：
 *    1.需要修改程序成为FLASH工程
 *    2.固定应用程序位置：APP程序的codeStart位置固定在0x84000位置,而不是直接使用_c_init的位置。
 *    3.在一次bootloader后，进入本二次bootloader程序，所以此程序的codeStart位置占用0x80000。
 *    4.使用引脚控制是否进入升级过程中，如果引脚为低电平，则进入升级程序中，否则直接通过二次bootloader
 *    直接跳转到应用程序中。
 ****************************************************************************/
#include "F28x_Project.h"
#include "F2837xD_Ipc_drivers.h"
#include "Shared_Erase.h"
#include <string.h>
#include "flash_programming_c28.h"
#include "c1_bootrom.h"
#include "F021_F2837xD_C28x.h"

/**************************CPU2 IPC引导命令和模式********************************/
#define C1C2_BROM_IPC_EXECUTE_BOOTMODE_CMD    0x00000013
#define C1C2_BROM_BOOTMODE_BOOT_FROM_SCI      0x00000001
#define C1C2_BROM_BOOTMODE_BOOT_FROM_RAM      0x0000000A
#define C1C2_BROM_BOOTMODE_BOOT_FROM_FLASH    0x0000000B
/**********************************SCI模式************************************/
#define SCI_BOOT_CUSTOMIZE                    0x02
/**********************************配置信息************************************/
#define PROGRAM_APP_ENTERADDR_ADDR            0x84000//应用程序入口地址
/*****************************************************************************
 * 函数声明
 *****************************************************************************/
void Example_Error(Fapi_StatusType status);
void Init_Flash_Sectors(void);
extern Uint32 SCI_GetFunction(Uint32 BootMode);
bool upgradeJudgment();
/*****************************************************************************/
uint32_t main(void)
{
    /*初始化*/
    Device_init();
    Device_initGPIO();
    DINT;
    Interrupt_initModule();
    Interrupt_initVectorTable();
    EINT;
    ERTM;

    InitIpc();//初始化IPC


    /*升级信息判断*/
    if(upgradeJudgment() == false)
    {
        IpcRegs.IPCCLR.bit.IPC6 = 1;
        return PROGRAM_APP_ENTERADDR_ADDR;
    }


#ifndef _FLASH    
    InitFlash();//初始化FLASH
#endif  

    SeizeFlashPump();//等待扇区可用信号


    Init_Flash_Sectors();//初始化flash和API

    //修改指示灯
    GPIO_setDirectionMode(DEVICE_GPIO_PIN_LED2, GPIO_DIR_MODE_OUT);
    GPIO_setDirectionMode(DEVICE_GPIO_PIN_LED1, GPIO_DIR_MODE_OUT);
    GPIO_writePin(DEVICE_GPIO_PIN_LED1, 1);
    GPIO_writePin(DEVICE_GPIO_PIN_LED2, 1);


    GPIO_setDirectionMode(DEVICE_GPIO_PIN_LED2, GPIO_DIR_MODE_OUT);
    GPIO_setMasterCore(DEVICE_GPIO_PIN_LED2, GPIO_CORE_CPU2);


    Uint32 EntryAddr;
    //SCI升级功能函数(SCI_BOOT正常SCI引导)
    EntryAddr = SCI_GetFunction(SCI_BOOT_CUSTOMIZE);
    //返回入口点后在汇编文件中重新初始化C语言环境并跳转到入口点
    return(EntryAddr);
}
/*****************************************************************************
 * 函数名称：Init_Flash_Sectors
 * 函数说明：初始化flash和API
 * 输入值：
 * 返回值：
 *****************************************************************************/
#pragma CODE_SECTION(Init_Flash_Sectors,".TI.ramfunc");
void Init_Flash_Sectors(void)
{
    EALLOW;
    Flash0EccRegs.ECC_ENABLE.bit.ENABLE = 0x0;
    Fapi_StatusType oReturnCheck;
    //初始化API
    oReturnCheck = Fapi_initializeAPI(F021_CPU0_BASE_ADDRESS, 150);
    if(oReturnCheck != Fapi_Status_Success)
    {
        Example_Error(oReturnCheck);
    }
    //初始化Bank
    oReturnCheck = Fapi_setActiveFlashBank(Fapi_FlashBank0);
    if(oReturnCheck != Fapi_Status_Success)
    {
        Example_Error(oReturnCheck);
    }
    Flash0EccRegs.ECC_ENABLE.bit.ENABLE = 0xA;
    EDIS;
}
/*****************************************************************************
 * 函数名称：setUpdateFlag
 * 函数说明：升级判断
 *         此函数判断是否需要升级，可根据不同硬件相应修改此函数中的判断条件。
 * 输入值：
 * 返回值：1 进入升级模式      0 进入用户程序
 *****************************************************************************/
bool upgradeJudgment()
{
    /*方法1:引脚判断*/
    GPIO_setDirectionMode(0, GPIO_DIR_MODE_IN);
    if(GPIO_readPin(0) == 0)
    {
        return true;
    }
    return false;
}
/*****************************************************************************
 * 函数名称：Example_Error
 * 函数说明：程序错误
 * 输入值：  Fapi_StatusType status  Fapi状态
 * 返回值：
 *****************************************************************************/
#pragma CODE_SECTION(Example_Error,".TI.ramfunc");
void Example_Error(Fapi_StatusType status)
{
    while(1)
    {
        GPIO_togglePin(DEVICE_GPIO_PIN_LED2);
        DEVICE_DELAY_US(100000);
    }

}

/******************* (C) COPYRIGHT 2020 LIYU *****END OF FILE*****************/
