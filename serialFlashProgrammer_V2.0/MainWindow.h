#pragma once
#include <QtWidgets/qmainwindow.h>
#include "ui_MainWindow.h"

class MainWindow :public QMainWindow
{
	Q_OBJECT
public:
    MainWindow(QWidget* parent = Q_NULLPTR);
    ~MainWindow();

private:
    Ui::MainWindow  ui;
};

