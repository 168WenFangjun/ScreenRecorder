#include <QDesktopServices>
#include <QUrl>

#include "AboutWidget.h"
#include "ui_AboutWidget.h"

AboutWidget::AboutWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AboutWidget)
{
    ui->setupUi(this);
    makePushButtonIntoWebLink();
}

AboutWidget::~AboutWidget()
{
    delete ui;
}

void AboutWidget::makePushButtonIntoWebLink()
{
    ui->pushButtonUrl->setStyleSheet("QPushButton {color: blue; text-decoration:underline;text-align:left; }");
    ui->pushButtonUrl->setCursor(Qt::PointingHandCursor);

    ui->pushButtonEmail->setStyleSheet("QPushButton {color: blue; text-decoration:underline;text-align:left; }");
    ui->pushButtonEmail->setCursor(Qt::PointingHandCursor);
}

void AboutWidget::on_pushButtonEmail_clicked()
{
    QDesktopServices::openUrl(QUrl(ui->pushButtonEmail->text()));
}

void AboutWidget::on_pushButtonUrl_clicked()
{
    QDesktopServices::openUrl(QUrl(ui->pushButtonUrl->text()));
}
