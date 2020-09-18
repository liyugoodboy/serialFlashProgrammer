//###########################################################################
// FILE:   serial_flash_programmer.cpp
// TITLE:  Serial Flash Programmer for firmware upgrades through SCI (host).
//
// This is demonstration code for use with the Texas Instruments Serial
// Flash Programmer. It reads data in the boot loader format from an input file,
// then sends that data to the microcontroller using a COM port. 
//###########################################################################
// $TI Release: $
// $Release Date: $
//###########################################################################

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <iostream>
#include <iomanip>
using namespace std;

#include "../include/f05_DownloadImage.h"
#include "../include/f021_DownloadImage.h"
#include "../include/f021_DownloadKernel.h"
#include "../include/f021_SendMessage.h"
#include "../include/serialFlash.h"

#include <qdebug.h>

//解决中文乱码
#if _MSC_VER >= 1600	
#pragma execution_character_set("utf-8")
#endif


/**********************************函数声明************************************/
void ExitApp(int iRetcode);
void PrintWelcome(void);
void ShowHelp(void);
int ParseCommandLine(int argc, wchar_t *argv[]);
void setDeviceName(void);
void setEraseSector(unsigned int CPU, uint32_t Sector);
void checkErrors(void);
void getStatus(void);
void printErrorStatus(uint16_t status);
uint32_t formatMemAddr(uint16_t firstHalf, uint16_t secondHalf);
/********************************外部函数声明**********************************/
extern void autobaudLock(void);
extern int f021_DownloadImage(void);
extern int f05_DownloadImage(void);
extern int f021_DownloadKernel(wchar_t* kernel);
extern uint32_t constructPacket(uint8_t* packet, uint16_t command, uint16_t length, uint8_t * data);
extern int f021_SendPacket(uint8_t* packet, uint32_t length);
extern int receiveACK(void);
extern uint16_t getPacket(uint16_t* length, uint16_t* data);
extern uint16_t getWord(void);

#pragma once
#include <conio.h>
#include <windows.h>
#include <dos.h>

//*****************************************************************************
//
// 调试信息
//
//*****************************************************************************
#define  VERBOSEPRINT(...)   if(g_bVerbose) { _tprintf(__VA_ARGS__); }
#define  QUIETPRINT(...)     if(!g_bQuiet)  { _tprintf(__VA_ARGS__); }

#define  QTMESSAGE(x)        printfMessage(x)  //消息传递到QT文本控件中,只能在类内使用
//*****************************************************************************
//
// 全局变量
//
//*****************************************************************************
bool g_bVerbose = false;
bool g_bQuiet = false;
bool g_bOverwrite = false;
bool g_bUpload = false;
bool g_bClear = false;
bool g_bBinary = false;
bool g_bWaitOnExit = false;
bool g_bReset = false;
bool g_bSwitchMode = false;
bool g_baudLock = false;         //波特率锁定标志
bool cpu1 = false;              //CPU1核心运行标志
bool cpu2 = false;
/**************************************设备号*********************************/
bool g_bf2802x = false;
bool g_bf2803x = false;
bool g_bf2805x = false;
bool g_bf2806x = false;
bool g_bf2833x = false;
bool g_bf2837xD = false;
bool g_bf2837xS = false;
bool g_bf2807x = false;
bool g_bf28004x = false;

/***********************************FLASH类型*********************************/
bool g_bf021 = false;  //内核B，逐字节校验和，使用状态包。
bool g_bf05 = false;  //内核A，逐字节校验和，无状态包。

/*****************************设备固件更新功能类型*****************************/
bool g_bDFU = false;
bool g_bDFU1 = false;
bool g_bDFU2 = false;
bool g_bDualCore = false;
bool g_bDFU_branch = false;
/********************************扇区擦除功能**********************************/
bool g_bErase1 = false;
bool g_bErase2 = false;
uint32_t gu32_SectorMask = 0xFFFFFFFF;
uint32_t gu32_EraseSectors1 = 0U; //CPU1将发送两次。 每一位代表一个扇区
uint32_t gu32_EraseSectors2 = 0U; //CPU2将发送两次。 每一位代表一个扇区

/*******************************命令行功能**************************************/
uint32_t gu32_Function = 0U;
uint16_t gu16_Command = 0U;

/********************************解锁功能***************************************/
bool g_bUnlock = false;
bool g_bUnlockZ1 = false;
bool g_bUnlockZ2 = false;
uint32_t gu32_Z1Password[4] = { 0xFFFFFFFF };
uint32_t gu32_Z2Password[4] = { 0xFFFFFFFF };
/********************************运行功能***************************************/
uint32_t gu32_branchAddress = 0x84000;   //入口地址
/**************************文件类相关路径字符串*********************************/
wchar_t *g_pszAppFile = NULL;
wchar_t *g_pszAppFile2 = NULL;
wchar_t *g_pszKernelFile = NULL;
wchar_t *g_pszKernelFile2 = NULL;
wchar_t *g_pszComPort = NULL;
wchar_t *g_pszBaudRate;
wchar_t *g_pszDeviceName = NULL;

/**********************************端口*****************************************/
HANDLE file;
DCB port;
/**********************************serialFlash类********************************/
SerialFlash* pSerialFlash = nullptr;

#define kernel

//*****************************************************************************
//
// The main entry point of the DFU programmer example application.
//
//*****************************************************************************
//int
//_tmain(int argc, TCHAR* argv[])
//{
//	int iExitCode = 0;
//
//	//
//	// Parse the command line parameters, print the welcome banner and
//	// tell the user about any errors they made.
//	//
//	ParseCommandLine(argc, argv);
//    if (g_pszKernelFile2 && g_pszAppFile2){
//		g_bDualCore = TRUE;
//	}
//
//	//
//	//开启端口
//	//
//	int iRetCode = 0;
//	TCHAR baudString[32];
//	TCHAR comString[32];
//

//

//

//
//
//	uint8_t* packet = (uint8_t*)malloc(100); 
//	uint32_t packetLength;
//
//	/***********************************************************************/
//	if (g_bf021 == true && g_bf2837xD == true) //F021内核
//	{
//        //下载核心文件
//#ifdef kernel
//		_tprintf(_T("\ncalling f021_DownloadKernel CPU1 Kernel\n"));
//		iRetCode = f021_DownloadKernel(g_pszKernelFile);  
//#endif 
//
//		Sleep(6);
//
//		//确保端口已经成功打开
//		//自动波特率锁定
//		VERBOSEPRINT(_T("\nAttempting autobaud to send function message..."));
//		autobaudLock();
//
//		bool done = false;
//		bool cpu1 = true;
//		bool cpu2 = false;
//		uint16_t command = 0;
//		uint16_t status = 0;
//		uint16_t data[10];
//		uint16_t length = 0;
//		string sector;
//		uint32_t branchAddress = 0;
//		//选择功能
//		while (!done)
//		{
//			_tprintf(_T("\nWhat operation do you want to perform?\n"));
//			_tprintf(_T("\t 1-DFU CPU1\n"));
//			_tprintf(_T("\t 2-DFU CPU2\n"));
//			_tprintf(_T("\t 3-Erase CPU1\n"));
//			_tprintf(_T("\t 4-Erase CPU2\n"));
//			_tprintf(_T("\t 5-Verify CPU1\n"));
//			_tprintf(_T("\t 6-Verify CPU2\n"));
//			_tprintf(_T("\t 7-Unlock CPU1 Zone 1\n"));
//			_tprintf(_T("\t 8-Unlock CPU1 Zone 2\n"));
//			_tprintf(_T("\t 9-Unlock CPU2 Zone 1\n"));
//			_tprintf(_T("\t10-Unlock CPU2 Zone 2\n"));
//			_tprintf(_T("\t11-Run CPU1\n"));
//			_tprintf(_T("\t12-Reset CPU1\n"));
//			_tprintf(_T("\t13-Run CPU1 and Boot CPU2\n"));
//			_tprintf(_T("\t14-Reset CPU1 and Boot CPU2\n"));
//			_tprintf(_T("\t15-Run CPU2\n"));
//			_tprintf(_T("\t16-Reset CPU2\n"));
//			_tprintf(_T("\t 0-DONE\n"));
//			uint32_t answer;
//			cin >> dec >> answer;
//
//			switch (answer)
//			{
//

//		    //------------------------------------ERASE_CPU2------------------------------//
//			case 4:
//				gu32_EraseSectors2 = 0;
//				if (cpu2 == false)
//				{
//					cout << "ERROR: You must Boot_CPU2 before performing any operations on CPU2!" << endl;
//					break;
//				}
//				_tprintf(_T("Please input which Sectors (Letter) you want to erase for CPU2. Type '0' when finished.\n"));
//				_tprintf(_T("To Erase all of the Sectors type \"ALL\".\n"));
//				_tprintf(_T("First Sector: "));
//				cin >> sector;
//				while (sector.compare("0") && sector.compare(0, 3, "ALL"))
//				{
//					setEraseSector(2U, sector[0]);
//					_tprintf(_T("Next Sector:  "));
//					cin >> sector;
//				}
//				if (sector == "ALL")
//				{
//					gu32_EraseSectors2 = 0xFFFFFFFF & gu32_SectorMask;
//				}
//				packetLength = constructPacket(packet, ERASE_CPU2, 4, (uint8_t*)&gu32_EraseSectors2);
//				_tprintf(_T("\ncalling f021_SendPacket\n"));
//				iRetCode = f021_SendPacket(packet, packetLength); //-1 means NACK, 0 means ACK	
//				
//				command = getPacket(&length, data);
//				if (command != ERASE_CPU2)
//				{
//					cout << "ERROR with Packet Command!" << endl;
//				}
//				if (data[0] != NO_ERROR)
//				{
//					printErrorStatus(data[0]);
//					// format mem addr output.
//					cout << "ERROR Address: 0x" << hex << formatMemAddr(data[2], data[1]) << endl;
//				}
//				break;
//
//				//------------------------------------VERIFY_CPU1------------------------------//
//			case 5:
//				if (!g_pszAppFile)
//				{
//					cout << "ERROR: No flash application specified for CPU1 Flash Verification!" << endl;
//					ExitApp(3);
//				}
//				if (cpu1 == false)
//				{
//					cout << "ERROR: Cannot perform operations on CPU1 after CPU2 is booted and given control of SCI!" << endl;
//					break;
//				}
//				packetLength = constructPacket(packet, (uint16_t)VERIFY_CPU1, 0, NULL);
//				_tprintf(_T("\ncalling f021_SendPacket\n"));
//				iRetCode = f021_SendPacket(packet, packetLength); //-1 means NACK, 0 means ACK
//
//				iRetCode = f021_DownloadImage(g_pszAppFile);
//
//				command = getPacket(&length, data);
//				if (command != VERIFY_CPU1)
//				{
//					cout << "ERROR with Packet Command!" << endl;
//				}
//				if (data[0] != NO_ERROR)
//				{
//					printErrorStatus(data[0]);
//					// format mem addr output.
//					cout << "ERROR Address: 0x" << hex << formatMemAddr(data[2], data[1]) << endl;
//				}
//				break;
//
//				//------------------------------------VERIFY_CPU2------------------------------//
//			case 6:
//				if (!g_pszAppFile2)
//				{
//					cout << "ERROR: No flash application specified for CPU2 Flash Verification!" << endl;
//					ExitApp(3);
//				}
//				if (cpu2 == false)
//				{
//					cout << "ERROR: You must Boot_CPU2 before performing any operations on CPU2!" << endl;
//					break;
//				}
//				packetLength = constructPacket(packet, (uint16_t)VERIFY_CPU2, 0, NULL);
//				_tprintf(_T("\ncalling f021_SendPacket\n"));
//				iRetCode = f021_SendPacket(packet, packetLength); //-1 means NACK, 0 means ACK
//
//				iRetCode = f021_DownloadImage(g_pszAppFile2);
//
//				command = getPacket(&length, data);
//				if (command != VERIFY_CPU2)
//				{
//					cout << "ERROR with Packet Command!" << endl;
//				}
//				if (data[0] != NO_ERROR)
//				{
//					printErrorStatus(data[0]);
//					// format mem addr output.
//					cout << "ERROR Address: 0x" << hex << formatMemAddr(data[2], data[1]) << endl;
//				}
//				break;
//
//				//------------------------------------UNLOCK_CPU1_Z1------------------------------//
//			case 7:
//				if (cpu1 == false)
//				{
//					cout << "ERROR: Cannot perform operations on CPU1 after CPU2 is booted and given control of SCI!" << endl;
//					break;
//				}
//				_tprintf(_T("\nPlease input the 128-bit password for Zone 1 as 4 32-bit hexadecimal numbers.\n"));
//				_tprintf(_T("Zone 1 Password 1st 32-bits: "));
//				cin >> hex >> gu32_Z1Password[0];
//				_tprintf(_T("Zone 1 Password 2nd 32-bits: "));
//				cin >> hex >> gu32_Z1Password[1];
//				_tprintf(_T("Zone 1 Password 3rd 32-bits: "));
//				cin >> hex >> gu32_Z1Password[2];
//				_tprintf(_T("Zone 1 Password 4th 32-bits: "));
//				cin >> hex >> gu32_Z1Password[3];
//
//				packetLength = constructPacket(packet, (uint16_t)CPU1_UNLOCK_Z1, 16, (uint8_t*)gu32_Z1Password);
//				_tprintf(_T("\ncalling f021_SendPacket\n"));
//				iRetCode = f021_SendPacket(packet, packetLength); //-1 means NACK, 0 means ACK
//				command = getPacket(&length,  data);
//				if (command != CPU1_UNLOCK_Z1)
//				{
//					cout << "ERROR with Packet Command!" << endl;
//				}
//				if (data[0] != NO_ERROR)
//				{
//					printErrorStatus(data[0]);
//					// format mem addr output.
//					cout << "ERROR Address: 0x" << hex << formatMemAddr(data[2], data[1]) << endl;
//				}
//				break;
//
//				//------------------------------------UNLOCK_CPU1_Z2------------------------------//				
//			case 8:
//				if (cpu1 == false)
//				{
//					cout << "ERROR: Cannot perform operations on CPU1 after CPU2 is booted and given control of SCI!" << endl;
//					break;
//				}
//				_tprintf(_T("\nPlease input the 128-bit password for Zone 2 as 4 32-bit hexadecimal numbers.\n"));
//				_tprintf(_T("Zone 2 Password 1st 32-bits: "));
//				cin >> hex >> gu32_Z2Password[0];
//				_tprintf(_T("Zone 2 Password 2nd 32-bits: "));
//				cin >> hex >> gu32_Z2Password[1];
//				_tprintf(_T("Zone 2 Password 3rd 32-bits: "));
//				cin >> hex >> gu32_Z2Password[2];
//				_tprintf(_T("Zone 2 Password 4th 32-bits: "));
//				cin >> hex >> gu32_Z2Password[3];
//
//				packetLength = constructPacket(packet, (uint16_t)CPU1_UNLOCK_Z2, 16, (uint8_t*)gu32_Z2Password);
//				_tprintf(_T("\ncalling f021_SendPacket\n"));
//				iRetCode = f021_SendPacket(packet, packetLength); //-1 means NACK, 0 means ACK
//				command = getPacket(&length, data);
//				if (command != CPU1_UNLOCK_Z2)
//				{
//					cout << "ERROR with Packet Command!" << endl;
//				}
//				if (data[0] != NO_ERROR)
//				{
//					printErrorStatus(data[0]);
//					// format mem addr output.
//					cout << "ERROR Address: 0x" << hex << formatMemAddr(data[2], data[1]) << endl;
//				}
//				break;
//
//				//------------------------------------UNLOCK_CPU2_Z1------------------------------//
//			case 9:
//				if (cpu2 == false)
//				{
//					cout << "ERROR: You must Boot_CPU2 before performing any operations on CPU2!" << endl;
//					break;
//				}
//				_tprintf(_T("\nPlease input the 128-bit password for Zone 1 as 4 32-bit hexadecimal numbers.\n"));
//				_tprintf(_T("Zone 1 Password 1st 32-bits: "));
//				cin >> hex >> gu32_Z1Password[0];
//				_tprintf(_T("Zone 1 Password 2nd 32-bits: "));
//				cin >> hex >> gu32_Z1Password[1];
//				_tprintf(_T("Zone 1 Password 3rd 32-bits: "));
//				cin >> hex >> gu32_Z1Password[2];
//				_tprintf(_T("Zone 1 Password 4th 32-bits: "));
//				cin >> hex >> gu32_Z1Password[3];
//
//				packetLength = constructPacket(packet, (uint16_t)CPU2_UNLOCK_Z1, 16, (uint8_t*)gu32_Z1Password);
//				_tprintf(_T("\ncalling f021_SendPacket\n"));
//				iRetCode = f021_SendPacket(packet, packetLength); //-1 means NACK, 0 means ACK
//				command = getPacket(&length, data);
//				if (command != CPU2_UNLOCK_Z1)
//				{
//					cout << "ERROR with Packet Command!" << endl;
//				}
//				if (data[0] != NO_ERROR)
//				{
//					printErrorStatus(data[0]);
//					// format mem addr output.
//					cout << "ERROR Address: 0x" << hex << formatMemAddr(data[2], data[1]) << endl;
//				}
//				break;
//
//				//------------------------------------UNLOCK_CPU2_Z2------------------------------//				
//			case 10:
//				if (cpu2 == false)
//				{
//					cout << "ERROR: You must Boot_CPU2 before performing any operations on CPU2!" << endl;
//					break;
//				}
//				_tprintf(_T("\nPlease input the 128-bit password for Zone 2 as 4 32-bit hexadecimal numbers.\n"));
//				_tprintf(_T("Zone 2 Password 1st 32-bits: "));
//				cin >> hex >> gu32_Z2Password[0];
//				_tprintf(_T("Zone 2 Password 2nd 32-bits: "));
//				cin >> hex >> gu32_Z2Password[1];
//				_tprintf(_T("Zone 2 Password 3rd 32-bits: "));
//				cin >> hex >> gu32_Z2Password[2];
//				_tprintf(_T("Zone 2 Password 4th 32-bits: "));
//				cin >> hex >> gu32_Z2Password[3];
//
//				packetLength = constructPacket(packet, (uint16_t)CPU2_UNLOCK_Z2, 16, (uint8_t*)gu32_Z2Password);
//				_tprintf(_T("\ncalling f021_SendPacket\n"));
//				iRetCode = f021_SendPacket(packet, packetLength); //-1 means NACK, 0 means ACK
//				command = getPacket(&length, data);
//				if (command != CPU2_UNLOCK_Z2)
//				{
//					cout << "ERROR with Packet Command!" << endl;
//				}
//				if (data[0] != NO_ERROR)
//				{
//					printErrorStatus(data[0]);
//					// format mem addr output.
//					cout << "ERROR Address: 0x" << hex << formatMemAddr(data[2], data[1]) << endl;
//				}
//				break;
//
//				//------------------------------------RUN_CPU1----------------------------------//			
//			case 11:
//				if (cpu1 == false)
//				{
//					cout << "ERROR: Cannot perform operations on CPU1 after CPU2 is booted and given control of SCI!" << endl;
//					break;
//				}
//				_tprintf(_T("\nPlease input a hexadecimal address to branch to:  "));
//				cin >> hex >> branchAddress;
//				packetLength = constructPacket(packet, (uint16_t)RUN_CPU1, 4, (uint8_t*)&branchAddress);
//				_tprintf(_T("\ncalling f021_SendPacket\n"));
//				iRetCode = f021_SendPacket(packet, packetLength); //-1 means NACK, 0 means ACK
//				ExitApp(5);
//				break;
//
//				//------------------------------------RESET_CPU1---------------------------------//			
//			case 12:
//				if (cpu1 == false)
//				{
//					cout << "ERROR: Cannot perform operations on CPU1 after CPU2 is booted and given control of SCI!" << endl;
//					break;
//				}
//				packetLength = constructPacket(packet, (uint16_t)RESET_CPU1, 0, NULL);
//				_tprintf(_T("\ncalling f021_SendPacket\n"));
//				iRetCode = f021_SendPacket(packet, packetLength); //-1 means NACK, 0 means ACK
//				ExitApp(5);
//				break;
//
//				//------------------------------------RUN_CPU1_BOOT_CPU2---------------------------------------//	
//			case 13: 
//				if (!g_pszKernelFile2)
//				{
//					cout << "ERROR: No CPU2 flash kernel provided!" << endl;
//					ExitApp(3);
//				}
//				if (cpu1 == false)
//				{
//					cout << "ERROR: Cannot perform operations on CPU1 after CPU2 is booted and given control of SCI!" << endl;
//					break;
//				}
//				_tprintf(_T("\nPlease input a hexadecimal address to branch to:  "));
//				cin >> hex >> branchAddress;
//				packetLength = constructPacket(packet, (uint16_t)RUN_CPU1_BOOT_CPU2, 4, (uint8_t*)&branchAddress);
//				_tprintf(_T("\ncalling f021_SendPacket\n"));
//				iRetCode = f021_SendPacket(packet, packetLength); //-1 means NACK, 0 means ACK
//
//				Sleep(1000);
//
//				//no acknowledge packet
//				//Send Kernel
//				_tprintf(_T("\ncalling f021_DownloadKernel CPU2 Kernel\n"));
//				iRetCode = f021_DownloadKernel(g_pszKernelFile2);
//
//				// added for delay (diff repo/curr copy).
//				Sleep(6);
//		
//
//				// Do AutoBaud
//				VERBOSEPRINT(_T("\nAttempting autobaud to send function message..."));
//				autobaudLock();
//
//				cpu2 = true;
//				cpu1 = false;
//				break;
//
//				//------------------------------------RESET_CPU1_BOOT_CPU2---------------------------------------//	
//			case 14:
//				if (!g_pszKernelFile2)
//				{
//					cout << "ERROR: No CPU2 flash kernel provided!" << endl;
//					ExitApp(3);
//				}
//				if (cpu1 == false)
//				{
//					cout << "ERROR: Cannot perform operations on CPU1 after CPU2 is booted and given control of SCI!" << endl;
//					break;
//				}
//				packetLength = constructPacket(packet, (uint16_t)RESET_CPU1_BOOT_CPU2, 0, NULL);
//				_tprintf(_T("\ncalling f021_SendPacket\n"));
//				iRetCode = f021_SendPacket(packet, packetLength); //-1 means NACK, 0 means ACK
//
//				Sleep(1000);
//
//				//no acknowledge packet
//				//Send Kernel
//				_tprintf(_T("\ncalling f021_DownloadKernel CPU2 Kernel\n"));
//				iRetCode = f021_DownloadKernel(g_pszKernelFile2);
//
//				// added for delay (diff repo/curr copy).
//				Sleep(6);
//
//				// Do AutoBaud
//				VERBOSEPRINT(_T("\nAttempting autobaud to send function message..."));
//				autobaudLock();
//
//				cpu2 = true;
//				cpu1 = false;
//				break;
//
//				//------------------------------------RUN_CPU2----------------------------------//			
//			case 15:
//				if (cpu2 == false)
//				{
//					cout << "ERROR: You must Boot_CPU2 before performing any operations on CPU2!" << endl;
//					break;
//				}
//				_tprintf(_T("\nPlease input a hexadecimal address to branch to:  "));
//				cin >> hex >> branchAddress;
//				packetLength = constructPacket(packet, (uint16_t)RUN_CPU2, 4, (uint8_t*)&branchAddress);
//				_tprintf(_T("\ncalling f021_SendPacket\n"));
//				iRetCode = f021_SendPacket(packet, packetLength); //-1 means NACK, 0 means ACK
//				ExitApp(5);
//				break;
//
//				//------------------------------------RESET_CPU2---------------------------------//			
//			case 16:
//				if (cpu2 == false)
//				{
//					cout << "ERROR: You must Boot_CPU2 before performing any operations on CPU2!" << endl;
//					break;
//				}
//				packetLength = constructPacket(packet, (uint16_t)RESET_CPU2, 0, NULL);
//				_tprintf(_T("\ncalling f021_SendPacket\n"));
//				iRetCode = f021_SendPacket(packet, packetLength); //-1 means NACK, 0 means ACK
//				ExitApp(5);
//				break;
//
//				//------------------------------------DONE-----------------------------------------//
//			case 0:
//				done = true;
//				cout << "Exiting the Application." << endl;
//				break;
//
//				//------------------------------------DEFAULT-------------------------------------//
//			default:
//				done = true;
//				cout << "Exiting the Application." << endl;
//				break;
//			}
//		}
//
//	}
//	else if (g_bf021 == true && (g_bf2807x || g_bf2837xS || g_bf28004x)) 
//	{
//		//
//		// Download the Kernel
//		//
//#ifdef kernel
//		_tprintf(_T("\ncalling f021_DownloadKernel CPU1 Kernel\n"));
//		iRetCode = f021_DownloadKernel(g_pszKernelFile);
//#endif 
//
//		// added for delay (diff repo/curr copy).
//		Sleep(6);
//		
//		//
//		// COM port is open
//		//
//		// Do AutoBaud
//		VERBOSEPRINT(_T("\nAttempting autobaud to send function message..."));
//		autobaudLock();
//
//		bool done = false;
//		bool cpu1 = true;
//		bool cpu2 = false;
//		uint16_t command = 0;
//		uint16_t status = 0;
//		uint16_t data[10];
//		uint16_t length = 0;
//		string sector;
//		uint32_t branchAddress = 0;
//		while (!done)
//		{
//			_tprintf(_T("\nWhat operation do you want to perform?\n"));
//			_tprintf(_T("\t 1-DFU\n"));
//			_tprintf(_T("\t 2-Erase\n"));
//			_tprintf(_T("\t 3-Verify\n"));
//			_tprintf(_T("\t 4-Unlock Zone 1\n"));
//			_tprintf(_T("\t 5-Unlock Zone 2\n"));
//			_tprintf(_T("\t 6-Run\n"));
//			_tprintf(_T("\t 7-Reset\n"));
//			_tprintf(_T("\t 8-Live DFU\n"));
//			_tprintf(_T("\t 0-DONE\n"));
//			uint32_t answer;
//			cin >> dec >> answer;
//			switch (answer)
//			{
//				//------------------------------------DFU_CPU1------------------------------//
//			case 1:
//				if (!g_pszAppFile)
//				{
//					cout << "ERROR: No flash application specified for CPU1 Flash Programming!" << endl;
//					ExitApp(3);
//				}
//				if (cpu1 == false)
//				{
//					cout << "ERROR: Cannot perform operations on CPU1 after CPU2 is booted and given control of SCI!" << endl;
//					break;
//				}
//				packetLength = constructPacket(packet, (uint16_t)DFU_CPU1, 0, NULL);
//				_tprintf(_T("\ncalling f021_SendPacket\n"));
//				iRetCode = f021_SendPacket(packet, packetLength); //-1 means NACK, 0 means ACK
//
//				iRetCode = f021_DownloadImage(g_pszAppFile);
//
//				command = getPacket(&length, data);
//				if (command != DFU_CPU1)
//				{
//					cout << "ERROR with Packet Command!" << endl;
//				}
//				if (data[0] != NO_ERROR)
//				{
//					printErrorStatus(data[0]);
//					cout << "ERROR Address: 0x" << hex << formatMemAddr(data[2], data[1]) << endl;
//				}
//				else  //if NO_ERROR then statusCode.address or data[2]|data[1] is the EnrtyAddr
//				{
//					// format mem addr output.
//					cout << endl << "Entry Point Address is: 0x" << hex << formatMemAddr(data[2],data[1]) << endl;
//				}
//				break;
//
//				//------------------------------------ERASE_CPU1------------------------------//
//			case 2:
//				gu32_EraseSectors1 = 0;
//				if (cpu1 == false)
//				{
//					cout << "ERROR: Cannot perform operations on CPU1 after CPU2 is booted and given control of SCI!" << endl;
//					break;
//				}
//				
//				// (0~15 sectors instead of A~P and "END" to stop instead of "0")
//				if (g_bf28004x == true) {
//					_tprintf(_T("Please input which Sectors you want to erase for CPU1. Type '0' when finished.\n"));
//					_tprintf(_T("A~P: Bank Zero sector 0 to 15. a~p: Bank One sector 0 to 15.\n"));
//					_tprintf(_T("For example, you would put b for Bank One, sector 1.\n"));
//					_tprintf(_T("To Erase all of the Sectors type \"ALL\".\n"));
//					_tprintf(_T("First Sector: "));
//					cin >> sector;
//					while (sector.compare("0") && sector.compare(0, 3, "ALL"))
//					{
//						setEraseSector(1U, sector[0]);
//						_tprintf(_T("Next Sector:  "));
//						cin >> sector;
//					}
//				} else {
//					_tprintf(_T("Please input which Sectors (Letter) you want to erase for CPU1. Type '0' when finished.\n"));
//					_tprintf(_T("To Erase all of the Sectors type \"ALL\".\n"));
//					_tprintf(_T("First Sector: "));
//					cin >> sector;
//					while (sector.compare("0") && sector.compare(0, 3, "ALL"))
//					{
//						// The pattern is A = 1, B = 2,.... Z = 26. To make AA = 27 and AB = 28, following the below approach
//						if(!sector.compare(0, 2, "AA") || !sector.compare(0, 2, "AB"))
//						{
//							sector[0] = sector[0] + (sector[1] - 65) + 26;
//						}
//
//						setEraseSector(1U, sector[0]);
//						_tprintf(_T("Next Sector:  "));
//						cin >> sector;
//					}
//				}
//				
//				if (sector == "ALL")
//				{
//					gu32_EraseSectors1 = 0xFFFFFFFF & gu32_SectorMask;
//				}
//				packetLength = constructPacket(packet, ERASE_CPU1, 4, (uint8_t*)&gu32_EraseSectors1);
//				_tprintf(_T("\ncalling f021_SendPacket\n"));
//				iRetCode = f021_SendPacket(packet, packetLength); //-1 means NACK, 0 means ACK	
//
//				command = getPacket(&length, data);
//				if (command != ERASE_CPU1)
//				{
//					cout << "ERROR with Packet Command!" << endl;
//				}
//				if (data[0] != NO_ERROR)
//				{
//					printErrorStatus(data[0]);
//					// format mem addr output.
//					cout << "ERROR Address: 0x" << hex << formatMemAddr(data[2], data[1]) << endl;
//				}
//				break;
//
//				//------------------------------------VERIFY_CPU1------------------------------//
//			case 3:
//				if (!g_pszAppFile)
//				{
//					cout << "ERROR: No flash application specified for CPU1 Flash Verification!" << endl;
//					ExitApp(3);
//				}
//				if (cpu1 == false)
//				{
//					cout << "ERROR: Cannot perform operations on CPU1 after CPU2 is booted and given control of SCI!" << endl;
//					break;
//				}
//				packetLength = constructPacket(packet, (uint16_t)VERIFY_CPU1, 0, NULL);
//				_tprintf(_T("\ncalling f021_SendPacket\n"));
//				iRetCode = f021_SendPacket(packet, packetLength); //-1 means NACK, 0 means ACK
//
//				iRetCode = f021_DownloadImage(g_pszAppFile);
//
//				command = getPacket(&length, data);
//				if (command != VERIFY_CPU1)
//				{
//					cout << "ERROR with Packet Command!" << endl;
//				}
//				if (data[0] != NO_ERROR)
//				{
//					printErrorStatus(data[0]);
//					// format mem addr output.
//					cout << "ERROR Address: 0x" << hex << formatMemAddr(data[2], data[1]) << endl;
//				}
//				break;
//
//				//------------------------------------UNLOCK_CPU1_Z1------------------------------//
//			case 4:
//				if (cpu1 == false)
//				{
//					cout << "ERROR: Cannot perform operations on CPU1 after CPU2 is booted and given control of SCI!" << endl;
//					break;
//				}
//				_tprintf(_T("\nPlease input the 128-bit password for Zone 1 as 4 32-bit hexadecimal numbers.\n"));
//				_tprintf(_T("Zone 1 Password 1st 32-bits: "));
//				cin >> hex >> gu32_Z1Password[0];
//				_tprintf(_T("Zone 1 Password 2nd 32-bits: "));
//				cin >> hex >> gu32_Z1Password[1];
//				_tprintf(_T("Zone 1 Password 3rd 32-bits: "));
//				cin >> hex >> gu32_Z1Password[2];
//				_tprintf(_T("Zone 1 Password 4th 32-bits: "));
//				cin >> hex >> gu32_Z1Password[3];
//
//				packetLength = constructPacket(packet, (uint16_t)CPU1_UNLOCK_Z1, 16, (uint8_t*)gu32_Z1Password);
//				_tprintf(_T("\ncalling f021_SendPacket\n"));
//				iRetCode = f021_SendPacket(packet, packetLength); //-1 means NACK, 0 means ACK
//				command = getPacket(&length, data);
//				if (command != CPU1_UNLOCK_Z1)
//				{
//					cout << "ERROR with Packet Command!" << endl;
//				}
//				if (data[0] != NO_ERROR)
//				{
//					printErrorStatus(data[0]);
//					// format mem addr output.
//					cout << "ERROR Address: 0x" << hex << formatMemAddr(data[2], data[1]) << endl;
//				}
//				break;
//
//				//------------------------------------UNLOCK_CPU1_Z2------------------------------//				
//			case 5:
//				if (cpu1 == false)
//				{
//					cout << "ERROR: Cannot perform operations on CPU1 after CPU2 is booted and given control of SCI!" << endl;
//					break;
//				}
//				_tprintf(_T("\nPlease input the 128-bit password for Zone 2 as 4 32-bit hexadecimal numbers.\n"));
//				_tprintf(_T("Zone 2 Password 1st 32-bits: "));
//				cin >> hex >> gu32_Z2Password[0];
//				_tprintf(_T("Zone 2 Password 2nd 32-bits: "));
//				cin >> hex >> gu32_Z2Password[1];
//				_tprintf(_T("Zone 2 Password 3rd 32-bits: "));
//				cin >> hex >> gu32_Z2Password[2];
//				_tprintf(_T("Zone 2 Password 4th 32-bits: "));
//				cin >> hex >> gu32_Z2Password[3];
//
//				packetLength = constructPacket(packet, (uint16_t)CPU1_UNLOCK_Z2, 16, (uint8_t*)gu32_Z2Password);
//				_tprintf(_T("\ncalling f021_SendPacket\n"));
//				iRetCode = f021_SendPacket(packet, packetLength); //-1 means NACK, 0 means ACK
//				command = getPacket(&length, data);
//				if (command != CPU1_UNLOCK_Z2)
//				{
//					cout << "ERROR with Packet Command!" << endl;
//				}
//				if (data[0] != NO_ERROR)
//				{
//					printErrorStatus(data[0]);
//					// format mem addr output.
//					cout << "ERROR Address: 0x" << hex << formatMemAddr(data[2], data[1]) << endl;
//				}
//				break;
//				//------------------------------------LIVE_DFU_CPU1---------------------------------//
//			case 8:
//
//				//
//				// Ensure that an application is specified in the command arguments
//				//
//				if (!g_pszAppFile)
//				{
//					cout << "ERROR: No flash application specified for CPU1 Flash Programming!" << endl;
//					ExitApp(3);
//				}
//				if (cpu1 == false)
//				{
//					cout << "ERROR: Cannot perform operations on CPU1!" << endl;
//					break;
//				}
//
//				packetLength = constructPacket(packet, (uint16_t)LIVE_DFU_CPU1, 0, NULL);
//				_tprintf(_T("\ncalling f021_SendPacket\n"));
//
//				Sleep(500);
//
//				//
//				// Send command to device
//				//
//				iRetCode = f021_SendPacket(packet, packetLength); //-1 means NACK, 0 means ACK
//
//				Sleep(500);
//
//				//
//				// Load specified file to device
//				//
//				iRetCode = f021_DownloadImage(g_pszAppFile);
//
//				break;
//			}
//		}
//	}
//	/***********************************************************************/
//	else if (g_bf05 == true)
//	{
//        //download kernel and application for F05 devices
//		_tprintf(_T("\ncalling f05_DownloadImage\n"));
//		iRetCode = f05_DownloadImage();
//	}
//
//	ExitApp(iExitCode);
//}

//*****************************************************************************
//函数名称：
// 退出应用程序，可以选择先暂停按键。
//
//*****************************************************************************
void ExitApp(int iRetcode)
{
    //判断配置信息：退出程序是否等待按键
	if (g_bWaitOnExit)
	{
		_tprintf(_T("\nPress any key to exit...\n"));
		while (!_kbhit())
		{
			//等待任意按键
		}
	}   
	exit(iRetcode);
}

//*****************************************************************************
//
// 打印欢迎信息
//
//*****************************************************************************
void PrintWelcome(void)
{
	if (g_bQuiet){return;}
	_tprintf(_T("\nC2000 Serial Firmware Upgrader\n"));
	_tprintf(_T("Copyright (c) 2013 Texas Instruments Incorporated.  All rights reserved.\n\n"));
}
//*****************************************************************************
//
// 在应用程序命令行参数上显示帮助。
//
//*****************************************************************************
void ShowHelp(void)
{
	if (g_bQuiet)
	{
		return;
	}

	_tprintf(_T("This application may be used to download images to a Texas Instruments\n"));
	_tprintf(_T("C2000 microcontroller in the SCI boot mode.\n\n"));
	_tprintf(_T("Supported parameters are:\n\n"));
	_tprintf(_T("-d <device>   - The name of the device to load to.\n"));
	_tprintf(_T("               f2802x, f2803x, f2805x, f2806x, f2833x, f2837xD, f2837xS, f2807x, or f28004x.\n")); 
	_tprintf(_T("-k <file>     - The file name for flash kernel.\n"));
	_tprintf(_T("               This file must be in the SCI boot format.\n"));
    _tprintf(_T("-a <file>     - The file name for download use.\n"));
	_tprintf(_T("               This file must be in the SCI boot format.\n"));
	_tprintf(_T("-p COM<num>   - Set the COM port to be used for communications.\n"));
	_tprintf(_T("-b <num>      - Set the baud rate for the COM port.\n"));
	_tprintf(_T("               If device is f2837xD and is dual core, you can provide optional arguments for dual\n"));
	_tprintf(_T("                  core firmware upgrade\n"));
	_tprintf(_T("-m <file>     - The CPU02 file name for flash kernel in dual core operations.\n"));
	_tprintf(_T("              - This file must be in the ASCII SCI boot format.\n"));
	_tprintf(_T("-n <file>     - The CPU02 file name for download use in dual core operations.\n"));
	_tprintf(_T("              - This file must be in the ASCII SCI boot format.\n"));

	_tprintf(_T("-? or -h      - Show this help.\n"));
	_tprintf(_T("-q            - Quiet mode. Disable output to stdout.\n"));
	_tprintf(_T("-w            - Wait for a key press before exiting.\n"));
	_tprintf(_T("-v            - Enable verbose output\n\n"));
	_tprintf(_T("-d -f, and -p are mandatory parameters.  If baud rate is omitted, \nthe communications will occur at 9600 baud.\n\n"));

	_tprintf(_T("Application and kernel files must be in the SCI8 ascii boot format. \nThese can be generated using the hex2000 utility.  An example of how to do \nthis follows:\nhex2000 application.out -boot -sci8 -a -o application.txt\n\n"));
}

//*****************************************************************************
//
//解析命令行，提取所有参数。
//
//成功返回0。 失败时，调用ExitApp（1）。
//
//*****************************************************************************
int ParseCommandLine(int argc, wchar_t *argv[])
{
	int iParm;
	bool bShowHelp;
	wchar_t *pcOptArg;

	//
	// By default, don't show the help screen.
	//
	bShowHelp = false;

	//
	// Set the default baud rate
	//
	g_pszBaudRate = L"9600";

	//
	// Walk through each of the parameters in the list, skipping the first one
	// which is the executable name itself.
	//
	for (iParm = 1; iParm < argc; iParm++)
	{
		//
		// Does this look like a valid switch?
		//
		if (!argv || ((argv[iParm][0] != L'-') && (argv[iParm][0] != L'/')) ||
			(argv[iParm][1] == L'\0'))
		{
        	//
			// We found something on the command line that didn't look like a
			// switch so ExitApp.
			//
			_tprintf(_T("Unrecognized or invalid argument: %s\n"), argv[iParm]);
			ExitApp(1);
		}
		else
		{
			//
			// Get a pointer to the next argument since it may be used
			// as a parameter for the case statements 
            //
			pcOptArg = ((iParm + 1) < argc) ? argv[iParm + 1] : NULL;
		}

		switch (argv[iParm][1])
		{
		case 'd':
			g_pszDeviceName = pcOptArg;
			iParm++;
			setDeviceName();
			break;

		case 'k':
			g_pszKernelFile = pcOptArg;
			iParm++;
			break;

		case 'a':
			g_pszAppFile = pcOptArg;
			iParm++;
			break;

		case 'm':
			g_pszKernelFile2 = pcOptArg;
			iParm++;
			break;

		case 'n':
			g_pszAppFile2 = pcOptArg;
			iParm++;
			break;

		case 'b':
			g_pszBaudRate = pcOptArg;
			iParm++;
			break;

		case 'p':
			g_pszComPort = pcOptArg;
			iParm++;
			break;

		case 'v':
			g_bVerbose = TRUE;
			break;

		case 'q':
			g_bQuiet = TRUE;
			break;

		case 'w':
			g_bWaitOnExit = TRUE;
			break;		

		case '?':
		case 'h':
			bShowHelp = TRUE;
			break;

		default:
			_tprintf(_T("Unrecognized argument: %s\n\n"), argv[iParm]);
			ShowHelp();
			ExitApp(1);

		}
	}

	//
	// Show the welcome banner unless we have been told to be quiet.
	//
	PrintWelcome();

	//
	// Show the help screen if requested.
	//
	if (bShowHelp)
	{
		ShowHelp();
		ExitApp(0);
	}

	checkErrors();
	
	return(0);
}
//解析设备名称
void setDeviceName(void)
{
	//
	// if not a proper input device name
	// 0 == false, !false = !0 = true. strncmp && wcsncmp returns !0 = true if different.
	//
	if (wcsncmp(g_pszDeviceName, (wchar_t*)L"f2802x", 6) && wcsncmp(g_pszDeviceName, (wchar_t*)L"f2803x", 6) && wcsncmp(g_pszDeviceName, (wchar_t*)L"f2805x", 6)
		&& wcsncmp(g_pszDeviceName, (wchar_t*)L"f2806x", 6) && wcsncmp(g_pszDeviceName, (wchar_t*)L"f2833x", 6) && wcsncmp(g_pszDeviceName, (wchar_t*)L"f2837xD", 7) && wcsncmp(g_pszDeviceName, (wchar_t*)L"f2837xS", 7)
         && wcsncmp(g_pszDeviceName, (wchar_t*)L"f2807x", 6) && wcsncmp(g_pszDeviceName, (wchar_t*)L"f28004x", 7))
	{
		_tprintf(_T("\nUnrecognized device name: X%sX\n\n"), g_pszDeviceName);
		ShowHelp();
		ExitApp(2);
	}
	else if (!wcsncmp(g_pszDeviceName, (wchar_t*)L"f2802x", 6))
	{
		g_bf05 = true;
		g_bf2802x = true;
		gu32_SectorMask = 0xF; //4 sectors, D
	}
	else if (!wcsncmp(g_pszDeviceName, (wchar_t*)L"f2803x", 6))
	{
		g_bf05 = true;
		g_bf2803x = true;
		gu32_SectorMask = 0xFF; //8 sectors, H
	}
	else if (!wcsncmp(g_pszDeviceName, (wchar_t*)L"f2805x", 6))
	{
		g_bf05 = true;
		g_bf2805x = true;
		gu32_SectorMask = 0x3FF; //10 sectors, J
	}
	else if (!wcsncmp(g_pszDeviceName, (wchar_t*)L"f2806x", 6))
	{
		g_bf05 = true;
		g_bf2806x = true;
		gu32_SectorMask = 0xFF; ///8 sectors, H
	}
	else if (!wcsncmp(g_pszDeviceName, (wchar_t*)L"f2833x", 6))
	{
		g_bf05 = true;
		g_bf2833x = true;
		gu32_SectorMask = 0xFF; ///8 sectors, H
	}
	else if (!wcsncmp(g_pszDeviceName, (wchar_t*)L"f2837xD", 7))
	{
		g_bf021 = true;
		g_bf2837xD = true;
		gu32_SectorMask = 0x3FFF; //14 sectors, N
	}
	else if (!wcsncmp(g_pszDeviceName, (wchar_t*)L"f2837xS", 7))
	{
		g_bf021 = true;
		g_bf2837xS = true;
		gu32_SectorMask = 0xFFFFFFF; //28 sectors, A to AB
	}
	else if (!wcsncmp(g_pszDeviceName, (wchar_t*)L"f2807x", 6))
	{
		g_bf021 = true;
		g_bf2807x = true;
		gu32_SectorMask = 0x3FFF; //14 sectors, M
	}
	else if (!wcsncmp(g_pszDeviceName, (wchar_t*)L"f28004x", 7))
	{
		g_bf021 = true;
		g_bf28004x = true;
		gu32_SectorMask = 0xFFFFFFFF; // 32 sectors, bank 0-sector 0~15, bank 1-sector 0~15
	}
	else
	{
		QUIETPRINT(_T("ERROR: Device name is not recognized.\n"));
		ExitApp(7);
	}
    return;
}
//擦除扇区
void setEraseSector(unsigned int CPU, uint32_t Sector)
{
	//get the correct EraseSectors variable
	uint32_t * ptr_EraseSectors;
	unsigned int sector;

	if (CPU == 1U)
	{
		ptr_EraseSectors = &gu32_EraseSectors1;
	}
	else if (CPU == 2U)
	{
		ptr_EraseSectors = &gu32_EraseSectors2;
	}
	//assign the Sector be erased to the de-referenced pointer
	// A : P / a : p to 1 : 32 mapping for Flash sectors.

	if (g_bf28004x == true) {
		// ASCII 65~80 is A~P, 97~112 is a~p.
		if (Sector >= 97 && Sector <= 112) {
			// lower case. a, 97 maps to 17.
			sector = Sector - 80;
		} else if (Sector >= 65 && Sector <= 80) {
			sector = Sector - 64;
		} else {
			// user has put something else than A~P/a~p
			sector = 0;
		}
	} else {
		// "A" ASCII value = 65.
		sector = Sector - 64;
	}
	
	if (sector >= 1 && sector <= 32) //the Capital Letter: A-P // 1~32 
	{
		// setting which sector to erase.
		unsigned int shift = sector - 1;
		*ptr_EraseSectors |= 1U << shift;
		*ptr_EraseSectors = *ptr_EraseSectors & gu32_SectorMask; //this eliminates any invalid sectors.
	}
	else
	{
		if (g_bf28004x == true) {
			QUIETPRINT(_T("ERROR: Sector is out of range.  Please use <A-P>(Bank Zero 0~15) or <a-p>(Bank One 0~15) to specify which sectors to erase.\n"));
		} else {
			QUIETPRINT(_T("ERROR: Sector is out of range.  Please use a capital letter <A-Z> to specify which sectors to erase.\n"));
		}

	}
	return;
}
//错误检查
void checkErrors(void)
{
	//
	// Catch no ComPort
	//
	if (!g_pszComPort)
	{
		//
		// No Com Port inputed.
		//
		ShowHelp();
		QUIETPRINT(_T("ERROR: No COM port number was specified. Please use -p to provide one.\n"));

	}
	//
	// Catch no CPU1 Kernel
	//
	if (!g_pszKernelFile)
	{
		//
		// No CPU1 Kernel inputed.
		//
		ShowHelp();
		QUIETPRINT(_T("ERROR: No COM port number was specified. Please use -p to provide one.\n"));
	}
}
//打印错误状态
void printErrorStatus(uint16_t status)
{
	switch (status)
	{
	case BLANK_ERROR:
		cout << "ERROR Status: BLANK_ERROR" << endl;
		break;
	case VERIFY_ERROR:
		cout << "ERROR Status: VERIFY_ERROR" << endl;
		break;
	case PROGRAM_ERROR:
		cout << "ERROR Status: PROGRAM_ERROR" << endl;
		break;
	case COMMAND_ERROR:
		cout << "ERROR Status: COMMAND_ERROR" << endl;
		break;
	case UNLOCK_ERROR:
		cout << "ERROR Status: UNLOCK_ERROR" << endl;
		break;
	default:
		cout << "ERROR Status: Not Recognized Error" << endl;
		break;
	}
}

//*****************************************************************************
//
// 内存地址输出错误修复。 例如）80-> 0x80000
//
//*****************************************************************************
uint32_t formatMemAddr(uint16_t firstHalf, uint16_t secondHalf) {
	uint32_t formatted = 0;

	formatted |= (firstHalf << 16);
	formatted |= secondHalf;

	return formatted;
}


/************************************************************************
* 函数名称：function_RESET_CPU1
* 函数说明：功能:复位CPU1
* 功能说明：
* 输入参数：
* 返回参数：
*           -2:错误：启动cpu2并给予sci控制后，无法对cpu1执行操作
*           -1:发送命令包无应答信号接收
*            0:操作正常
*************************************************************************/
int function_RESET_CPU1()
{	
	if (cpu1 == false){return -2;}
	uint8_t* packet = (uint8_t*)malloc(100);
	uint32_t packetLength;
	int iRetCode;
	packetLength = constructPacket(packet, (uint16_t)RESET_CPU1, 0, NULL);
	iRetCode = f021_SendPacket(packet, packetLength); 
	free(packet);
	return iRetCode;
}
/************************************************************************
* 函数名称：openCom
* 函数说明：开启端口
* 功能说明：
* 输入参数：
* 返回参数：
*           0:开启失败   1:开启成功
*************************************************************************/
int openCom()
{
	int iRetCode = 0;
	TCHAR baudString[32];
	TCHAR comString[32];
	//端口名称
	_stprintf(comString, _T("\\\\.\\%s"), g_pszComPort);
	//创建端口
	file = CreateFile((LPCWSTR)comString,GENERIC_READ | GENERIC_WRITE,0,NULL,OPEN_EXISTING,0,NULL);
    //检查COM口是否正确打开
	if (INVALID_HANDLE_VALUE == file) {
		QUIETPRINT(_T("无法打开端口:%s请检查设备\n"), g_pszComPort);
		return 0;
    }
    //将波特率附加到配置字符串
	_stprintf(baudString, _T("%s,n,8,1"), g_pszBaudRate);
	
	//获取当前的DCB
	memset(&port, 0, sizeof(port));
	port.DCBlength = sizeof(port);
	iRetCode = GetCommState(file, &port);
	if (iRetCode)
	{
	    QUIETPRINT(_T("获取端口状态成功\n"));
	}
	else
	{
		QUIETPRINT(_T("获取端口状态失败\n"));
	    return 0;
	}
	//重新建立DCB
	iRetCode = BuildCommDCB((LPCTSTR)baudString, &port);
	if (iRetCode)
	{
		QUIETPRINT(_T("建立端口配置成功\n"));
	}
	else
	{
		QUIETPRINT(_T("建立端口配置失败\n"));
		return 0;
	}
	//建立端口状态
	iRetCode = SetCommState(file, &port);
	if (iRetCode)
	{
		QUIETPRINT(_T("配置端口成功\n"));
	}
	else
	{
		QUIETPRINT(_T("配置端口失败\n"));
		return 0;
	}
	return 1;
}
/************************************************************************
* 函数名称：closeCom
* 函数说明：关闭端口
* 功能说明：
* 输入参数：
* 返回参数：
*           0:关闭失败   1:关闭成功
*************************************************************************/
int closeCom()
{
	/**如果有串口被打开，关闭它 */
	if (file != INVALID_HANDLE_VALUE)
	{
		if (CloseHandle(file) == false)
		{
			return 0;
		}
		file = INVALID_HANDLE_VALUE;
	}
	return 1;
}
/************************************************************************
* 函数名称：
* 函数说明：下载核心文件
* 功能说明：
* 输入参数：
* 返回参数：
*************************************************************************/
int downloadKernel()
{
	int iRetCode = 0;
	//F021内核,F2837xD双核设备
	if (g_bf021 == true && g_bf2837xD == true) 
	{
		//下载核心文件
#ifdef kernel
		_tprintf(_T("\ncalling f021_DownloadKernel CPU1 Kernel\n"));
		iRetCode = f021_DownloadKernel(g_pszKernelFile);
#endif 
		Sleep(6);

		//自动波特率锁定
		VERBOSEPRINT(_T("Attempting autobaud to send function message...\n"));
		autobaudLock();
	}
	//FO21内核，单核设备
	else if (g_bf021 == true && (g_bf2807x || g_bf2837xS || g_bf28004x))
	{
#ifdef kernel
		VERBOSEPRINT(_T("calling f021_DownloadKernel CPU1 Kernel\n"));
		iRetCode = f021_DownloadKernel(g_pszKernelFile);
#endif 
		Sleep(6);

		//波特率锁定
		VERBOSEPRINT(_T("Attempting autobaud to send function message...\n"));
		autobaudLock();
	}
	return 0;
}
/************************************************************************
* 函数名称：
* 函数说明：构造函数
* 功能说明：
* 输入参数：
* 返回参数：
*************************************************************************/
SerialFlash::SerialFlash()
{

}
SerialFlash::~SerialFlash()
{

}
/************************************************************************
* 函数名称：
* 函数说明：功能：CPU1升级 DFU_CPU1
* 功能说明：
* 输入参数：
* 返回参数：
*************************************************************************/
int SerialFlash::function_DFU_CPU1()
{
	//检查文件 
	if (!g_pszAppFile)
	{
		cout << "错误：未为CPU1闪存编程指定闪存应用程序" << endl;
		QTMESSAGE(QString("没有可用的APP文件，无法升级."));
		return 3;
	}
	//检查核心驱动
	if (cpu1 == false)
	{
		cout << "错误：引导CPU2并获得SCI控制后，无法在CPU1上执行操作！" << endl;
		QTMESSAGE(QString("CPU1未连接，无法升级."));
		return 2;
	}
	//编码命令包
	uint8_t* packet = (uint8_t*)malloc(100);
	uint32_t packetLength;
	int iRetCode;
	packetLength = constructPacket(packet, (uint16_t)DFU_CPU1, 0, NULL);
	cout << "发送升级命令(f021_SendPacket(DFU_CPU1))" << endl;
	QTMESSAGE(QString("发送升级命令."));
	//发送命令包
	iRetCode = f021_SendPacket(packet, packetLength);
	free(packet);
	//发送应用程序文件
	iRetCode = f021_DownloadImage(g_pszAppFile);
	//获取命令包
	uint16_t command = 0;
	uint16_t data[10];
	uint16_t length = 0;
	command = getPacket(&length, data);
	if (command != DFU_CPU1)
	{
		cout << "ERROR with Packet Command!" << endl;
	}
	if (data[0] != NO_ERROR)
	{
		printErrorStatus(data[0]);
		cout << "ERROR Address: 0x" << hex << formatMemAddr(data[2], data[1]) << endl;
	}
	else  //if NO_ERROR then statusCode.address or data[2]|data[1] is the EnrtyAddr
	{
		cout << endl << "Entry Point Address is: 0x" << hex << setw(4) << setfill('0') << data[2] << hex << setw(4) << setfill('0') << data[1] << endl;
	}
	//记录入口点
	gu32_branchAddress = formatMemAddr(data[2], data[1]);
}
/************************************************************************
* 函数名称：
* 函数说明： 功能:运行CPU1(RUN_CPU1)
* 功能说明：
* 输入参数：
* 返回参数：
*           -2:错误：启动cpu2并给予sci控制后，无法对cpu1执行操作
*           -1:发送命令包无应答信号接收
*            0:操作正常
*************************************************************************/
int SerialFlash::function_RUN_CPU1()
{		
	if (cpu1 == false) { return -2; }
	uint8_t* packet = (uint8_t*)malloc(100);
	uint32_t packetLength;
	int iRetCode;
	packetLength = constructPacket(packet, (uint16_t)RUN_CPU1, 4, (uint8_t*)&gu32_branchAddress);
	iRetCode = f021_SendPacket(packet, packetLength);
	free(packet);
	return iRetCode;
}
/************************************************************************
* 函数名称：
* 函数说明：功能：CPU2升级 DFU_CPU2
* 功能说明：
* 输入参数：
* 返回参数：
*************************************************************************/
int SerialFlash::function_DFU_CPU2()
{
	if (!g_pszAppFile2)
	{
		cout << "ERROR: 没有为CPU2 Flash编程指定Flash应用程序." << endl;
		return(3);
	}
	if (cpu2 == false)
	{
		cout << "ERROR: 在CPU2上执行任何操作之前，必须先启动Boot_CPU2." << endl;
		return(2);
	}
	/*发送命令包*/
	uint8_t* packet = (uint8_t*)malloc(100);
	uint32_t packetLength;
	int iRetCode;
	packetLength = constructPacket(packet, (uint16_t)DFU_CPU2, 0, NULL);
	_tprintf(_T("\ncalling f021_SendPacket.\n"));
	iRetCode = f021_SendPacket(packet, packetLength); //-1 means NACK, 0 means ACK
	free(packet);
	/*下载APP程序文件*/
	iRetCode = f021_DownloadImage(g_pszAppFile2);
	/*接收反馈包*/
	uint16_t command = 0;
	uint16_t data[10];
	uint16_t length = 0;
	command = getPacket(&length, data);

	if (command != DFU_CPU2)
	{
		cout << "ERROR:数据包命令错误!." << endl;
	}
	if (data[0] != NO_ERROR)
	{
		printErrorStatus(data[0]);
		// format mem addr output.
		cout << "ERROR Address: 0x" << hex << formatMemAddr(data[2], data[1]) << endl;
	}
	else  //if NO_ERROR then statusCode.address or data[2]|data[1] is the EnrtyAddr
	{
		cout << endl << "Entry Point Address is: 0x" << hex << setw(4) << setfill('0') << data[2] << hex << setw(4) << setfill('0') << data[1] << endl;
	}
}
///************************************************************************
//* 函数名称：
//* 函数说明：功能：运行CPU2(RUN_CUP2)
//* 功能说明：
//* 输入参数：
//* 返回参数：
//*************************************************************************/
int SerialFlash::function_RUN_CPU2()
{
	if (cpu2 == false)
	{
		cout << "ERROR: 在CPU2上执行任何操作之前，必须先启动CPU2!" << endl;
		return(3);
	}
	uint8_t* packet = (uint8_t*)malloc(100);
	uint32_t packetLength;
	int iRetCode;
	packetLength = constructPacket(packet, (uint16_t)RUN_CPU2, 4, (uint8_t*)&gu32_branchAddress);
	_tprintf(_T("\n发送RUN_CPU2命令\n"));
	iRetCode = f021_SendPacket(packet, packetLength); //-1 means NACK, 0 means ACK
}
///************************************************************************
//* 函数名称：
//* 函数说明：功能：复位CPU1引导CPU2  RESET_CPU1_BOOT_CPU2
//* 功能说明：
//* 输入参数：
//* 返回参数：
//*************************************************************************/
int SerialFlash::function_RESET_CPU1_BOOT_CPU2()
{
	//检查核心文件(不下载核心文件时禁用)
	//if (!g_pszKernelFile2)
	//{
	//	cout << "ERROR: No CPU2 flash kernel provided!" << endl;
	//	return(3);
	//}
	//检查CPU1是否在线
	if (cpu1 == false)
	{
		cout << "ERROR:引导CPU2并获得SCI控制后，无法在CPU1上执行操作." << endl;
	    return(2);
	}
	/*发送命令包*/
	uint8_t* packet = (uint8_t*)malloc(100);
	uint32_t packetLength;
	int iRetCode;
	packetLength = constructPacket(packet, (uint16_t)RESET_CPU1_BOOT_CPU2, 0, NULL);
	_tprintf(_T("\n发送RESET_CPU1_BOOT_CPU2命令.\n"));
	iRetCode = f021_SendPacket(packet, packetLength); //-1 means NACK, 0 means ACK
	free(packet);

	Sleep(1000);

    /*下载核心(不使用SCI引导则禁用)*/
	//Send Kernel
	//_tprintf(_T("\ncalling f021_DownloadKernel CPU2 Kernel\n"));
	//iRetCode = f021_DownloadKernel(g_pszKernelFile2);

	//自动波特率锁定
	VERBOSEPRINT(_T("\n怎在执行波特率锁定."));
	autobaudLock();

	//更换CPU在线状态
	cpu2 = true;
	cpu1 = false;
	return 0;
}
///************************************************************************
//* 函数名称：
//* 函数说明：功能：运行CPU1引导CPU2  RUN_CPU1_BOOT_CPU2
//* 功能说明：
//* 输入参数：
//* 返回参数：
//*************************************************************************/
int SerialFlash::function_RUN_CPU1_BOOT_CPU2()
{
	//if (!g_pszKernelFile2)
	//{
	//	cout << "ERROR: No CPU2 flash kernel provided!" << endl;
	//	return 3;
	//}
	if (cpu1 == false)
	{
		cout << "ERROR: Cannot perform operations on CPU1 after CPU2 is booted and given control of SCI!" << endl;
		return 2;
	}
	/*发送命令包*/
	uint8_t* packet = (uint8_t*)malloc(100);
	uint32_t packetLength;
	int iRetCode;
	packetLength = constructPacket(packet, (uint16_t)RUN_CPU1_BOOT_CPU2, 4, (uint8_t*)&gu32_branchAddress);
	_tprintf(_T("\ncalling f021_SendPacket\n"));
	iRetCode = f021_SendPacket(packet, packetLength); //-1 means NACK, 0 means ACK

	Sleep(1000);

	//no acknowledge packet
	//Send Kernel
	//_tprintf(_T("\ncalling f021_DownloadKernel CPU2 Kernel\n"));
	//iRetCode = f021_DownloadKernel(g_pszKernelFile2);

	// added for delay (diff repo/curr copy).
	Sleep(6);


	// Do AutoBaud
	VERBOSEPRINT(_T("\nAttempting autobaud to send function message..."));
	autobaudLock();

	cpu2 = true;
	cpu1 = false;
}
/************************************************************************
* 函数名称：
* 函数说明：功能:擦除CPU1指定FLASH扇区(ERASE_CPU1)
* 功能说明：
* 输入参数：
* 返回参数：
*************************************************************************/
int SerialFlash::function_ERASE_CPU1()
{
	gu32_EraseSectors1 = 0;
	//检查CPU1在线
	if (cpu1 == false)
	{
		cout << "ERROR: 引导CPU2并获得SCI控制后，无法在CPU1上执行操作！" << endl;
		return(3);
	}
	_tprintf(_T("Please input which Sectors (Letter) you want to erase for CPU1. Type '0' when finished.\n"));
	_tprintf(_T("To Erase all of the Sectors type \"ALL\".\n"));
	_tprintf(_T("First Sector: "));
	//cin >> sector;
	//while (sector.compare("0") && sector.compare(0, 3, "ALL"))
	//{
	//	setEraseSector(1U, sector[0]);
	//	_tprintf(_T("Next Sector:  "));
	//	cin >> sector;
	//}
	//if (sector == "ALL")
	//{
	//	gu32_EraseSectors1 = 0xFFFFFFFF & gu32_SectorMask;
	//}
	//发送命令
	uint8_t* packet = (uint8_t*)malloc(100);
	uint32_t packetLength;
	int iRetCode;
	packetLength = constructPacket(packet, ERASE_CPU1, 4, (uint8_t*)&gu32_EraseSectors1);
	_tprintf(_T("\ncalling f021_SendPacket\n"));
	iRetCode = f021_SendPacket(packet, packetLength); //-1 means NACK, 0 means ACK	
	free(packet);

	//获取命令
	uint16_t command = 0;
	uint16_t data[10];
	uint16_t length = 0;
	command = getPacket(&length, data);
	if (command != ERASE_CPU1)
	{
		cout << "ERROR with Packet Command!." << endl;
	}
	if (data[0] != NO_ERROR)
	{
		printErrorStatus(data[0]);
		// format mem addr output.
		cout << "ERROR Address: 0x" << hex << formatMemAddr(data[2], data[1]) << endl;
	}
}

/************************************************************************
* 函数名称：
* 函数说明：输出工作进度
* 功能说明：
* 输入参数：
* 返回参数：
*************************************************************************/
void SerialFlash::emitWorkScheduleSignal(int num)
{
	emit workScheduleSignal(num);
}
/************************************************************************
* 函数名称：
* 函数说明：信息打印
* 功能说明：
* 输入参数：
* 返回参数：
*************************************************************************/
void SerialFlash::printfMessage(QString str)
{
	emit workMessageSignal(str);
}
/************************************************************************
* 函数名称：
* 函数说明：工作函数(槽函数)
* 功能说明：
* 输入参数：
* 返回参数：
*************************************************************************/
void SerialFlash::dowork(int num)
{
	//首次需进行波特率锁定
	if (g_baudLock == false)
	{
		QTMESSAGE(QString("执行锁定波特率."));
		autobaudLock();
		QTMESSAGE(QString("波特率锁定成功."));
		g_baudLock = true;
	}
//----------------------------------DFU_CPU1---------------------------//
	if (num == DFU_CPU1)
	{
		QTMESSAGE(QString("正在执行CPU1升级..."));
		function_DFU_CPU1();
		QTMESSAGE(QString("CPU1升级成功"));
	}
//----------------------------------RUN_CPU1--------------------------//	
	else if (num == RUN_CPU1)
	{
		QTMESSAGE(QString("正在执行运行CPU1"));
		function_RUN_CPU1();
		QTMESSAGE(QString("运行CPU1成功"));
	}
//----------------------------------DFU_CPU2-------------------------//
	else if (num == DFU_CPU2)
	{
		QTMESSAGE(QString("正在执行DFU_CPU2"));
		function_DFU_CPU2();
		QTMESSAGE(QString("成功执行DFU_CPU2"));
	}
//----------------------------------RUN_CPU2-------------------------//
	else if (num == RUN_CPU2)
	{
		QTMESSAGE(QString("正在执行RUN_CPU2"));
		function_RUN_CPU2();
		QTMESSAGE(QString("成功执行RUN_CPU2"));
	}
//-----------------------RESET_CPU1_BOOT_CPU2------------------------//	
	else if (num == RESET_CPU1_BOOT_CPU2)
	{
		QTMESSAGE(QString("正在执行RESET_CPU1_BOOT_CPU2"));
		function_RESET_CPU1_BOOT_CPU2();
		QTMESSAGE(QString("成功执行RESET_CPU1_BOOT_CPU2"));
	}
//-----------------------RUN_CPU1_BOOT_CPU2------------------------//	
	else if (num == RUN_CPU1_BOOT_CPU2)
	{
		QTMESSAGE(QString("正在执行RUN_CPU1_BOOT_CPU2"));
		function_RUN_CPU1_BOOT_CPU2();
		QTMESSAGE(QString("成功执行RUN_CPU1_BOOT_CPU2"));
	}
	//发射完成信号
	emit workFinishSignal(num);
}