#pragma once

#include <QWidget>

namespace Ui {
class TransparentMaximizedWindow;
}

class TransparentMaximizedWindow : public QWidget
{
    Q_OBJECT

public:
    explicit TransparentMaximizedWindow(QWidget *parent = 0);
    ~TransparentMaximizedWindow();

    void show(int width, int height, QScreen *screen);

    void drawSizeGrip();
    void startCapture(const QPoint &point_start,
                      const QPoint &point_stop);
    void moveToScreen(const QScreen *screen);
    
protected:
    void drawHole(const QRect rect);

signals:
    void selectRegionForCapture(const QPoint &top_left, const QRect &border_rect);
    void captureRegionSizeChanged(float top_left_x, float top_left_y, float bottom_right_x, float bottom_right_y);
private:
    Ui::TransparentMaximizedWindow *ui;
    QPoint m_point_start;
    QPoint m_point_stop;
    bool m_capturing;
    QScreen *m_screen;

    QRect getRectForBorder() const;
    QPoint getTopLeft() const;
    virtual void paintEvent(QPaintEvent *);
    virtual void mousePressEvent(QMouseEvent *mouse_event);
    virtual void mouseReleaseEvent(QMouseEvent *mouse_event);
    virtual void mouseMoveEvent(QMouseEvent *mouse_event);
};
