/********************************************************
**　  文件名称：main.c
**　  文件描述：
**　  功能说明：
**　  修改说明：
**　          版本：
**　  修改记录：
********************************************************/
#include "F28x_Project.h"
/******************************************************
函数名称:main
函数描述:主函数
输入参数:
返回值:
******************************************************/
int main(void)
{
    //芯片时钟初始化
    Device_init();
    //引脚初始化
    Device_initGPIO();
    //禁用中断
    DINT;
    //初始化中断模块
    Interrupt_initModule();
    //初始化中断向量表
    Interrupt_initVectorTable();
    //启用全局中断
    EINT;
    ERTM;

#ifdef _STANDALONE
    //初始化IPC
    InitIpc();
    //引导CPU2
#ifdef _FLASH
    //发送引导命令以允许CPU2应用程序开始执行,引导到FLASH中
    IPCBootCPU2(C1C2_BROM_BOOTMODE_BOOT_FROM_FLASH);
#else
    //发送引导命令以允许CPU02应用程序开始执行,引导到RAM中
    IPCBootCPU2(C1C2_BROM_BOOTMODE_BOOT_FROM_RAM);
#endif //#ifdef _FLASH
#endif //#ifdef _STANDALONE

    GPIO_setDirectionMode(DEVICE_GPIO_PIN_LED1, GPIO_DIR_MODE_OUT);
    GPIO_setDirectionMode(DEVICE_GPIO_PIN_LED2, GPIO_DIR_MODE_OUT);
    GPIO_setMasterCore(DEVICE_GPIO_PIN_LED2, GPIO_CORE_CPU2);


    while(1)
	{
        GPIO_togglePin(DEVICE_GPIO_PIN_LED1);
        DEVICE_DELAY_US(100000);
	}
}
