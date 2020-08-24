//###########################################################################
// FILE:   f021_DownloadKernel.cpp
// TITLE:  Download Kernel function for f021 devices.
//
// This function is used to communicate and download with the device.  For 
// F021 devices, the serial flash programmer sends the application the same
// way it does the kernel.  In both instances, the serial flash programmer
// send one byte and the device echos back that same byte.
//###########################################################################
// $TI Release: F28X7X Support Library$
// $Release Date: Octobe 23, 2014 $
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

uint16_t checksum = 0;

//COM Port handle
extern HANDLE file;
extern DCB port;

#define ACK						0x2D
#define NAK						0xA5

//*****************************************************************************
//
// Function prototypes
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
//
// This function constructs the packet to be send to the device
// Packet Format:
// | Start | Length | Command | Data | Checksum | End | 
// |   2   |   2    |    2    |Length|    2     |  2  |
//
// Data length must be multiple of 2 bytes.
//
// Returns length of the packet.
//
//*****************************************************************************
uint32_t constructPacket(uint8_t* packet, uint16_t command, uint16_t length, uint8_t * data)
{
	uint16_t checksum = 0; //checksum of the Command and the Data
	packet[0] = 0xE4; //start LSB
	packet[1] = 0x1B; //start MSB
	packet[2] = (uint8_t)(length & 0xFF); //length LSB
	packet[3] = (uint8_t)((length & 0xFF00) >> 8); //length MSB
	packet[4] = (uint8_t)(command & 0xFF); checksum += (command & 0xFF);//command LSB
	packet[5] = (uint8_t)((command & 0xFF00) >> 8); checksum += ((command & 0xFF00) >> 8); //command MSB
	uint32_t index = 6;
	for (int i = 0; i < length; i++) //swap order of the data buffer
	{
		checksum += data[i];
		packet[index++] = data[i];
		i++;
		checksum += data[i];
		packet[index++] = data[i];
	}
	packet[index++] = (uint8_t)(checksum & 0xFF); //checksum LSB
	packet[index++] = (uint8_t)((checksum & 0xFF00) >> 8); //checksum MSB
	packet[index++] = 0x1B; //end LSB
	packet[index++] = 0xE4; //end MSB
	//index is now one larger than last index of array so equals the length.
	return(index);
}

//*****************************************************************************
//
// Send the function packet to the kernel runing in the device.  This fuction first
// performs an autobaud lock with the kernel.  The image to be downloaded and other 
// parameters related to the operation are controlled by command line parameters 
// via global variables.
//
// Returns 0 on ACK or -1 NAK return code on failure.
//
//*****************************************************************************
int
f021_SendPacket(uint8_t* packet, uint32_t length)
{
	unsigned int rcvData = 0;
	unsigned int rcvDataH = 0;
	DWORD dwWritten;
	DWORD dwRead;

	// packet is in the LSB|MSB format.  good to send
	for (int i = 0; i < length; i++)
	{
		WriteFile(file, &packet[i], 1, &dwWritten, NULL);
	} //finished sending packet

	dwRead = 0;
	while (dwRead == 0)
	{
		ReadFile(file, &rcvData, 1, &dwRead, NULL);
	}

	if (ACK != rcvData) //Check return byte
	{
		VERBOSEPRINT(_T("\nNACK Error with sending the Function Packet... Please press Ctrl-C to abort."));
		return(-1);
	}

	VERBOSEPRINT(_T("\nFinished sending Packet... Received ACK to Packet... "));
	clearBuffer();
	return(0);
}

//*****************************************************************************
//
// This function waits to receive and ACK or NAK from the device.
//
// Returns 0 on ACK or -1 NAK return code on failure.
//
//*****************************************************************************
int receiveACK(void)
{
	unsigned int rcvData = 0;
	unsigned int rcvDataH = 0;
	DWORD dwWritten;
	DWORD dwRead;

	dwRead = 0;
	while (dwRead == 0)
	{
		ReadFile(file, &rcvData, 1, &dwRead, NULL);
	}

	if (ACK != rcvData) //Check return byte
	{
		VERBOSEPRINT(_T("\nNACK Error with sending the Function Packet... Please press Ctrl-C to abort."));
		return(-1);
	}
	return(0);
}

//*****************************************************************************
//
// This function receives a 16 bit word. LSB then MSB.
//
// Returns the uint16_t word.
//
//*****************************************************************************
uint16_t getWord(void)
{
	uint16_t word;
	unsigned int rcvData = 0;
	unsigned int rcvDataH = 0;
	DWORD dwWritten;
	DWORD dwRead;

	dwRead = 0;
	while (dwRead == 0)
	{
		ReadFile(file, &rcvData, 1, &dwRead, NULL);
	}
	sendACK();
	dwRead = 0;
	while (dwRead == 0)
	{
		ReadFile(file, &rcvDataH, 1, &dwRead, NULL);
	}
	sendACK();

	checksum += rcvDataH + rcvData;
	word = ((rcvDataH << 8) | rcvData);

	return(word);
}

//*****************************************************************************
//
// This function receives the Packet from the device.
//
// Returns the uint16_t command.
//
//*****************************************************************************
uint16_t getPacket(uint16_t* length, uint16_t* data)
{
	int fail = 0;
	uint16_t word;
	word = getWord();
	if (word != 0x1BE4)
	{
		cout << "ERROR header " << hex << word << endl;
		fail++;
		//return(100);
	}

	* length = getWord();
	checksum = 0;
	uint16_t command = getWord();

	for (int i = 0; i < (*length) / 2; i++)
	{
		*(data + i) = getWord();
	}
	uint16_t dataChecksum = checksum;

	if (dataChecksum != getWord())
	{
		cout << "ERROR checksum" << endl;
		fail++;
		//return(101);
	}
	word = getWord();
	if (word != 0xE41B)
	{
		cout << "ERROR footer " << word << endl;
		fail++;
		//return(102);
	}
	if (fail)
	{
		sendNAK();
		command = fail;
	}
	else
	{
		sendACK();
		cout << endl << "SUCCESS of Command" << endl;
	}
	return command;
}

//*****************************************************************************
//
// This function sends an ACK.
//
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
//
// This function sends an NAK.
//
//*****************************************************************************
void sendNAK(void)
{
	DWORD dwWritten;
	DWORD dwRead;
	unsigned char sendData[8];
	sendData[0] = NAK;

	WriteFile(file, &sendData[0], 1, &dwWritten, NULL);
}