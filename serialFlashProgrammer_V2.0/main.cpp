#include "serialFlashProgrammer.h"
#include "MainWindow.h"
#include <QtWidgets/QApplication>
#include <qdialog.h>
#include <Windows.h>
int main(int argc, char *argv[])
{

#ifdef _WIN32
    //设置控制台编码格式为UTF8
    SetConsoleOutputCP(65001);
#endif

    QApplication a(argc, argv);
    MainWindow x;
    serialFlashProgrammer w;
    w.setWindowTitle(QString::fromLocal8Bit("C2000串口升级程序"));
    a.setWindowIcon(QIcon(":/serialFlashProgrammer/avatar.ico"));
    x.show();
    w.show();
    return a.exec();
}
