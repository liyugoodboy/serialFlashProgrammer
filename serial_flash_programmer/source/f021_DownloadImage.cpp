//###########################################################################
// FILE:   f021_DownloadIimage.cpp
// TITLE:  Download Image function for f021 devices.
//
// This function is used to communicate and download with the device.  For 
// F021 devices, the serial flash programmer sends the application the same
// way it does the kernel.  In both instances, the serial flash programmer
// send one byte and the device echos back that same byte.
//###########################################################################
// $TI Release: F28X7X Support Library$
// $Release Date: Octobe 23, 2014 $
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
// External prototypes used by f021_DownloadImage()
// These functions are declared in f021_DownloadKernel.cpp
//
//*****************************************************************************
extern void clearBuffer(void);
extern void autobaudLock(void);
extern void loadProgram(FILE *fh);
extern int f021_SendFunctionMessage(uint8_t message);
int f021_DownloadImage(wchar_t* applicationFile);

//*****************************************************************************
//
// Download an image to the the device identified by the passed handle.  The
// image to be downloaded and other parameters related to the operation are
// controlled by command line parameters via global variables.
//
// Returns 1 on success for single core.
// Returns 2 on success for dual core.
// Returns -1 on failure.
//
//*****************************************************************************
#define checksum_enable 1
#define g_bBlockSize 0x80 //number of words transmitted until checksum
#include <assert.h>
void loadProgram_checksum(FILE *fh)
{
	unsigned char sendData[8];
	unsigned int fileStatus;
	unsigned int rcvData = 0;
	DWORD dwRead;
	unsigned int checksum = 0;
	char ack = 0x2D;
	assert(g_bBlockSize % 4 == 0); //because ECC uses multiple of 64 bits, or 4 words. % 4 == 0
	DWORD dwWritten;

	getc(fh);
	getc(fh);
	getc(fh);

	float bitRate = 0;
	DWORD millis = GetTickCount();
	//First 22 bytes are initialization data
	for (int i = 0; i < 22; i++)
	{
		fileStatus = fscanf_s(fh, "%x", &sendData[0]);
		//Send next char
		WriteFile(file, &sendData[0], 1, &dwWritten, NULL);
		bitRate += 8;
		checksum += sendData[0];
	}
	//Device will immediately ask for checksum
	//Receive LSB from checksum
	dwRead = 0;
	while (dwRead == 0)
	{
		ReadFile(file, &sendData[0], 1, &dwRead, NULL);
	}
	//Send ACK as expected
	WriteFile(file, &ack, 1, &dwWritten, NULL);

	//Receive MSB from checksum
	dwRead = 0;
	while (dwRead == 0)
	{
		ReadFile(file, &sendData[1], 1, &dwRead, NULL);
	}
	//Send ACK as expected
	WriteFile(file, &ack, 1, &dwWritten, NULL);

	rcvData = (sendData[1] << 8) + sendData[0];
	//Ensure checksum matches
	if (checksum != rcvData)
	{
		VERBOSEPRINT(_T("\nChecksum does not match... Please press Ctrl-C to abort."));
		while (1){}
	}

	while (fileStatus == 1)
	{
		unsigned int blockSize;
		//Read next block size (2 bytes) from hex2000 text file
		fileStatus = fscanf_s(fh, "%x", &sendData[0]); //LSB
		fileStatus = fscanf_s(fh, "%x", &sendData[1]); //MSB
		blockSize = (sendData[1] << 8) | sendData[0];

		//Send block size LSB
		WriteFile(file, &sendData[0], 1, &dwWritten, NULL);
		QUIETPRINT(_T("\n%lx"), sendData[0]);
		checksum += sendData[0];
		bitRate += 8;

		//Send block size MSB
		WriteFile(file, &sendData[1], 1, &dwWritten, NULL);
		QUIETPRINT(_T("\n%lx"), sendData[1]);
		checksum += sendData[1];
		bitRate += 8;

		if (blockSize == 0x0000) //end of file
		{
			break;
		}

		//Read next destination address from hex2000 text file (4 bytes, 32 bits)
		fileStatus = fscanf_s(fh, "%x", &sendData[0]); //MSW[23:16]
		fileStatus = fscanf_s(fh, "%x", &sendData[1]); //MSW[31:24]
		fileStatus = fscanf_s(fh, "%x", &sendData[2]); //LSW[7:0]
		fileStatus = fscanf_s(fh, "%x", &sendData[3]); //LSW[15:8]
		unsigned long destAddr = (sendData[1] << 24) | (sendData[0] << 16) |
			(sendData[3] << 8) | (sendData[2]);

		//Send destination address MSW[23:16]
		WriteFile(file, &sendData[0], 1, &dwWritten, NULL);
		QUIETPRINT(_T("\n%lx"), sendData[0]);
		checksum += sendData[0];
		bitRate += 8;

		//Send destination address MSW[31:24]
		WriteFile(file, &sendData[1], 1, &dwWritten, NULL);
		QUIETPRINT(_T("\n%lx"), sendData[1]);
		checksum += sendData[1];
		bitRate += 8;

		//Send block size LSW[7:0]
		WriteFile(file, &sendData[2], 1, &dwWritten, NULL);
		QUIETPRINT(_T("\n%lx"), sendData[2]);
		checksum += sendData[2];
		bitRate += 8;

		//Send block size LSW[15:8]
		WriteFile(file, &sendData[3], 1, &dwWritten, NULL);
		QUIETPRINT(_T("\n%lx"), sendData[3]);
		checksum += sendData[3];
		bitRate += 8;

		for (int j = 0; j < blockSize; j++)
		{
			if (((j % g_bBlockSize == 0) && (j > 0)) || ((blockSize < g_bBlockSize) && (j == blockSize)))
			{
				//receive checksum LSB
				dwRead = 0;
				while (dwRead == 0)
				{
					ReadFile(file, &sendData[0], 1, &dwRead, NULL);
				}
				//Send ACK as expected
				WriteFile(file, &ack, 1, &dwWritten, NULL);
				//receive checksum MSB
				dwRead = 0;
				while (dwRead == 0)
				{
					ReadFile(file, &sendData[1], 1, &dwRead, NULL);
				}
				//Send ACK as expected
				WriteFile(file, &ack, 1, &dwWritten, NULL);

				rcvData = sendData[0] | (sendData[1] << 8);
				//Ensure checksum matches
				if ((checksum & 0xFFFF) != rcvData)
				{
					VERBOSEPRINT(_T("\nChecksum does not match... Please press Ctrl-C to abort."));
					while (1){}
				}
			}

			//send LSB of word data
			fileStatus = fscanf_s(fh, "%x", &sendData[0]);
			WriteFile(file, &sendData[0], 1, &dwWritten, NULL);
			QUIETPRINT(_T("\n%lx"), sendData[0]);
			checksum += sendData[0];
			bitRate += 8;

			//send MSB of word data
			fileStatus = fscanf_s(fh, "%x", &sendData[0]);
			WriteFile(file, &sendData[0], 1, &dwWritten, NULL);
			QUIETPRINT(_T("\n%lx"), sendData[0]);
			checksum += sendData[0];
			bitRate += 8;
		}
		//receive checksum LSB
		dwRead = 0;
		while (dwRead == 0)
		{
			ReadFile(file, &sendData[0], 1, &dwRead, NULL);
		}
		//Send ACK as expected
		WriteFile(file, &ack, 1, &dwWritten, NULL);
		//receive checksum MSB
		dwRead = 0;
		while (dwRead == 0)
		{
			ReadFile(file, &sendData[1], 1, &dwRead, NULL);
		}
		//Send ACK as expected
		WriteFile(file, &ack, 1, &dwWritten, NULL);

		rcvData = sendData[0] | (sendData[1] << 8);
		//Ensure checksum matches
		if ((checksum & 0xFFFF) != rcvData)
		{
			VERBOSEPRINT(_T("\nChecksum does not match... Please press Ctrl-C to abort."));
			while (1){}
		}
	}
	millis = GetTickCount() - millis;
	bitRate = bitRate / millis * 1000;
	QUIETPRINT(_T("\nBit rate /s of transfer was: %f"), bitRate);
	rcvData = 0;
}
int
f021_DownloadImage(wchar_t* applicationFile)
{
	FILE *Afh;
	unsigned int rcvData = 0;
	unsigned int rcvDataH = 0;
	unsigned int txCount = 0;

	DWORD dwLen = 1;

	QUIETPRINT(_T("Downloading %s to device...\n"), applicationFile);

    //Opens the application file 
	Afh = _tfopen(applicationFile, L"rb");
	if (!Afh)
	{
		QUIETPRINT(_T("Unable to open Application file %s. Does it exist?\n"), applicationFile);
		return(-1);
	}

#if checksum_enable
	loadProgram_checksum(Afh);
#else
	loadProgram(Afh);
#endif

	VERBOSEPRINT(_T("\nApplication load successful!"));

	VERBOSEPRINT(_T("\nDone waiting for application to download and boot... "));
	clearBuffer();
	return(1);
}