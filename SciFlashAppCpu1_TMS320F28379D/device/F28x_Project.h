/*****************************************************
 * 文件名称:F28x_Project.h
 * 文件说明:全局头文件
 * 功能说明:
 * 完成时间:
 *    版本:
 * 修改记录:
 *****************************************************/
#ifndef F28X_PROJECT_H
#define F28X_PROJECT_H
//************************头文件************************
//----------------------基础头文件-----------------------
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
//--------------------F2837xD头文件---------------------
#include "F2837xD_Examples.h"
#include "F2837xD_Ipc_drivers.h"
#include "F2837xD_device.h"
#include "device.h"
//--------------------Flash API------------------------
#include "F021_F2837xD_C28x.h"

/******************************************************
函数名称:USER_getLibVersion
函数描述：获取使用的driverlib库版本
输入参数:
返回值:
******************************************************/
static inline
Uint32 USER_getLibVersion()
{
   return(Version_getLibVersion());
}
/******************************************************
函数名称:USER_getWareVersion
函数描述：获取使用的C2000ware版本
输入参数:
返回值:
******************************************************/
static inline
Uint32 USER_getWareVersion()
{
   return(2000003U);
}
/******************************************************
函数名称:USER_getComplierVersion
函数描述：获取使用的编译器版本
输入参数:
返回值:
******************************************************/
static inline
Uint32 USER_getComplierVersion()
{
   return(180104U);
}
#endif  // end of F28X_PROJECT_H definition
//
// End of file
//
