//###########################################################################
//文件名称：f021_DownloadIimage.h
//文件说明：为f021设备下载数据功能。
//          此功能用于与设备进行通信和下载。 对于F021器件，串行闪存编程器以与内核
//          相同的方式发送应用程序。 在这两种情况下，串行闪存编程器都发送一个字节，
//          而设备回显该相同字节。
//###########################################################################

#ifndef __F021_SENDMESSAGE__
#define __F021_SENDMESSAGE__

extern void clearBuffer(void);
extern void autobaudLock(void);
extern void loadProgram(FILE *fh);
uint32_t constructPacket(uint8_t* packet, uint16_t command, uint16_t length, uint8_t * data);
int f021_SendPacket(uint8_t* packet, uint32_t length);

#endif