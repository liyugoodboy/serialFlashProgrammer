#pragma once

#include <QtWidgets/QWidget>
#include <qprocess.h>
#include <qthread.h>
#include <qtimer.h>
#include "./flashProgramer/include/serialFlash.h"
#include "ui_serialFlashProgrammer.h"



#define PRG_FILE_NAME      "program.txt"
#define PRG_FILE1_NAME     "program1.txt"
#define APP_ERTER_ADDRESS  "84000"         //APP程序入口地址  
#define MAX_BLACK_NUMBER   100             //允许APP数据块的最大数量

//消息类型
#define MSG_TEXT           0x01            //文本
#define MSG_CMD            0x02            //命令
#define MSG_ERROR          0x04            //错误
#define MSG_WARNING        0x08            //警告
#define MSG_WITH_TIME      0x10            //带时间信息

//消息文本
#define WELCOME_TEXT  "*******************  C2000串口升级程序  *******************"

class serialFlashProgrammer : public QWidget
{
    Q_OBJECT
        //appFileInfo结构体
        typedef struct APPFILEINFO {
        QString   appFileDir;      //路径
        QString   appFileName;     //文件名称
        quint32   appEnterAddress; //入口地址
        quint32   allBlockNumber;  //块总数量
        quint32   allBlockSize;    //块总大小
        float     estimatedTime;   //预计下载用时
    }appFileInfo;
public:
    serialFlashProgrammer(QWidget *parent = Q_NULLPTR);
    ~serialFlashProgrammer();
    void programToSci8(QString path,QString programName);
    void appFileMessage(QString name, appFileInfo* info);
    void outputMessage(QString str,qint16 type = MSG_TEXT | MSG_WITH_TIME);
    void scanComPort();
    bool nativeEvent(const QByteArray& eventType, void* message, long* result);
private slots:
    void on_btn_selectProgramFile_clicked();//选择程序文件按钮单击槽
    void on_btn_selectProgramFile1_clicked();//选择程序文件1按钮单击槽
    void on_btn_portSearch_clicked();//端口搜索按钮单击槽
    void on_btn_openPart_clicked();//打开串口按钮单击槽
    void vn_cbox_device_activated(QString);//设备类型选择框改变槽
    void vn_btn_execute_clicked();//执行按钮单击槽
    void vn_lineEdit_enterAddr_editingFinished();//地址输入框输入数据
    void vn_checkBox_bandStatus_stateChange(int);//波特率可配置选择框
    void vn_btn_cleanMessage_clicked();//清除输出消息按钮
    void vn_btn_viewProgram_clicked();//查看程序信息按钮
private slots:
    void workThreadStart();//线程启动槽函数
    void workThreadFinish();//线程结束槽函数
    void workFinish(int);//功能完成信号槽
    void workMessage(QString);//功能消息信号槽
    void workSchedule(int);//功能完成进度信号槽
    void timeoutTimerUpdate();//超时定时器溢出
signals:
    //给线程发送信号，信号携带功能码
    void sendFunctionNumToThread(int code);

private:
    void closeEvent(QCloseEvent*);//窗口关闭

private:
    Ui::serialFlashProgrammerClass ui;
    QProcess hex2000;

private:

    bool m_functionStatus;//功能执行状态 

    wchar_t m_appName[20];//应用程序名称
    appFileInfo m_appCpu1FileInfo;//CPU1程序文件信息
    wchar_t m_appName1[20];//应用程序名称
    appFileInfo m_appCpu2FileInfo;//CPU2程序文件信息
    uint32_t m_branchAddress;//应用程序入口地址

    int m_workProgress;//功能执行进度

    wchar_t m_comPort[20];//串口号(类型兼容底层驱动)
    wchar_t m_baudRate[20];//波特率
    bool m_autoBand;//自动波特率
    bool m_comStatus;//端口状态

    QThread workerThread;//创建工作线程
    SerialFlash serialFlash;//创建serialFlash对象
    QTimer timeoutTimer;//超时定时器
    uint m_timeCount;//超时时间
};
