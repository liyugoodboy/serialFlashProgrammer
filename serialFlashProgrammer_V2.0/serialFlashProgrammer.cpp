/************************************************************************
* 文件名称：
* 文件说明：
* 功能说明：
*           1.固定APP程序入口地址：0x84000
*     作者：
* 完成时间：
* 修改记录：
*************************************************************************/
#include "serialFlashProgrammer.h"
#include "./flashProgramer/include/serialFlash.h"
#include <qdebug.h>
#include <qfiledialog.h>
#include <QtSerialPort/qserialport.h>
#include <QtSerialPort/qserialportinfo.h>
#include <qdatetime.h>
#include <qtoolbutton.h>
#include <qmessagebox.h>
#include <qevent.h>
#include <qmenu.h>
#include <qaction.h>
#include <iostream>
#include <qtoolbar.h>
#include <windows.h>

//解决中文乱码
#if _MSC_VER >= 1600	
#pragma execution_character_set("utf-8")
#endif

//增加标题栏右键菜单关于项
#pragma comment(lib, "User32.lib")
#define IDM_ABOUTBOX 0x0010

/************************************************************************
* 函数名称：serialFlashProgrammer
* 函数说明：构造函数
* 功能说明：
* 输入参数：
* 返回参数：
*************************************************************************/
serialFlashProgrammer::serialFlashProgrammer(QWidget* parent)
	: QWidget(parent)
{
	//界面绘制
	ui.setupUi(this);
	//变量初始化

	//入口地址输入框初始化
	ui.enterAddrLineEdit->setText(APP_ERTER_ADDRESS);
	ui.enterAddrLineEdit->setEnabled(false);
	//隐藏进度条
	ui.progressBar->setMinimum(0);
	ui.progressBar->setMaximum(100);
	ui.progressBar->setValue(0);
	ui.progressBar->hide();
	//核心下拉框
	ui.cbox_core->addItem("F021");
	//设备下拉框
	ui.cbox_device->addItem("F2837xD");
	ui.cbox_device->addItem("F2837xS");
	ui.cbox_device->addItem("F2807x");
	ui.cbox_device->addItem("F28004x");
	//F021内核，F2827xD功能下拉框项配置
	ui.cbox_function->clear();
	ui.cbox_function->addItem("升级CPU1", QVariant(DFU_CPU1));
	ui.cbox_function->addItem("升级CPU2", QVariant(DFU_CPU2));
	ui.cbox_function->addItem("擦除CPU1", QVariant(ERASE_CPU1));
	ui.cbox_function->addItem("擦除CPU2", QVariant(ERASE_CPU2));
	ui.cbox_function->addItem("验证CPU1", QVariant(VERIFY_CPU1));
	ui.cbox_function->addItem("验证CPU2", QVariant(VERIFY_CPU2));
	ui.cbox_function->addItem("解锁CPU1区域1", QVariant(CPU1_UNLOCK_Z1));
	ui.cbox_function->addItem("解锁CPU1区域2", QVariant(CPU1_UNLOCK_Z2));
	ui.cbox_function->addItem("解锁CPU2区域1", QVariant(CPU2_UNLOCK_Z1));
	ui.cbox_function->addItem("解锁CPU2区域2", QVariant(CPU2_UNLOCK_Z2));
	ui.cbox_function->addItem("运行CPU1", QVariant(RUN_CPU1));
	ui.cbox_function->addItem("重置CPU1", QVariant(RESET_CPU1));
	ui.cbox_function->addItem("运行CPU1引导CPU2", QVariant(RUN_CPU1_BOOT_CPU2));
	ui.cbox_function->addItem("重置CPU1引导CPU2", QVariant(RESET_CPU1_BOOT_CPU2));
	ui.cbox_function->addItem("运行CPU2", QVariant(RUN_CPU2));
	ui.cbox_function->addItem("重置CPU2", QVariant(RESET_CPU2));
	//输出欢迎信息
	outputMessage(WELCOME_TEXT, MSG_TEXT);
	//扫描端口信息显示到界面上
	scanComPort();
	//设置波特率
	ui.cbox_baud->clear();
	ui.cbox_baud->addItem("1200");
	ui.cbox_baud->addItem("2400");
	ui.cbox_baud->addItem("4800");
	ui.cbox_baud->addItem("9600");
	ui.cbox_baud->addItem("19200");
	ui.cbox_baud->addItem("38400");
	ui.cbox_baud->addItem("57600");
	ui.cbox_baud->addItem("115200");
	//可选框默认选中
	m_autoBand = true;
	ui.bandSelectCheckBox->setChecked(true);
	//设置地址框输入限制
	QRegExp regExp("[a-fA-F0-9]{6}");
	ui.enterAddrLineEdit->setValidator(new QRegExpValidator(regExp, this));
    //初始化超时时间（10s）
	m_timeCount = 10000;
	//连接超时定时器信号
	connect(&timeoutTimer, SIGNAL(timeout()), this, SLOT(timeoutTimerUpdate()));
	//线程处理
	serialFlash.moveToThread(&workerThread);//把下载功能移到工作线程中
	//连接相关信号
	connect(this, SIGNAL(sendFunctionNumToThread(int)), &serialFlash, SLOT(dowork(int)));//工作
	connect(&serialFlash, SIGNAL(workFinishSignal(int)), this, SLOT(workFinish(int)));//工作完成
	connect(&serialFlash, SIGNAL(workMessageSignal(QString)), this, SLOT(workMessage(QString)));//信息输出
	connect(&serialFlash, SIGNAL(workScheduleSignal(int)), this, SLOT(workSchedule(int)));//进度
	connect(&workerThread, SIGNAL(started()), this, SLOT(workThreadStart()));//开始
	connect(&workerThread, SIGNAL(finished()), this, SLOT(workThreadFinish()));//结束
	//开启线程
	workerThread.start();
	//设置窗口标题栏右键菜单
	HMENU sysMenu = GetSystemMenu((HWND)winId(), FALSE);
	if (sysMenu != NULL)
	{
		QString str = "About(&A)";
		AppendMenuA(sysMenu, MF_SEPARATOR, 0, 0);
		AppendMenuA(sysMenu, MF_STRING, IDM_ABOUTBOX, str.toStdString().c_str());		
	}
	//兼容C语言
	pSerialFlash = &serialFlash;
}
/************************************************************************
* 函数名称：~serialFlashProgrammer
* 函数说明：析构函数
* 功能说明：
* 输入参数：
* 返回参数：
*************************************************************************/
serialFlashProgrammer::~serialFlashProgrammer()
{

}
/************************************************************************
* 函数名称：
* 函数说明：
* 功能说明：
* 输入参数：
* 返回参数：
*************************************************************************/
bool serialFlashProgrammer::nativeEvent(const QByteArray& eventType, void* message, long* result)
{
	Q_UNUSED(eventType);
    MSG* msg = reinterpret_cast<MSG*>(message);
	if (msg->message == WM_SYSCOMMAND)
	{
		if ((msg->wParam & 0xfff0) == IDM_ABOUTBOX)
		{
			*result = 0;
			//打开自定义关于对话框
			QMessageBox::about(this, "关于", "作者:liyu\r\n时间:2020.9.3");
			return (true);
		}
	}
	return (false);
}
/************************************************************************
* 函数名称：on_btn_selectProgramFile_clicked()
* 函数说明：选择CPU1程序按钮->单击信号->槽函数
* 功能说明：显示文件路径到lineEdit中,并转换文件成SCI8格式
* 输入参数：
* 返回参数：
*************************************************************************/
void serialFlashProgrammer::on_btn_selectProgramFile_clicked()
{

	QString digTile = "选择程序文件";//标题
	QString curPath = QDir::currentPath();//获取系统当前目录
	QString filter = "程序文件(*.out)";//文本过滤器
	//打开文件对话框
	QString aFileName = QFileDialog::getOpenFileName(this, digTile, curPath, filter);
	//返回的文件名不为空
	if (!aFileName.isEmpty())
	{
		//路径显示到text框中
		ui.lineEdit->setText(aFileName);
		//保存路径和名称
		int first = aFileName.lastIndexOf("/");
		QString name = aFileName.right(aFileName.length() - first - 1);
		QString dir = aFileName.left(aFileName.length() - first);
		m_appCpu1FileInfo.appFileDir = dir;
		m_appCpu1FileInfo.appFileName = name;
		//转换文件
		programToSci8(aFileName, PRG_FILE_NAME);
		//输出文件信息
		appFileMessage(PRG_FILE_NAME, &m_appCpu1FileInfo);
		//设置转换的文件名
		memset(m_appName, 0, sizeof(m_appName));
		QString(PRG_FILE_NAME).toWCharArray(m_appName);

	}
}
/************************************************************************
* 函数名称：on_btn_selectProgramFile1_clicked()
* 函数说明：选择CPU2程序按钮->单击信号->槽函数
* 功能说明：显示文件路径到lineEdit中,并转换文件成SCI8格式
* 输入参数：
* 返回参数：
*************************************************************************/
void serialFlashProgrammer::on_btn_selectProgramFile1_clicked()
{
	QString digTile = "选择程序文件";//标题
	QString curPath = QDir::currentPath();//获取系统当前目录
	QString filter = "程序文件(*.out)";//文本过滤器
	//打开文件对话框
	QString aFileName = QFileDialog::getOpenFileName(this, digTile, curPath, filter);
	//返回的文件名不为空
	if (!aFileName.isEmpty())
	{
		//路径显示到text框中
		ui.lineEditPrg1FilePath->setText(aFileName);
		//转换文件
		programToSci8(aFileName, PRG_FILE1_NAME);
		//输出文件信息
		appFileMessage(PRG_FILE1_NAME, &m_appCpu2FileInfo);
		//设置文件名
		memset(m_appName1, 0, sizeof(m_appName1));
		QString(PRG_FILE1_NAME).toWCharArray(m_appName1);
	}
}
/************************************************************************
* 函数名称：on_btn_portSearch_clicked()
* 函数说明：端口搜索按钮->单击信号->槽函数
* 功能说明：
* 输入参数：
* 返回参数：
*************************************************************************/
void serialFlashProgrammer::on_btn_portSearch_clicked()
{
	//扫描全部端口信息并显示在下拉框中
	scanComPort();
}
/************************************************************************
* 函数名称：on_btn_openPart_clicked()
* 函数说明：打开串口按钮->单击信号->槽函数
* 功能说明：
* 输入参数：
* 返回参数：
*************************************************************************/
void serialFlashProgrammer::on_btn_openPart_clicked()
{
	//判断按键状态
	if (ui.btn_openPart->text() == "打开串口")
	{
		//检查端口选择下拉框有没有设备项
		if (ui.cbox_port->count() == 0)
		{
			outputMessage(QString("开启失败，未选择正确串口"),MSG_ERROR |MSG_WITH_TIME);
			return;
		}
		//设置端口号
		memset(m_comPort, 0, sizeof(m_comPort));
		ui.cbox_port->currentText().toWCharArray(m_comPort);
		g_pszComPort = m_comPort;
		//设置波特率
		memset(m_baudRate, 0, sizeof(m_baudRate));
		//判断波特率检测标志
		if (m_autoBand == true)
		{
			ui.cbox_baud->currentText().toWCharArray(m_baudRate);
		}
		else
		{
			//固定波特率
			QString("115200").toWCharArray(m_baudRate);
		}
		g_pszBaudRate = m_baudRate;
		//开启端口
		int ret = openCom();

		//判断是否开启成功
		if (ret == 0)
		{
			outputMessage(QString("串口开启失败"), MSG_ERROR | MSG_WITH_TIME);
		}
		else
		{
			outputMessage(QString("串口开启成功"), MSG_TEXT | MSG_WITH_TIME);
			ui.btn_openPart->setText("关闭串口");
			//禁止端口及波特率选择以及扫描
			ui.cbox_port->setEnabled(false);
			ui.cbox_baud->setEnabled(false);
			ui.btn_portSearch->setEnabled(false);
			m_comStatus = true;//改变状态
			if (m_autoBand == false)
			{
				g_baudLock = true;//关闭波特率检测
			}
			
		}
	}
    //关闭串口
	else if (ui.btn_openPart->text() == "关闭串口")
	{
		//关闭端口
		int rtn = closeCom();
	    //判断是否关闭成功
		if (rtn == 0)
		{
			outputMessage(QString("串口关闭失败"), MSG_ERROR | MSG_WITH_TIME);
		}
		else
		{
			outputMessage(QString("串口关闭成功"));
			ui.btn_openPart->setText("打开串口"); 
			//开启端口及波特率选择以及扫描
			ui.cbox_port->setEnabled(true);
			ui.cbox_baud->setEnabled(true);
			ui.btn_portSearch->setEnabled(true);
			m_comStatus = false;//改变状态
		}
	}
}
/************************************************************************
* 函数名称：vn_cbox_device_activated
* 函数说明：设备选择改变槽函数
* 功能说明：
* 输入参数：
* 返回参数：
*************************************************************************/
void serialFlashProgrammer::vn_cbox_device_activated(QString str)
{
	//由设备决定功能
	if (str == "F2837xD")
	{
		//F021内核，F2827xD功能下拉框项配置
		ui.cbox_function->clear();
		ui.cbox_function->addItem("升级CPU1", QVariant(DFU_CPU1));
		ui.cbox_function->addItem("升级CPU2", QVariant(DFU_CPU2));
		ui.cbox_function->addItem("擦除CPU1", QVariant(ERASE_CPU1));
		ui.cbox_function->addItem("擦除CPU2", QVariant(ERASE_CPU2));
		ui.cbox_function->addItem("验证CPU1", QVariant(VERIFY_CPU1));
		ui.cbox_function->addItem("验证CPU2", QVariant(VERIFY_CPU2));
		ui.cbox_function->addItem("解锁CPU1区域1", QVariant(CPU1_UNLOCK_Z1));
		ui.cbox_function->addItem("解锁CPU1区域2", QVariant(CPU1_UNLOCK_Z2));
		ui.cbox_function->addItem("解锁CPU2区域1", QVariant(CPU2_UNLOCK_Z1));
		ui.cbox_function->addItem("解锁CPU2区域2", QVariant(CPU2_UNLOCK_Z2));
		ui.cbox_function->addItem("运行CPU1", QVariant(RUN_CPU1));
		ui.cbox_function->addItem("重置CPU1", QVariant(RESET_CPU1));
		ui.cbox_function->addItem("运行CPU1引导CPU2", QVariant(RUN_CPU1_BOOT_CPU2));
		ui.cbox_function->addItem("重置CPU1引导CPU2", QVariant(RESET_CPU1_BOOT_CPU2));
		ui.cbox_function->addItem("运行CPU2", QVariant(RUN_CPU2));
		ui.cbox_function->addItem("重置CPU2", QVariant(RESET_CPU2));
	}
	else
	{
		//F021内核，F2807x，F2837xS，F28004x设备功能下拉框配置
		ui.cbox_function->clear();
		ui.cbox_function->addItem("升级CPU", QVariant(DFU_CPU1));
		ui.cbox_function->addItem("擦除CPU", QVariant(ERASE_CPU1));
		ui.cbox_function->addItem("验证CPU", QVariant(VERIFY_CPU1));
		ui.cbox_function->addItem("解锁CPU区域1", QVariant(CPU1_UNLOCK_Z1));
		ui.cbox_function->addItem("解锁CPU区域2", QVariant(CPU1_UNLOCK_Z2));
		ui.cbox_function->addItem("运行CPU", QVariant(RUN_CPU1));
		ui.cbox_function->addItem("复位CPU", QVariant(RESET_CPU1));
	}

}
/************************************************************************
* 函数名称：vn_lineEdit_enterAddr_editingFinished
* 函数说明：入口地址输入框->返回或回车->槽
* 功能说明：
* 输入参数：
* 返回参数：
*************************************************************************/
void serialFlashProgrammer::vn_lineEdit_enterAddr_editingFinished()
{
	////获取文本
	//QString str = ui.enterAddrLineEdit->text();
	//qDebug() << str << endl;
	////转换成32位数据
	//qDebug() << str.toUInt(nullptr,16) << endl;
}
/************************************************************************
* 函数名称：timeoutTimerUpdate
* 函数说明：超时定时器超时信号槽
* 功能说明：
* 输入参数：
* 返回参数：
*************************************************************************/
void serialFlashProgrammer::timeoutTimerUpdate()
{
	//关闭定时器
	timeoutTimer.stop();
	//消息框显示已超时，是否强制关闭线程
	QMessageBox::StandardButton rtn;
	QMessageBox::StandardButtons buttons = QMessageBox::Yes | QMessageBox::No;
	QMessageBox::StandardButton defbutton = QMessageBox::Yes;
	QString txtLable = "执行功能超时，是否强制重启线程";
	rtn = QMessageBox::warning(this, "警告", txtLable, buttons, defbutton);//警告
	//确认强制重启线程
	if (rtn == QMessageBox::Yes)
	{
		outputMessage(QString("正在尝试关闭线程"));
		workerThread.terminate();
		outputMessage(QString("等待线程关闭"));
		workerThread.wait();
		outputMessage(QString("重启中"));
		workerThread.start();
		//清除功能执行标志
		m_functionStatus = false;
		//启动执行按钮
		ui.btn_execute->setText("执行");
		ui.btn_execute->setEnabled(true);
		//启用关闭串口按钮
		ui.btn_openPart->setEnabled(true);
		//隐藏进度条
		ui.progressBar->hide();
	}
	else
	{
		//重新开启定时器
		timeoutTimer.start(m_timeCount);
	}
}
/************************************************************************
* 函数名称：vn_checkBox_bandStatus_stateChange
* 函数说明：波特率状态选择框->状态改变->槽
* 功能说明：
* 输入参数：
* 返回参数：
*************************************************************************/
void serialFlashProgrammer::vn_checkBox_bandStatus_stateChange(int state)
{
	//选中
	if (state == Qt::Checked)
	{
		//波特率选择框使能
		ui.cbox_baud->setEnabled(true);
		m_autoBand = true;
	}
	//未选中
	else if(state == Qt::Unchecked)
	{
		ui.cbox_baud->setCurrentIndex(ui.cbox_baud->findText("115200"));
		//波特率选择框失能
		ui.cbox_baud->setEnabled(false);
		m_autoBand = false;
	}
}
/************************************************************************
* 函数名称：vn_btn_cleanMessage_clicked
* 函数说明：清除消息按钮->单击->槽
* 功能说明：
* 输入参数：
* 返回参数：
*************************************************************************/
void serialFlashProgrammer::vn_btn_cleanMessage_clicked()
{

}
/************************************************************************
* 函数名称：vn_btn_execute_clicked()
* 函数说明：执行按钮->单击->槽
* 功能说明：
* 输入参数：
* 返回参数：
*************************************************************************/
void serialFlashProgrammer::vn_btn_execute_clicked()
{
	//检查按钮状态
	if (ui.btn_execute->text() == "执行")
	{
		//检查串口是否打开
		if (m_comStatus == false)
		{
			outputMessage(QString("串口未打开，功能未执行"), MSG_ERROR | MSG_WITH_TIME);
			return;
		}
		//检查设备
		if (ui.cbox_device->currentText() == "F2837xD")
		{
			cpu1 = true;
			g_bf2837xD = true;
		}
		
		//检查升级线程是否在运行
		if (workerThread.isRunning() == false)
		{
			outputMessage(QString("升级线程未运行，请重启软件"), MSG_ERROR | MSG_WITH_TIME);
			return;
		}
		//获取功能码
		int command = ui.cbox_function->currentData().toInt();
		//传递文件信息
		if (command == DFU_CPU1)
		{
			g_pszAppFile = m_appName;
		}
		if (command == DFU_CPU2)
		{
			g_pszAppFile2 = m_appName1;
		}
		//设置程序入口地址
		if (command == RUN_CPU1)
		{
			//获取文本
			//QString str = ui.enterAddrLineEdit->text();
			QString str = APP_ERTER_ADDRESS;
			//按16进制转换成32位数据
			m_branchAddress = str.toUInt(nullptr, 16);
			gu32_branchAddress = m_branchAddress;
		}
		//发射对应功能码信号
		emit sendFunctionNumToThread(ui.cbox_function->currentData().toInt());
		//执行按钮修改为强制关闭按钮
		ui.btn_execute->setText("强制关闭");
		//禁用关闭串口按钮
		ui.btn_openPart->setEnabled(false);
		//显示进度条
		ui.progressBar->show();
		//开启超时定时器
		outputMessage(QString("超时定时器启动"));
		timeoutTimer.start(m_timeCount);
		//进入功能执行状态
		m_functionStatus = true;
	}
	else if (ui.btn_execute->text() == "强制关闭")
	{
		ui.btn_execute->setEnabled(false);
		outputMessage(QString("正在尝试关闭线程"));
		workerThread.terminate();
		outputMessage(QString("等待线程关闭"));
		workerThread.wait();
		outputMessage(QString("重启中"));
		workerThread.start();
		//清除功能执行标志
		m_functionStatus = false;
		//启动执行按钮
		ui.btn_execute->setText("执行");
		ui.btn_execute->setEnabled(true);
		//启用关闭串口按钮
		ui.btn_openPart->setEnabled(true);
		//隐藏进度条
		ui.progressBar->hide();
        //关闭超时定时器
		timeoutTimer.stop();
	}
}
/************************************************************************
* 函数名称：
* 函数说明：文件转换函数
* 功能说明：把传入文件名的文件转换成UTF8格式文件(文件名默认为program.txt)
*           hex2000 -boot -a -sci8 <.out文件名> -o <输出文件名>
* 输入参数：
*           QString   path          文件名(包含路径)
*           QString   programName   输出文件名(带后缀)
* 返回参数：
*************************************************************************/
void serialFlashProgrammer::programToSci8(QString path,QString programName)
{
	//程序设置
	QString program = "hex2000.exe";
	//参数设置
	QStringList arguments;
	arguments << "-boot" << "-a" << "-sci8" << path << "-o" << programName;
	//运行程序
	hex2000.start(program, arguments);
	//等待完成
	hex2000.waitForFinished();
	//输出信息
	outputMessage(QString("文件转换成功"));
}
/************************************************************************
* 函数名称：
* 函数说明：转换程序校验及信息输出
* 功能说明：
* 输入参数：
*           QString       name      转换完成的文件名称
*           appFileInfo*  fileInfo  转换前文件信息指针
* 返回参数：
*************************************************************************/
void serialFlashProgrammer::appFileMessage(QString name,appFileInfo* info)
{
	/*获取转换文件信息(解析文件结构)*/
	//块地址及大小
	unsigned int blackNumber = 0;
	unsigned int allBlackSize = 0;
	unsigned int blackSize = 0;
	unsigned int blackAddr = 0;
    //打开文件
	QFile programFile(name);
	if (!programFile.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		std::cout << programFile.errorString().toStdString() << endl;
	}
	//设置文本流
	QTextStream in(&programFile);
	/*获取入口地址*/
	in.seek(57);//入口地址偏移量
	unsigned int array[4];
	QString str;
	bool ok;
	int i;
	for ( i = 0; i < 4; i++)
	{
		in >> str;
		array[i] = str.toUInt(&ok, 16);
	}
	array[0] = array[0] << 16;
	array[1] = array[1] << 24;
	array[3] = array[3] << 8;
	unsigned int addr = array[0] + array[1] + array[2] + array[3];
	qDebug() << hex << showbase << addr << endl;

	//获取所有块信息
	while (blackNumber < MAX_BLACK_NUMBER)
	{
		//获取块大小信息
		for (i = 0; i < 2; i++)
		{
			in >> str;
			array[i] = str.toUInt(&ok, 16);
		}
		array[1] = array[1] << 8;
		blackSize = array[0] + array[1];
		//如果块大小为0则直接退出
		if (blackSize == 0) { break; }
		allBlackSize += blackSize;
		//获取块地址
		for (i = 0; i < 4; i++)
		{
			in >> str;
			array[i] = str.toUInt(&ok, 16);
		}
		array[0] = array[0] << 16;
		array[1] = array[1] << 24;
		array[3] = array[3] << 8;
		blackAddr = array[0] + array[1] + array[2] + array[3];
		//数据地址与booltloader2程序段冲突
		if (blackAddr < 0x84000 && blackAddr > 0x80000)
		{
			outputMessage("应用程序存储分配和bootloader存储区冲突，无法进行升级", MSG_ERROR | MSG_WITH_TIME);
		}
		//数据块在RAM区
		if (blackAddr < 0x80000)
		{
			outputMessage("应用程序数据在RAM区，RAM部分不会被下载，请检查应用程序", MSG_ERROR | MSG_WITH_TIME);
		}
		//块信息后位置
		qDebug() << in.pos();
		//偏移到下一个块
		blackNumber++;
		//TODO:此处无法通过计算位置偏移加速读取信息，因为位置偏移量无法计算。
		for (i = 0; i < (blackSize * 2); i++)
		{
			in >> str;
		}
		//信息输出
		qDebug() << "black addr:" << hex << showbase << blackAddr << endl;
		qDebug() << "blackSize:" << blackSize << endl;
	} 
	//判断是否到达最大块数量
	if (blackNumber > MAX_BLACK_NUMBER)
	{
		outputMessage(QString("程序信息中块数据超过可读最大值，信息读取失败"), MSG_ERROR|MSG_WITH_TIME);
	}
	//保存数据
	info->appEnterAddress = addr;
	info->allBlockNumber = blackNumber;
	info->allBlockSize = allBlackSize;
	info->estimatedTime = (float)allBlackSize * 100 / 115200;
	//输出显示信息
	outputMessage(QString("******************程序信息*****************"),MSG_TEXT);
	outputMessage(QString("程序路径：") + info->appFileDir, MSG_TEXT);
	outputMessage(QString("程序名称：") + info->appFileName, MSG_TEXT);
	outputMessage(QString("_c_int00:0x%1").arg(addr, 4, 16, QLatin1Char('0')), MSG_TEXT);
	outputMessage(QString("块数量:%1").arg(blackNumber), MSG_TEXT);
	outputMessage(QString("数据总大小:%1").arg(allBlackSize), MSG_TEXT);
	outputMessage(QString("预计下载用时:%1s").arg(info->estimatedTime,3), MSG_TEXT);
	outputMessage(QString("*******************************************"), MSG_TEXT);
	programFile.close();
}
/************************************************************************
* 函数名称：
* 函数说明：扫描COM口
* 功能说明：
* 输入参数：
* 返回参数：
*************************************************************************/
void serialFlashProgrammer::scanComPort()
{
	outputMessage(QString("扫描端口设备"),MSG_CMD | MSG_WITH_TIME);
	ui.cbox_port->clear();
	qint16 count = 0;
	//遍历端口信息
	foreach(const QSerialPortInfo & info, QSerialPortInfo::availablePorts())
	{
		count++;
		outputMessage(QString("设备%1:").arg(count), MSG_TEXT);
		outputMessage(QString("端口:") + info.portName(), MSG_TEXT);
		outputMessage(QString("设备名称:") + info.description(), MSG_TEXT);
		ui.cbox_port->addItem(info.portName());
	}
	//未扫描到端口
	if (count == 0)
	{
		outputMessage(QString("未扫描到任何串口设备"),MSG_WARNING | MSG_WITH_TIME);
	}
}
/************************************************************************
* 函数名称：
* 函数说明：信息输出
* 功能说明：
* 输入参数：
*           qint16    type    消息类型
*           QString   str     输出信息
* 返回参数：
*************************************************************************/
void serialFlashProgrammer::outputMessage(QString str,qint16 type)
{
	QDateTime current_date_time = QDateTime::currentDateTime();
	QString current_date = current_date_time.toString("yyyy.MM.dd hh:mm:ss") + "    ";
	QString color_end = "</font>";
	QString color_start;
	bool showTime = false;

	if ((type & MSG_WITH_TIME) > 0)
	{
		showTime = true;
		type = type & (~MSG_WITH_TIME);
	}
	else
	{
		showTime = false;
		current_date.clear();
	}
	switch (type)
	{
	    case MSG_TEXT:
			color_start = "<font color=\"#000000\">";break;
	    case MSG_CMD:
			color_start = "<font color=\"#000000\">";str = ">>" + str; break;
		case MSG_WARNING:
			color_start = "<font color=\"#0000FF\">";str = "WARNING:" + str; break;

		case MSG_ERROR:
			color_start = "<font color=\"#FF0000\">";str = "ERROR:" + str; break;
	}
	std::cout << current_date.toStdString() << str.toStdString() << std::endl;
	ui.textBrowser->append(current_date + color_start + str + color_end);
}
/************************************************************************
* 函数名称：
* 函数说明：工作线程启动信号槽函数
* 功能说明：
* 输入参数：
* 返回参数：
*************************************************************************/
void serialFlashProgrammer::workThreadStart()
{
	outputMessage(QString("升级线程启动成功"));
}
/************************************************************************
* 函数名称：
* 函数说明：工作线程完成信号槽函数
* 功能说明：
* 输入参数：
* 返回参数：
*************************************************************************/
void serialFlashProgrammer::workThreadFinish()
{
	outputMessage(QString("升级线程退出成功"));
}
/************************************************************************
* 函数名称：
* 函数说明：工作线程功能完成信号槽函数
* 功能说明：
* 输入参数：
* 返回参数：
*************************************************************************/
void serialFlashProgrammer::workFinish(int num)
{
    //输出信息
	outputMessage(QString("功能完成"));
	//清除功能执行标志
	m_functionStatus = false;
	//关闭超时定时器
	timeoutTimer.stop();
	//启动执行按钮
	ui.btn_execute->setText("执行");
	ui.btn_execute->setEnabled(true);
	//启用关闭串口按钮
	ui.btn_openPart->setEnabled(true);
	//隐藏进度条
	ui.progressBar->hide();
}
/************************************************************************
* 函数名称：
* 函数说明：功能信息信号槽函数
* 功能说明：
* 输入参数：
* 返回参数：
*************************************************************************/
void serialFlashProgrammer::workMessage(QString str)
{
	outputMessage(str);
}
/************************************************************************
* 函数名称：
* 函数说明：进度信号槽函数
* 功能说明：
* 输入参数：
* 返回参数：
*************************************************************************/
void serialFlashProgrammer::workSchedule(int num)
{
	//设置进度条
	ui.progressBar->setValue(num);
	m_workProgress = num;
	std::cout << "执行进度:" << num << std::endl;
}
/************************************************************************
* 函数名称：
* 函数说明：窗口退出事件
* 功能说明：
* 输入参数：
* 返回参数：
*************************************************************************/
void serialFlashProgrammer::closeEvent(QCloseEvent* ev)
{
    //检查工作线程是否在运行
    if (workerThread.isRunning() == true)
    {
		//检查是否在执行功能
		if (m_functionStatus == true)
		{
			QMessageBox::StandardButton rtn;
			QMessageBox::StandardButtons buttons = QMessageBox::Ignore | QMessageBox::Abort;
			QMessageBox::StandardButton defbutton = QMessageBox::NoButton;
			QString txtLable = "正在执行功能，关闭程序可能会导致升级失败，强制关闭按Ignore按键,中止按Abort按键";
			rtn = QMessageBox::warning(this, "警告", txtLable, buttons, defbutton);//警告
			//忽略，退出
			if (rtn == QMessageBox::Ignore)
			{
				//改为强制使线程退出
				workerThread.terminate();
				//workerThread.exit();
				workerThread.wait();
				ev->accept();
			}
			//中止退出
			else
			{
				ev->ignore();
			}
		}
		else
		{
			//未在执行功能，但线程在运行，则让线程正常退出
			workerThread.exit();
			workerThread.wait();
		}      
    }
    else
    {
        ev->accept();
    }
}

