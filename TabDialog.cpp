#include "TabDialog.h"
#include "ui_TabDialog.h"

TabDialog::TabDialog(QList<QPair<QString, QWidget *> > widgets, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TabDialog)
{
    ui->setupUi(this);
    for(auto widget : widgets)
    {
        ui->tabWidget->addTab(widget.second, widget.first);
    }
}

TabDialog::~TabDialog()
{
    delete ui;
}
