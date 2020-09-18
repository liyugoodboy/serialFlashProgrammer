//###########################################################################
//文件名称：f05_DownloadImage.cpp
//文件说明：下载串行闪存编程器所需的应用功能。
//         此功能用于与设备进行通信和下载。对于F05器件，串行闪存编程器将字节装入内
//         核字节回显。在将应用程序与内核通信时，它的工作方式有所不同。 它发送应用
//         程序的块并等待该数据的校验和。
//###########################################################################

#include "../include/f05_DownloadImage.h"


#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h> 
#include <stdbool.h>

#pragma once
#include <conio.h>
#include <windows.h>
#include <dos.h>

//*****************************************************************************
//
//帮助信息输出
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

/*********************************端口信息*************************************/
extern HANDLE file;
extern DCB port;
//*****************************************************************************
//函数名称：f05_DownloadImage
//函数说明：将图像下载到通过传递的句柄标识的设备。 命令行参数通过全局变量控制要下载的
//          图像和与操作有关的其他参数。
//         注：此函数流程：内核下载-->应用程序下载
//输入参数：
//返回参数：成功返回0，失败则返回正错误。
//*****************************************************************************
int f05_DownloadImage(void)
{
	FILE *Kfh;
	FILE *Afh;
	
	unsigned int rcvData = 0;
	unsigned int rcvDataH = 0;
	int txCount = 0;

	uint16_t checksum;
	unsigned int fileStatus;
	DWORD dwRead;
	errno_t error;
	DWORD dwWritten;
	unsigned char sendData[8];
	
	QUIETPRINT(_T("Downloading %s to device...\n"), g_pszAppFile);

	//打开核心文件
	error = _wfopen_s(&Kfh, g_pszKernelFile, _T("rb"));

	if (!Kfh)
	{
		QUIETPRINT(_T("Unable to open Kernel file %s. Does it exist?\n"), g_pszKernelFile);
		return(10);
	}

    //打开应用程序文件
	error = _wfopen_s(&Afh, g_pszAppFile, L"rb");

	if (!Afh)
	{
		QUIETPRINT(_T("Unable to open Application file %s. Does it exist?\n"), g_pszAppFile);
		return(10);
	}
	
	//自动波特率锁定
 	dwRead = 0;
	sendData[0] = 'A';

	WriteFile(file, &sendData[0], 1, &dwWritten, NULL);
	while (dwRead == 0)
	{
		ReadFile(file, &rcvData, 1, &dwRead, NULL);
	}

	if (sendData[0] != rcvData)
		return(12);


	VERBOSEPRINT(_T("\nKernel AutoBaud Successful"));
	
	//下载核心文件
	getc(Kfh);
	getc(Kfh);
	getc(Kfh);

	fileStatus = fscanf_s(Kfh, "%x", &sendData[0]);
    int i = 0;
	while (fileStatus == 1)
	{
		i++;
		QUIETPRINT(_T("\n%lx"), sendData[0]);
		//发送字符
		WriteFile(file, &sendData[0], 1, &dwWritten, NULL);

		dwRead = 0;
		//读取回显字符
		while (dwRead == 0)
		{
			ReadFile(file, &rcvData, 1, &dwRead, NULL);
		}
		QUIETPRINT(_T("==%lx"), rcvData);
		//字符对比校验
		if (sendData[0] != rcvData){
			VERBOSEPRINT(_T("\nData does not match... Please press Ctrl-C to abort."));
			while (1){}
		}

		//从文件中读取下一个字符
		fileStatus = fscanf_s(Kfh, "%x", &sendData[0]);
	}

	VERBOSEPRINT(_T("\nKernel Loaded"));

	Sleep(3000);
	
	VERBOSEPRINT(_T("\nDone Waiting for kernel boot...attempting autobaud"));
	PurgeComm(file, PURGE_RXCLEAR);


	//自动波特率锁定
	sendData[0] = 'A';

    WriteFile(file, &sendData[0], 1, &dwWritten, NULL);

	dwRead = 0;
	while (dwRead == 0)
	{
		ReadFile(file, &rcvData, 1, &dwRead, NULL);
	}

	if (sendData[0] != rcvData)
	{
		VERBOSEPRINT(_T("\nApplication AutoBaud Fail"));
		return(12);
	}
		

	VERBOSEPRINT(_T("\nApplication AutoBaud Successful"));

	Sleep(3000);

	//发送应用程序文件
	txCount = 0;
	checksum = 0;

	getc(Afh);
	getc(Afh);
	getc(Afh);

	while (txCount < 22)
	{
		txCount++;
		fscanf_s(Afh, "%x", &sendData[0]);
		checksum += sendData[0];
		//发送字符
        WriteFile(file, &sendData[0], 1, &dwWritten, NULL);

	}
	dwRead = 0;
	while (dwRead == 0)
	{
		ReadFile(file, &rcvData, 1, &dwRead, NULL);
	}	
	dwRead = 0;
	while (dwRead == 0)
	{
		ReadFile(file, &rcvDataH, 1, &dwRead, NULL);
	}

	//和校验
	if (checksum != (rcvData | (rcvDataH << 8)))
		return(12);

	int wordData;
	int byteData;
	txCount = 0;
	checksum = 0;

	int totalCount = 0;
	wordData = 0x0000;
	byteData = 0x0000;
	fileStatus = 1;

	//下载应用程序文件
	while (1){

		fileStatus = fscanf_s(Afh, "%x ", &sendData[0]);
		if (fileStatus == 0)
			break;

		WriteFile(file, &sendData[0], 1, &dwWritten, NULL);

		checksum += sendData[0];

		//获取块大小信息
		if (txCount == 0x00)
		{
			wordData = sendData[0];
		}
		else if (txCount == 0x01)
		{
			byteData = sendData[0];
			//MSB:LSB
			wordData |= (byteData << 8);
		}

		txCount++;
		totalCount++;

		//如果下一个块大小为0，则退出while循环。
		if (wordData == 0x00 && txCount > 1)
		{

			wordData = 0x0000;
			byteData = 0x0000;

			break;
		}
		//发送块中的所有数据后将执行
		else if (txCount == 2 * (wordData + 3))
		{
			dwRead = 0;
			while (dwRead == 0)
			{
				ReadFile(file, &rcvData, 1, &dwRead, NULL);
			}
			dwRead = 0;
			while (dwRead == 0)
			{				
				ReadFile(file, &rcvDataH, 1, &dwRead, NULL);
			}
			//校验和
			if (checksum != (rcvData | (rcvDataH << 8)))
				return(12);
			else
				checksum = 0;

			wordData = 0x0000;
			byteData = 0x0000;
			txCount = 0x00;
		}
		//将在Flash内核缓冲区已满（0x400字== 0x800字节）时执行
		else if ((txCount - 6) % 0x800 == 0 && txCount > 6)
		{
			dwRead = 0;
			while (dwRead == 0)
			{
				ReadFile(file, &rcvData, 1, &dwRead, NULL);
			}
			dwRead = 0;
			while (dwRead == 0)
			{
				ReadFile(file, &rcvDataH, 1, &dwRead, NULL);
			}
			//校验和
			if (checksum != (rcvData | (rcvDataH << 8)))
				return(12);
			else
				checksum = 0;
		}		
	}
	//显示下载成功提示
	VERBOSEPRINT(_T("\nApplication Load Successful"));
    return 0;
}