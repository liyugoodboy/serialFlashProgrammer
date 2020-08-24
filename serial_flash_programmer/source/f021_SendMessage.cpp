//###########################################################################
//文件名称:f021_DownloadKernel.cpp
//文件说明:为f021设备下载内核功能。
//
//此功能用于与设备进行通信和下载。 对于F021器件，串行闪存编程器将相同的应用程序
//发送给应用程序它执行内核的方式。 在这两种情况下，串行闪存编程器发送一个字节，设
//备回显该相同字节。
//###########################################################################

#include "../include/f021_SendMessage.h"
#include "../include/f021_DownloadKernel.h"

#include "stdafx.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
using namespace std;

#pragma once
#include <conio.h>
#include <windows.h>
#include <dos.h>

//*****************************************************************************
//
// 用于输出详细信息
//
//*****************************************************************************
#define VERBOSEPRINT(...) if(g_bVerbose) { _tprintf(__VA_ARGS__); }
#define QUIETPRINT(...) if(!g_bQuiet) { _tprintf(__VA_ARGS__); }

//*****************************************************************************
//
// 通过命令行参数设置或覆盖其值的全局变量。
//
//*****************************************************************************
extern bool g_bVerbose;
extern bool g_bQuiet;
extern bool g_bOverwrite;
extern bool g_bUpload;
extern bool g_bClear;
extern bool g_bBinary;
extern bool g_bWaitOnExit;
extern bool g_bReset;
extern bool g_bSwitchMode;
extern bool g_bDualCore;
extern wchar_t *g_pszAppFile;
extern wchar_t *g_pszAppFile2;
extern wchar_t *g_pszKernelFile;
extern wchar_t *g_pszKernelFile2;
extern wchar_t *g_pszComPort;
extern wchar_t *g_pszBaudRate;
extern wchar_t *g_pszDeviceName;
/*****************************校验和*****************************************/
uint16_t checksum = 0;
/*****************************串口句柄及端口**********************************/
extern HANDLE file;
extern DCB port;
/*****************************应答与非应答标识码*******************************/
#define ACK						0x2D
#define NAK						0xA5

//*****************************************************************************
//
//函数声明
//
//*****************************************************************************
extern void clearBuffer(void);
extern void autobaudLock(void);
extern void loadProgram(FILE *fh);
uint32_t constructPacket(uint8_t* packet, uint16_t command, uint16_t length, uint8_t * data);
int f021_SendPacket(uint16_t message);
int receiveACK(void);
uint16_t getPacket(uint16_t* length, uint16_t* data);
uint16_t getWord(void);
void sendACK(void);
void sendNAK(void);
//*****************************************************************************
// 函数名称：constructPacket
// 函数说明：构造要发送到设备的数据包
// 数据包帧格式
// | Start | Length | Command | Data | Checksum | End | 
// |   2   |   2    |    2    |Length|    2     |  2  |
//
// 数据长度必须是2个字节的倍数。
// 输入参数：
//          uint8_t*  package  数据包存储位置指针
//          uint16_t  command  命令
//          uint16_t  length   数据长度(字长度)
//          uint8_t*  data     数据指针
// 输出参数：
//          uint32_t           数据包的长度
//
//*****************************************************************************
uint32_t constructPacket(uint8_t* packet, uint16_t command, uint16_t length, uint8_t * data)
{
	uint16_t checksum = 0;
	packet[0] = 0xE4; //帧头LSB字节
	packet[1] = 0x1B; //帧头MSB字节
	packet[2] = (uint8_t)(length & 0xFF); //长度信息LSB字节
	packet[3] = (uint8_t)((length & 0xFF00) >> 8); //长度信息MSB字节
	packet[4] = (uint8_t)(command & 0xFF); checksum += (command & 0xFF);//命令LSB字节
	packet[5] = (uint8_t)((command & 0xFF00) >> 8); checksum += ((command & 0xFF00) >> 8); //命令MSB字节
	uint32_t index = 6;
	for (int i = 0; i < length; i++) //装载数据
	{
		checksum += data[i];
		packet[index++] = data[i];
		i++;
		checksum += data[i];
		packet[index++] = data[i];
	}
	packet[index++] = (uint8_t)(checksum & 0xFF); //校验和LSB字节
	packet[index++] = (uint8_t)((checksum & 0xFF00) >> 8); //校验和MSB字节
	packet[index++] = 0x1B; //帧尾LSB字节
	packet[index++] = 0xE4; //帧尾MSB字节
	//返回数据包字节长度
	return(index);
}

//*****************************************************************************
//函数名称：f021_SendPacket
//函数说明：将功能包发送到设备中运行的内核。 此功能需要先对内核执行自动波特锁定。 
//         要下载的程序及其他与操作相关的参数由命令行参数控制通过全局变量。
//输入参数：
//         uint8_t*  package  数据包指针
//         uint32_t  length   数据包长度
//返回参数：
//          int  ACK时返回0  失败时返回-1。
//
//*****************************************************************************
int  f021_SendPacket(uint8_t* packet, uint32_t length)
{
	unsigned int rcvData = 0;
	unsigned int rcvDataH = 0;
	DWORD dwWritten;
	DWORD dwRead;

	//数据包采用LSB | MSB格式。
	for (int i = 0; i < length; i++)
	{
		WriteFile(file, &packet[i], 1, &dwWritten, NULL);
	} 
    //读取应答信号
	dwRead = 0;
	while (dwRead == 0)
	{
		ReadFile(file, &rcvData, 1, &dwRead, NULL);
	}
    //检查应答信号是否正确
	if (ACK != rcvData) 
	{
		VERBOSEPRINT(_T("\nNACK Error with sending the Function Packet... Please press Ctrl-C to abort."));
		return(-1);
	}

	VERBOSEPRINT(_T("\nFinished sending Packet... Received ACK to Packet... "));
	clearBuffer();
	return(0);
}

//*****************************************************************************
//函数名称：receiveACK
//函数说明：
//         此功能等待接收设备的ACK信号
//输入参数：
//返回参数：
//         int   ACK时返回0，失败时返回-1 
//*****************************************************************************
int receiveACK(void)
{
	unsigned int rcvData = 0;
	unsigned int rcvDataH = 0;
	DWORD dwWritten;
	DWORD dwRead;

	dwRead = 0;
	//接收应答信号
	while (dwRead == 0)
	{
		ReadFile(file, &rcvData, 1, &dwRead, NULL);
	}
    //应答信号检查
	if (ACK != rcvData) 
	{
		VERBOSEPRINT(_T("\nNACK Error with sending the Function Packet... Please press Ctrl-C to abort."));
		return(-1);
	}
	return(0);
}

//*****************************************************************************
//函数名称：getWord
//函数说明：接收16位的字数据，数据格式先接收低字节后高字节
//输入参数：
//返回参数：
//         uint16_t  返回接收到的数据
//*****************************************************************************
uint16_t getWord(void)
{
	uint16_t word;
	unsigned int rcvData = 0;
	unsigned int rcvDataH = 0;
	DWORD dwWritten;
	DWORD dwRead;
	dwRead = 0;
	//接收低字节
	while (dwRead == 0)
	{
		ReadFile(file, &rcvData, 1, &dwRead, NULL);
	}
	//发送应答信号
	sendACK();
	dwRead = 0;
	//接收高字节
	while (dwRead == 0)
	{
		ReadFile(file, &rcvDataH, 1, &dwRead, NULL);
	}
	//发送应答信号
	sendACK();
    //和校验
	checksum += rcvDataH + rcvData;
	word = ((rcvDataH << 8) | rcvData);
	return(word);
}

//*****************************************************************************
//函数名称：getPacket
//函数说明：从设备中接收一个数据包
//输入参数：
//         uint16_t length 接收到的数据长度指针
//         uint16_t data   接收到的数据指针
//返回参数：
//          uint16_t  命令码,数据错误则为错误码
//*****************************************************************************
uint16_t getPacket(uint16_t* length, uint16_t* data)
{
	int fail = 0;
	uint16_t word;
	//接收帧头
	word = getWord();
	//帧头校验
	if (word != 0x1BE4)
	{
		cout << "ERROR header " << hex << word << endl;
		fail++;
		//return(100);
	}
    //接收数据长度信息
	* length = getWord();
	//清除校验和
	checksum = 0;
	//接收命令
	uint16_t command = getWord();
    //接收数据
	for (int i = 0; i < (*length) / 2; i++)
	{
		*(data + i) = getWord();
	}
	//保存校验和
	uint16_t dataChecksum = checksum;
    //接收校验和并校验
	if (dataChecksum != getWord())
	{
		cout << "ERROR checksum" << endl;
		fail++;
		//return(101);
	}
	//接收帧尾
	word = getWord();
	if (word != 0xE41B)
	{
		cout << "ERROR footer " << word << endl;
		fail++;
		//return(102);
	}
	//帧错误则发送NACK
	if (fail)
	{
		sendNAK();
		command = fail;
	}
	else//帧正确则发送ACK
	{
		sendACK();
		cout << endl << "SUCCESS of Command" << endl;
	}
	return command;
}
//*****************************************************************************
//函数名称：sendACK
//函数说明：发送ACK信号
//输入参数：
//返回参数：
//*****************************************************************************
void sendACK(void)
{
	DWORD dwWritten;
	DWORD dwRead;
	unsigned char sendData[8];
	sendData[0] = ACK;

	WriteFile(file, &sendData[0], 1, &dwWritten, NULL);
}

//*****************************************************************************
//函数名称：sendNAK
//函数说明：发送NAK信号
//输入参数：
//返回参数：
//*****************************************************************************
void sendNAK(void)
{
	DWORD dwWritten;
	DWORD dwRead;
	unsigned char sendData[8];
	sendData[0] = NAK;

	WriteFile(file, &sendData[0], 1, &dwWritten, NULL);
}