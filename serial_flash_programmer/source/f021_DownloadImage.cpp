//###########################################################################
//文件名称：f021_DownloadIimage.cpp
//文件说明：为f021设备下载程序功能。
//         此功能用于与设备进行通信和下载。 对于F021器件，串行闪存编程器将相同的
//         应用程序发送给应用程序它执行内核的方式。 在这两种情况下，串行闪存编程器
//         发送一个字节，设备回显该相同字节。
//###########################################################################
#include "../include/f021_DownloadImage.h"
#include "../include/f021_DownloadKernel.h"
#include "../include/f021_SendMessage.h"

#include "stdafx.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#pragma once
#include <conio.h>
#include <windows.h>
#include <dos.h>

//*****************************************************************************
//
// 调试信息输出
//
//*****************************************************************************
#define VERBOSEPRINT(...) if(g_bVerbose) { _tprintf(__VA_ARGS__); }
#define QUIETPRINT(...) if(!g_bQuiet) { _tprintf(__VA_ARGS__); }

//*****************************************************************************
//
// 外部全局变量
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

/**************************全局变量中端口句柄以及端口号************************/
extern HANDLE file;
extern DCB port;
//*****************************************************************************
//外部函数声明
//*****************************************************************************
extern void clearBuffer(void);
extern void autobaudLock(void);
extern void loadProgram(FILE *fh);
/*********************************函数声明************************************/
extern int f021_SendFunctionMessage(uint8_t message);
int f021_DownloadImage(wchar_t* applicationFile);

//*****************************************************************************
//函数名称：loadProgram_checksum
//函数说明：将程序下载到通过传递的句柄标识的设备上，要下载的程序以及与该操作有关的
//         其他参数通过全局变量由命令行参数控制。
//传入参数：FILE  *fh    程序文件
//返回参数：
//         如果单核成功，则返回1。
//         双核成功返回2。
//         失败时返回-1。
//
//*****************************************************************************
#define checksum_enable 1
#define g_bBlockSize 0x80 //直到校验和为止传输的字数
#include <assert.h>
void loadProgram_checksum(FILE *fh)
{
	unsigned char sendData[8];
	unsigned int fileStatus;
	unsigned int rcvData = 0;
	DWORD dwRead;
	unsigned int checksum = 0;
	char ack = 0x2D;
	assert(g_bBlockSize % 4 == 0); //因为ECC使用64位或4个字的倍数。％4 == 0
	DWORD dwWritten;
    //移除文件中无关信息
	getc(fh);
	getc(fh);
	getc(fh);

	float bitRate = 0;
	DWORD millis = GetTickCount();
	//前22个字节为初始化数据
	for (int i = 0; i < 22; i++)
	{
		fileStatus = fscanf_s(fh, "%x", &sendData[0]);
		//发送字符
		WriteFile(file, &sendData[0], 1, &dwWritten, NULL);
		bitRate += 8;
		checksum += sendData[0];
	}
	//接收设备校验和的低字节
	dwRead = 0;
	while (dwRead == 0)
	{
		ReadFile(file, &sendData[0], 1, &dwRead, NULL);
	}
	//发送ACK信号
	WriteFile(file, &ack, 1, &dwWritten, NULL);

	//接收设备校验和的高字节
	dwRead = 0;
	while (dwRead == 0)
	{
		ReadFile(file, &sendData[1], 1, &dwRead, NULL);
	}
	//发送ACK信号
	WriteFile(file, &ack, 1, &dwWritten, NULL);
 
	rcvData = (sendData[1] << 8) + sendData[0];
	//校验
	if (checksum != rcvData)
	{
		VERBOSEPRINT(_T("\nChecksum does not match... Please press Ctrl-C to abort."));
		while (1){}
	}

	while (fileStatus == 1)
	{
		unsigned int blockSize;
		//读取下一个数据块大小（字）从hex2000转换出的文件
		fileStatus = fscanf_s(fh, "%x", &sendData[0]); //LSB
		fileStatus = fscanf_s(fh, "%x", &sendData[1]); //MSB
		blockSize = (sendData[1] << 8) | sendData[0];

		//发送块大小LSB
		WriteFile(file, &sendData[0], 1, &dwWritten, NULL);
		QUIETPRINT(_T("\n%lx"), sendData[0]);
		checksum += sendData[0];
		bitRate += 8;

		//发送块大小MSB
		WriteFile(file, &sendData[1], 1, &dwWritten, NULL);
		QUIETPRINT(_T("\n%lx"), sendData[1]);
		checksum += sendData[1];
		bitRate += 8;
        //如果块大小为0,则文件结束
		if (blockSize == 0x0000)
		{
			break;
		}

		//从hex2000文本文件中读取下一个目标地址（4个字节，32位）
		fileStatus = fscanf_s(fh, "%x", &sendData[0]); //MSW[23:16]
		fileStatus = fscanf_s(fh, "%x", &sendData[1]); //MSW[31:24]
		fileStatus = fscanf_s(fh, "%x", &sendData[2]); //LSW[7:0]
		fileStatus = fscanf_s(fh, "%x", &sendData[3]); //LSW[15:8]
		unsigned long destAddr = (sendData[1] << 24) | (sendData[0] << 16) |
			(sendData[3] << 8) | (sendData[2]);

		//发送目标地址MSW[23:16]
		WriteFile(file, &sendData[0], 1, &dwWritten, NULL);
		QUIETPRINT(_T("\n%lx"), sendData[0]);
		checksum += sendData[0];
		bitRate += 8;

		//发送目标地址MSW[31:24]
		WriteFile(file, &sendData[1], 1, &dwWritten, NULL);
		QUIETPRINT(_T("\n%lx"), sendData[1]);
		checksum += sendData[1];
		bitRate += 8;

		//发送目标地址LSW[7:0]
		WriteFile(file, &sendData[2], 1, &dwWritten, NULL);
		QUIETPRINT(_T("\n%lx"), sendData[2]);
		checksum += sendData[2];
		bitRate += 8;

		//发送目标地址LSW[15:8]
		WriteFile(file, &sendData[3], 1, &dwWritten, NULL);
		QUIETPRINT(_T("\n%lx"), sendData[3]);
		checksum += sendData[3];
		bitRate += 8;
        //发送块数据
		for (int j = 0; j < blockSize; j++)
		{
			if (((j % g_bBlockSize == 0) && (j > 0)) || ((blockSize < g_bBlockSize) && (j == blockSize)))
			{
				//接收校验和LSB
				dwRead = 0;
				while (dwRead == 0)
				{
					ReadFile(file, &sendData[0], 1, &dwRead, NULL);
				}
				//发送ACK信号
				WriteFile(file, &ack, 1, &dwWritten, NULL);
				//接收校验和MSB
				dwRead = 0;
				while (dwRead == 0)
				{
					ReadFile(file, &sendData[1], 1, &dwRead, NULL);
				}
				//发送ACK信号
				WriteFile(file, &ack, 1, &dwWritten, NULL);

				rcvData = sendData[0] | (sendData[1] << 8);
				//校验
				if ((checksum & 0xFFFF) != rcvData)
				{
					VERBOSEPRINT(_T("\nChecksum does not match... Please press Ctrl-C to abort."));
					while (1){}
				}
			}

			//发送字数据LSB
			fileStatus = fscanf_s(fh, "%x", &sendData[0]);
			WriteFile(file, &sendData[0], 1, &dwWritten, NULL);
			QUIETPRINT(_T("\n%lx"), sendData[0]);
			checksum += sendData[0];
			bitRate += 8;

			//发送字数据MSB
			fileStatus = fscanf_s(fh, "%x", &sendData[0]);
			WriteFile(file, &sendData[0], 1, &dwWritten, NULL);
			QUIETPRINT(_T("\n%lx"), sendData[0]);
			checksum += sendData[0];
			bitRate += 8;
		}
		//接收校验和LSB
		dwRead = 0;
		while (dwRead == 0)
		{
			ReadFile(file, &sendData[0], 1, &dwRead, NULL);
		}
		//发送ACK信号
		WriteFile(file, &ack, 1, &dwWritten, NULL);
		//接收校验和MSB
		dwRead = 0;
		while (dwRead == 0)
		{
			ReadFile(file, &sendData[1], 1, &dwRead, NULL);
		}
		//发送ACK信号
		WriteFile(file, &ack, 1, &dwWritten, NULL);

		rcvData = sendData[0] | (sendData[1] << 8);
		//校验
		if ((checksum & 0xFFFF) != rcvData)
		{
			VERBOSEPRINT(_T("\nChecksum does not match... Please press Ctrl-C to abort."));
			while (1){}
		}
	}
	//计算发送的数据速度
	millis = GetTickCount() - millis;
	bitRate = bitRate / millis * 1000;
	QUIETPRINT(_T("\nBit rate /s of transfer was: %f"), bitRate);
	rcvData = 0;
}
/******************************************************************************
 * 函数名称：f021_DownloadImage
 * 函数说明：下载程序文件
 * 输入参数：
 *           wchar_t* applicationFile 应用程序文件路径
 * 返回参数：
 *           1成功    -1失败
 * ***************************************************************************/
int f021_DownloadImage(wchar_t* applicationFile)
{
	FILE *Afh;
	unsigned int rcvData = 0;
	unsigned int rcvDataH = 0;
	unsigned int txCount = 0;

	DWORD dwLen = 1;
    //显示应用程序名称并提示开始下载
	QUIETPRINT(_T("Downloading %s to device...\n"), applicationFile);

    //打开应用程序文件
	Afh = _tfopen(applicationFile, L"rb");
	//文件打开失败
	if (!Afh)
	{
		QUIETPRINT(_T("Unable to open Application file %s. Does it exist?\n"), applicationFile);
		return(-1);
	}
    //下载程序文件
#if checksum_enable
	loadProgram_checksum(Afh);
#else
	loadProgram(Afh);
#endif
    //输出信息
	VERBOSEPRINT(_T("\nApplication load successful!"));
	VERBOSEPRINT(_T("\nDone waiting for application to download and boot... "));
	//清除串口缓存
	clearBuffer();
	return(1);
}