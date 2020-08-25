//###########################################################################
//文件名称：f021_DownloadKernel.cpp
//文件说明：为f021设备下载内核功能。
//功能说明：
//         此功能用于与设备进行通信和下载。 对于F021器件，串行闪存编程器以与内核
//         相同的方式发送应用程序。在这两种情况下，串行闪存编程器都发送一个字节，
//         而设备回显该相同字节。
//###########################################################################

#include "../include/f021_DownloadKernel.h"

#include "stdafx.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#pragma once
#include <conio.h>
#include <windows.h>
#include <dos.h>
#include <time.h>

//*****************************************************************************
//
// 调试信息输出
//
//*****************************************************************************
#define VERBOSEPRINT(...) if(g_bVerbose) { _tprintf(__VA_ARGS__); }
#define QUIETPRINT(...) if(!g_bQuiet) { _tprintf(__VA_ARGS__); }

//*****************************************************************************
//
// 全局变量
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

/**********************************端口信息***********************************/
extern HANDLE file;
extern DCB port;

//*****************************************************************************
//
// 函数声明
//
//*****************************************************************************
void clearBuffer(void);
void autobaudLock(void);
void loadProgram(FILE *fh);
int f021_DownloadKernel(wchar_t* kernel);

//*****************************************************************************
//
// 清除串口缓存
//
//*****************************************************************************
void clearBuffer(void)
{
	PurgeComm(file, PURGE_RXCLEAR);
	unsigned char readBufferData[800];
	DWORD dwRead;
	COMSTAT ComStat;
	DWORD dwErrorFlags;
	ClearCommError(file, &dwErrorFlags, &ComStat);
	ReadFile(file, &readBufferData, ComStat.cbInQue, &dwRead, NULL);
}
//*****************************************************************************
//函数名称：autobandLock
//函数说明：自动波特率锁定
//输入参数：
//返回参数：
//*****************************************************************************
void autobaudLock(void)
{
	//清除串口缓存
	clearBuffer();
	unsigned char sendData[8];
	unsigned int rcvData = 0;
	DWORD dwWritten;
	DWORD dwRead;
	sendData[0] = 'A';
    //发送锁定字符
	WriteFile(file, &sendData[0], 1, &dwWritten, NULL);
	dwRead = 0;
	while (dwRead == 0){
		ReadFile(file, &rcvData, 1, &dwRead, NULL);
	}
    //锁定失败
	if (sendData[0] != rcvData)
	{
		QUIETPRINT(_T("\n%lx"), sendData[0]);
		QUIETPRINT(_T("==%lx"), rcvData);
		VERBOSEPRINT(_T("\nError with autobaud lock echoback... Please press Ctrl-C to abort."));
		while (1){}
	}
}
//*****************************************************************************
//函数名称：loadProgram
//函数说明：以SCI-8格式发送内核或应用程序数据。
//输入参数：
//         FILE *fh 内核或应用程序文件
//返回参数： 
//*****************************************************************************
void loadProgram(FILE *fh)
{
	unsigned char sendData[8];
	unsigned int fileStatus;
	unsigned int rcvData = 0;
	DWORD dwRead;
	DWORD dwWritten;

	getc(fh);
	getc(fh);
	getc(fh);

	fileStatus = fscanf_s(fh, "%x", &sendData[0]);

	float bitRate = 0;
	DWORD millis = GetTickCount();
	while (fileStatus == 1)
	{
		QUIETPRINT(_T("\n%lx"), sendData[0]);
		//发送字符
		WriteFile(file, &sendData[0], 1, &dwWritten, NULL);

		bitRate++;
		dwRead = 0;
		//接收回显字符
		while (dwRead == 0)
		{           
            ReadFile(file, &rcvData, 1, &dwRead, NULL);
        }
		QUIETPRINT(_T("==%lx"), rcvData);
		//数据比对
		if (sendData[0] != rcvData){
			VERBOSEPRINT(_T("\nData does not match... Please press Ctrl-C to abort."));
			while (1){}
		}

		//从文件中读取下一个字符
		fileStatus = fscanf_s(fh, "%x", &sendData[0]);
	}
	millis = GetTickCount() - millis;
	bitRate = bitRate / millis * 1000 * 8;
	QUIETPRINT(_T("\nBit rate /s of transfer was: %f"), bitRate);
	rcvData = 0;
}

//*****************************************************************************
//函数名称：f021_DownloadKernel
//函数说明：将内核下载到通过传递的句柄标识的设备上。 命令行参数通过全局变量控制要下载
//         的内核以及与该操作有关的其他参数。
//输入参数：
//返回参数：成功返回0，失败则返回10。
//*****************************************************************************
int f021_DownloadKernel(wchar_t * kernel)
{
	FILE *Kfh;

	unsigned int rcvData = 0;
	unsigned int rcvDataH = 0;
	unsigned int txCount = 0;

	DWORD dwLen = 1;

	QUIETPRINT(_T("Downloading %s to device...\n"), kernel);

	//打开内核文件
	Kfh = _tfopen(kernel, _T("rb"));

	if (!Kfh)
	{
		QUIETPRINT(_T("Unable to open Kernel file %s. Does it exist?\n"), kernel);
		return(10);
	}

	//自动波特率锁定
	VERBOSEPRINT(_T("\nAttempting autobaud to load kernel..."));
	autobaudLock();

    //下载核心文件
	VERBOSEPRINT(_T("\nAutobaud for kernel successful! Loading kernel file..."));
	loadProgram(Kfh);

	VERBOSEPRINT(_T("\nKernel loaded! Booting kernel..."));

	Sleep(2000);

	VERBOSEPRINT(_T("\nDone waiting for kernel boot... "));
	clearBuffer();
	return(0);
}
