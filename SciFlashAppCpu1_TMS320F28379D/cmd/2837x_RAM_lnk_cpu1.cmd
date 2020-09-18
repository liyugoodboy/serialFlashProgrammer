/*******************************存储空间映射**************************************/
MEMORY
{
/*程序存储空间*/
PAGE 0 :
   /*bootloader模式为引导至SARAM中C程序入口*/
   BEGIN           	: origin = 0x000000, length = 0x000002
   /*CPU1 CPU2*/
   RAMGS0           : origin = 0x00C000, length = 0x001000
   RAMGS1           : origin = 0x00D000, length = 0x001000
   RAMGS2           : origin = 0x00E000, length = 0x001000
   RAMGS3           : origin = 0x00F000, length = 0x001000
   RAMGS4           : origin = 0x010000, length = 0x001000
   RAMGS5           : origin = 0x011000, length = 0x001000
   RAMGS6           : origin = 0x012000, length = 0x001000
   RAMGS7           : origin = 0x013000, length = 0x001000
   RAMGS8           : origin = 0x014000, length = 0x001000
   RAMGS9           : origin = 0x015000, length = 0x001000
   RAMGS10          : origin = 0x016000, length = 0x001000
   RAMGS11          : origin = 0x017000, length = 0x001000
   RAMGS12          : origin = 0x018000, length = 0x001000
   RAMGS13          : origin = 0x019000, length = 0x001000
   RAMGS14          : origin = 0x01A000, length = 0x001000
   RAMGS15          : origin = 0x01B000, length = 0x001000
   RESET           	: origin = 0x3FFFC0, length = 0x000002
/*数据存储空间*/
PAGE 1 :
   /*M0的一部分,BOOT ROM会将其用于堆栈*/
   BOOT_RSVD       : origin = 0x000002, length = 0x000120
   /*CPU1*/
   RAMM0           : origin = 0x000122, length = 0x0002DE
   RAMM1           : origin = 0x000400, length = 0x000400
   RAMD0           : origin = 0x00B000, length = 0x000800
   RAMD1           : origin = 0x00B800, length = 0x000800
   /*CPU1 CLA1*/
   RAMLS0          	: origin = 0x008000, length = 0x000800
   RAMLS1          	: origin = 0x008800, length = 0x000800
   RAMLS2      		: origin = 0x009000, length = 0x000800
   RAMLS3      		: origin = 0x009800, length = 0x000800
   RAMLS4      		: origin = 0x00A000, length = 0x000800
   RAMLS5           : origin = 0x00A800, length = 0x000800
   /*CPU1 CLA1 MSG*/
   CLA1_MSGRAMLOW   : origin = 0x001480, length = 0x000080
   CLA1_MSGRAMHIGH  : origin = 0x001500, length = 0x000080
   /*CPU1 CPU2 MSG*/
   CPU2TOCPU1RAM    : origin = 0x03F800, length = 0x000400
   CPU1TOCPU2RAM    : origin = 0x03FC00, length = 0x000400
}

/*******************************段存储分配****************************************/
SECTIONS
{
   /*代码入口*/
   codestart        : > BEGIN,     PAGE = 0,  ALIGN(4)
   
#ifdef __TI_COMPILER_VERSION__
   #if __TI_COMPILER_VERSION__ >= 15009000
   .TI.ramfunc   : {} > RAMGS9,     PAGE = 0,  ALIGN(4)
   #else
   ramfuncs         : > RAMGS9,     PAGE = 0,  ALIGN(4)
   #endif
#endif   
   /*代码段*/
   .text            : >>RAMGS0 | RAMGS1 |  RAMGS2 | RAMGS3,  PAGE = 0,  ALIGN(4)
   /*初始化的全局变量和静态变量表*/
   .cinit           : > RAMGS10,     PAGE = 0,  ALIGN(4)
   /*全局对象构造函数表*/
   .pinit           : > RAMGS10,     PAGE = 0,  ALIGN(4)
   /*switch表格*/
   .switch          : > RAMGS10,     PAGE = 0,  ALIGN(4)
   /*复位向量*/
   .reset           : > RESET,     PAGE = 0, TYPE = DSECT /* not used, */
   /*堆栈*/
   .stack           : > RAMD0,     PAGE = 1,  ALIGN(4)
   /*全局变量和静态变量*/
   .ebss            : > RAMM1,     PAGE = 1,  ALIGN(4)
   /*初始化的全局变量和静态变量*/
   .data            : > RAMM1,     PAGE = 1,  ALIGN(4)
   /*未初始化全局变量和静态变量*/
   .bss             : > RAMM1,     PAGE = 1,  ALIGN(4)
   /*全局常量*/
   .econst          : > RAMD1,     PAGE = 1,  ALIGN(4)
   /*全局常量*/
   .const           : > RAMD1,     PAGE = 1,  ALIGN(4)
   /*堆数据*/
   .esysmem         : > RAMLS5,    PAGE = 1,  ALIGN(4)
   /*CI/O数据流*/
   .cio             : > RAMM0,     PAGE = 1,  ALIGN(4)
   
   /* 使用IPC API驱动程序时，需要以下部分定义 */
    GROUP : > CPU1TOCPU2RAM, PAGE = 1 
    {
        PUTBUFFER 
        PUTWRITEIDX 
        GETREADIDX 
    }
    
    GROUP : > CPU2TOCPU1RAM, PAGE = 1
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
