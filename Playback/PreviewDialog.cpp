#include "PreviewDialog.h"
#include "ui_PreviewDialog.h"

PreviewDialog::PreviewDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PreviewDialog)
{
    ui->setupUi(this);
}

PreviewDialog::~PreviewDialog()
{
    delete ui;
}

void PreviewDialog::SetImage(const QImage &img)
{
    ui->label->setPixmap(QPixmap::fromImage(img));
    ui->label->show();
}
