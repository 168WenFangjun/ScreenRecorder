#include "CrossCursorWidget.h"
#include "ui_CrossCursorWidget.h"

#include <QDebug>
#include <QScreen>

CrossCursorWidget::CrossCursorWidget(const QString &title,
        const QString &label,
        const QImage &img,
        QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CrossCursorWidget)
{
    ui->setupUi(this);
    if(!img.isNull())
    {
        ui->pushButtonStartDrag->setIcon(QPixmap::fromImage(img.scaledToHeight(100)));
    }
    setWindowTitle(title);
}

CrossCursorWidget::~CrossCursorWidget()
{
    delete ui;
}

void CrossCursorWidget::on_pushButtonStartDrag_clicked()
{
    close();
    emit done();
}

void CrossCursorWidget::show(int x, int y)
{
    move(x, y);
    setWindowFlags(Qt::Window
                   | Qt::WindowStaysOnTopHint
                   );
    QWidget::show();
}
