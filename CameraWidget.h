#pragma once

#include <QWidget>

namespace Ui {
class CameraWidget;
}

class QCamera;
class QCameraViewfinder;
class CameraWidget : public QWidget
{
    Q_OBJECT
public:
    explicit CameraWidget(QWidget *parent = 0);
    ~CameraWidget();

protected:
    void resizeEvent(QResizeEvent* event);
    void closeEvent(QCloseEvent *event);

    Ui::CameraWidget *ui;
    QCamera *m_camera;
    QCameraViewfinder *m_view_finder;
};
