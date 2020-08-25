//###########################################################################
//文件名称：f021_DownloadKernel.cpp
//文件说明：为f021设备下载内核功能。
//功能说明：
//         此功能用于与设备进行通信和下载。 对于F021器件，串行闪存编程器以与内核
//         相同的方式发送应用程序。在这两种情况下，串行闪存编程器都发送一个字节，
//         而设备回显该相同字节。
//###########################################################################
#ifndef __F021_DOWNLOADKERNEL__
#define __F021_DOWNLOADKERNEL__

void clearBuffer(void);
void autobaudLock(void);
void loadProgram(FILE *fh);
int f021_DownloadKernel(wchar_t* kernel);

#endif