#include "MainWindow.h"

//解决中文乱码
#if _MSC_VER >= 1600	
#pragma execution_character_set("utf-8")
#endif

/************************************************************************
* 函数名称：serialFlashProgrammer
* 函数说明：构造函数
* 功能说明：
* 输入参数：
* 返回参数：
*************************************************************************/
MainWindow::MainWindow(QWidget* parent)
	: QMainWindow(parent)
{
	//界面绘制
	ui.setupUi(this);
	ui.tabWidget->setTabText(0,"测试");
}
/************************************************************************
* 函数名称：~serialFlashProgrammer
* 函数说明：析构函数
* 功能说明：
* 输入参数：
* 返回参数：
*************************************************************************/
MainWindow::~MainWindow()
{

}