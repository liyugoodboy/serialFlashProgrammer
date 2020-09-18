#ifndef SERIAL_FLASH_H_
#define SERIAL_FLASH_H_
#include <qobject.h>
#include <QtWidgets/QWidget>

/************************************变量声明************************************/
extern bool g_bVerbose;
extern bool g_bQuiet;
extern bool g_bOverwrite;
extern bool g_bUpload;
extern bool g_bClear;
extern bool g_bBinary;
extern bool g_bWaitOnExit;
extern bool g_bReset;
extern bool g_bSwitchMode;
extern bool g_baudLock;       //波特率锁定标志
extern bool cpu1;            //CPU1在线标志
extern bool cpu2;            //CPU2在线标志
/**************************************设备号*********************************/
extern bool g_bf2802x;
extern bool g_bf2803x;
extern bool g_bf2805x;
extern bool g_bf2806x;
extern bool g_bf2833x;
extern bool g_bf2837xD;
extern bool g_bf2837xS;
extern bool g_bf2807x;
extern bool g_bf28004x;


/********************************运行功能***************************************/
extern uint32_t gu32_branchAddress;   //入口地址



extern wchar_t* g_pszAppFile;
extern wchar_t* g_pszAppFile2;
extern wchar_t* g_pszKernelFile;
extern wchar_t* g_pszKernelFile2;
extern wchar_t* g_pszComPort;
extern wchar_t* g_pszBaudRate;
extern wchar_t* g_pszDeviceName;
/*******************************功能命令*****************************************/
#define DFU_CPU1					    0x0100
#define DFU_CPU2					    0x0200
#define ERASE_CPU1					0x0300
#define ERASE_CPU2					0x0400
#define VERIFY_CPU1					0x0500
#define VERIFY_CPU2					0x0600
#define LIVE_DFU_CPU1                 0x0700
#define CPU1_UNLOCK_Z1				0x000A
#define CPU1_UNLOCK_Z2				0x000B
#define CPU2_UNLOCK_Z1				0x000C
#define CPU2_UNLOCK_Z2				0x000D
#define RUN_CPU1					    0x000E
#define RESET_CPU1					0x000F
#define RUN_CPU1_BOOT_CPU2			    0x0004
#define RESET_CPU1_BOOT_CPU2          0x0007
#define RUN_CPU2					    0x0010
#define RESET_CPU2					0x0020
/*******************************返回错误码*****************************************/
#define NO_ERROR					    0x1000
#define BLANK_ERROR					0x2000
#define VERIFY_ERROR				    0x3000
#define PROGRAM_ERROR				    0x4000
#define COMMAND_ERROR				    0x5000
#define UNLOCK_ERROR				    0x6000
/************************************函数声明************************************/
extern int openCom();
extern int closeCom();


/********************************建立类线程使用**********************************/
class SerialFlash : public QObject
{
	Q_OBJECT
public:
	SerialFlash();
	~SerialFlash();
	int function_DFU_CPU1();//升级CPU1功能
	int function_RUN_CPU1();
	int function_DFU_CPU2();
	int function_RUN_CPU2();
	int function_ERASE_CPU1();
	int function_RESET_CPU1_BOOT_CPU2();
	int function_RUN_CPU1_BOOT_CPU2();

	void emitWorkScheduleSignal(int);//发射下载进度信号
	void printfMessage(QString);//打印信息函数

public slots:
	void dowork(int);

signals:
	//工作完成信号：功能码
	void workFinishSignal(int);
	//信息输出信号：调试信息
	void workMessageSignal(QString);
	//工作进度信号：下载进度
	void workScheduleSignal(int);
};

extern SerialFlash* pSerialFlash;

#endif // !SERIAL_FLASH_H_

