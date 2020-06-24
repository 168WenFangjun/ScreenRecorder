#include "CameraWidget.h"
#include "ui_CameraWidget.h"

#include <QDebug>
#include <QResizeEvent>
#include <QtMultimedia/QCamera>
#include <QtMultimediaWidgets/QCameraViewfinder>

CameraWidget::CameraWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::CameraWidget)
    , m_camera(new QCamera(this))
    , m_view_finder(new QCameraViewfinder(this))
{
    ui->setupUi(this);

    m_view_finder->setGeometry(0, 0, width(), height());
    m_camera->setViewfinder(m_view_finder);

    m_view_finder->show();
    m_camera->start();

}

CameraWidget::~CameraWidget()
{
    delete ui;
}

void CameraWidget::resizeEvent(QResizeEvent *event)
{
    qDebug() <<"rezied" << event->size();
    m_view_finder->setGeometry(0, 0, event->size().width(), event->size().height());
}

void CameraWidget::closeEvent(QCloseEvent *event)
{
    event->ignore();
    hide();
}
