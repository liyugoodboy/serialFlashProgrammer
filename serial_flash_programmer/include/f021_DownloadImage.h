//###########################################################################
//文件名称：f021_DownloadIimage.cpp
//文件说明：为f021设备下载程序功能。
//         此功能用于与设备进行通信和下载。 对于F021器件，串行闪存编程器将相同的
//         应用程序发送给应用程序它执行内核的方式。 在这两种情况下，串行闪存编程器
//         发送一个字节，设备回显该相同字节。
//###########################################################################

#ifndef __F021_DOWNLOADIMAGE__
#define __F021_DOWNLOADIMAGE__

extern void clearBuffer(void);
extern void autobaudLock(void);
extern void loadProgram(FILE *fh);
extern int f021_SendFunctionPacket(uint8_t message);
int f021_DownloadImage(wchar_t* applicationFile);

#endif