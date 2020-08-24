//###########################################################################
// FILE:   f05_DownloadImage.cpp
// TITLE:  Download Image function required for serial flash programmer.
//
// This function is used to communicate and download with the device.  For 
// F05 devices, the serial flash programmer loads the kernel with a byte by
// byte echo back.  When communicating the application to the kernel, it
// works differently.  It sends chunks of the application and waits for 
// a checksum of that data.
//###########################################################################
// $TI Release: F28X7X Support Library$
// $Release Date: Octobe 23, 2014 $
//###########################################################################

#include "../include/f05_DownloadImage.h"

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
// Helpful macros for generating output depending upon verbose and quiet flags.
//
//*****************************************************************************
#define VERBOSEPRINT(...) if(g_bVerbose) { _tprintf(__VA_ARGS__); }
#define QUIETPRINT(...) if(!g_bQuiet) { _tprintf(__VA_ARGS__); }

//*****************************************************************************
//
// Globals whose values are set or overridden via command line parameters.
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

//COM Port handles
extern HANDLE file;
extern DCB port;

//*****************************************************************************
//
// Download an image to the the device identified by the passed handle.  The
// image to be downloaded and other parameters related to the operation are
// controlled by command line parameters via global variables.
//
// Returns 0 on success or a positive error return code on failure.
//
//*****************************************************************************
int
f05_DownloadImage(void)
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

	//
	// Does the input file exist?
	//
	// Opens the Flash Kernel File
	error = _wfopen_s(&Kfh, g_pszKernelFile, _T("rb"));

	if (!Kfh)
	{
		QUIETPRINT(_T("Unable to open Kernel file %s. Does it exist?\n"), g_pszKernelFile);
		return(10);
	}

    //Opens the application file 
	error = _wfopen_s(&Afh, g_pszAppFile, L"rb");

	if (!Afh)
	{
		QUIETPRINT(_T("Unable to open Application file %s. Does it exist?\n"), g_pszAppFile);
		return(10);
	}

	//Both Kernel, Application, and COM port are open
	
	//Do AutoBaud
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
	//Find the start of the kernel data
	getc(Kfh);
	getc(Kfh);
	getc(Kfh);

	fileStatus = fscanf_s(Kfh, "%x", &sendData[0]);
    int i = 0;
	while (fileStatus == 1)
	{
		i++;
		QUIETPRINT(_T("\n%lx"), sendData[0]);
		//Send next char
		WriteFile(file, &sendData[0], 1, &dwWritten, NULL);

		dwRead = 0;
		while (dwRead == 0)
		{
			ReadFile(file, &rcvData, 1, &dwRead, NULL);
		}
		QUIETPRINT(_T("==%lx"), rcvData);
		//Ensure data matches
		if (sendData[0] != rcvData){
			VERBOSEPRINT(_T("\nData does not match... Please press Ctrl-C to abort."));
			while (1){}
		}

		//Read next char
		fileStatus = fscanf_s(Kfh, "%x", &sendData[0]);
	}

	VERBOSEPRINT(_T("\nKernel Loaded"));

	Sleep(3000);
	
	VERBOSEPRINT(_T("\nDone Waiting for kernel boot...attempting autobaud"));
	PurgeComm(file, PURGE_RXCLEAR);


	//Do AutoBaud
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

	//Find the start of the application data
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
		//Send next char
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

	//Ensure checksum matches
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

	//Load the flash application
	while (1){

		fileStatus = fscanf_s(Afh, "%x ", &sendData[0]);
		if (fileStatus == 0)
			break;

		WriteFile(file, &sendData[0], 1, &dwWritten, NULL);

		checksum += sendData[0];

		// Get block size
		if (txCount == 0x00)
		{
			wordData = sendData[0];
		}
		else if (txCount == 0x01)
		{
			byteData = sendData[0];
			// form the wordData from the MSB:LSB
			wordData |= (byteData << 8);
		}

		txCount++;
		totalCount++;

		//If the next block size is 0, exit the while loop. 
		if (wordData == 0x00 && txCount > 1)
		{

			wordData = 0x0000;
			byteData = 0x0000;

			break;
		}
		// will execute when all the data in the block has been sent
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
			//Ensure checksum matches
			if (checksum != (rcvData | (rcvDataH << 8)))
				return(12);
			else
				checksum = 0;

			wordData = 0x0000;
			byteData = 0x0000;
			txCount = 0x00;
		}
		// will execute when the flash kernel buffer is full (0x400 words == 0x800 bytes)
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
			//Ensure checksum matches
			if (checksum != (rcvData | (rcvDataH << 8)))
				return(12);
			else
				checksum = 0;
		}
		
	}
	VERBOSEPRINT(_T("\nApplication Load Successful"));

    return 0;
}