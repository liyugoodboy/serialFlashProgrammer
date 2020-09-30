#pragma once
#include <qdialog.h>
#include "ui_viewProgram.h"
class ViewProgram : public QDialog
{
    Q_OBJECT
public:
     ViewProgram(QWidget* parent = Q_NULLPTR);
    ~ViewProgram();
private slots:

signals:


private:
    Ui::viewProgramClass ui;
};

