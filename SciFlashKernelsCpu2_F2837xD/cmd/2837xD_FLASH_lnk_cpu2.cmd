/*bootloader文件*/
/*固件升级应用:
    芯片默认flash引导的入口地址为:0x80000
    固件升级应用占用FLASHA和FLASHB两个数据段存储程序以及配置信息。所以用户程序应不得占用此部分FLASH。
*/
MEMORY
{
PAGE 0 :  /*程序空间*/

   /* BEGIN is used for the "boot to Flash" bootloader mode */
   BEGIN           	: origin = 0x080000, length = 0x000002
   /*FLASH加载到RAM*/
   RAMLS3_5         : origin = 0x009800, length = 0x001800
   /*程序段*/
   FLASHA           : origin = 0x080002, length = 0x001FFE
   FLASHB           : origin = 0x082000, length = 0x002000
   /*复位*/
   RESET           	: origin = 0x3FFFC0, length = 0x000002

PAGE 1 : /*数据空间*/

   BOOT_RSVD        : origin = 0x000002, length = 0x000121
   RAMM1            : origin = 0x000400, length = 0x0003F8
   RAMD1            : origin = 0x00B800, length = 0x000800
   RAMM0           	: origin = 0x000123, length = 0x0002DD
   RAMD0           	: origin = 0x00B000, length = 0x000800
   RAMLS0          	: origin = 0x008000, length = 0x000800
   RAMLS1          	: origin = 0x008800, length = 0x000800
   RAMLS2      		: origin = 0x009000, length = 0x000800
/* RAMLS3      		: origin = 0x009800, length = 0x000800
   RAMLS4      		: origin = 0x00A000, length = 0x000800
   RAMLS5           : origin = 0x00A800, length = 0x000800*/


   CPU2TOCPU1RAM   : origin = 0x03F800, length = 0x000400
   CPU1TOCPU2RAM   : origin = 0x03FC00, length = 0x000400
}

SECTIONS
{
   /*程序入口*/
   codestart        : > BEGIN       PAGE = 0, ALIGN(4)
   /*代码段*/
   .text            : >> FLASHA | FLASHB,     PAGE = 0,  ALIGN(4)
   /*初始化的全局变量和静态变量表*/
   .cinit           : > FLASHA,     PAGE = 0,  ALIGN(4)
   /*全局对象构造函数表*/
   .pinit           : > FLASHA,     PAGE = 0,  ALIGN(4)
   /*switch表格*/
   .switch          : > FLASHA,     PAGE = 0,  ALIGN(4)
   /*全局常量*/
   .econst          : > FLASHB,     PAGE = 0,  ALIGN(4)
   /*全局常量*/
   .const           : > FLASHB,     PAGE = 0,  ALIGN(4)
   /*复位向量*/
   .reset           : > RESET,      PAGE = 0, TYPE = DSECT /* not used, */
   /*堆栈*/
   .stack           : > RAMD0,      PAGE = 1,  ALIGN(4)
   /*全局变量和静态变量*/
   .ebss            : > RAMM1,      PAGE = 1,  ALIGN(4)
   /*初始化的全局变量和静态变量*/
   .data            : > RAMM1,      PAGE = 1,  ALIGN(4)
   /*未初始化全局变量和静态变量*/
   .bss             : > RAMM1,      PAGE = 1,  ALIGN(4)
   /*堆数据*/
   .esysmem         : > RAMLS2,     PAGE = 1,  ALIGN(4)
   /*CI/O数据流*/
   .cio             : > RAMM0,      PAGE = 1,  ALIGN(4)

   /*FLASH加载到RAM段*/
   .TI.ramfunc :    {} LOAD = FLASHB,
                       RUN = RAMLS3_5,
                       LOAD_START(_RamfuncsLoadStart),
                       LOAD_SIZE(_RamfuncsLoadSize),
                       LOAD_END(_RamfuncsLoadEnd),
                       RUN_START(_RamfuncsRunStart),
                       RUN_SIZE(_RamfuncsRunSize),
                       RUN_END(_RamfuncsRunEnd),
                       PAGE = 0, ALIGN(4)

   /* IPC API驱动*/
    GROUP : > CPU2TOCPU1RAM, PAGE = 1
    {
        PUTBUFFER
        PUTWRITEIDX
        GETREADIDX
    }

    GROUP : > CPU1TOCPU2RAM, PAGE = 1
    {
        GETBUFFER :    TYPE = DSECT
        GETWRITEIDX :  TYPE = DSECT
        PUTREADIDX :   TYPE = DSECT
    }

}
/*
//===========================================================================
// End of file.
//===========================================================================
*/
