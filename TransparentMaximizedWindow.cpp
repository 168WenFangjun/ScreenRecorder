#include "TransparentMaximizedWindow.h"
#include "ui_TransparentMaximizedWindow.h"

#if defined(Win32) || defined(Win64)
#include <windows.h>
#endif

#include <QScreen>
#include <QDesktopWidget>
#include <QDebug>
#include <QMouseEvent>
#include <QPainter>
#include <QVBoxLayout>
#include <QLabel>
#include <QSizeGrip>

static const int BORDER_WIDTH = 5;
static const float WINDOW_OPACITY = 0.5f;

TransparentMaximizedWindow::TransparentMaximizedWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TransparentMaximizedWindow),
    m_capturing(false)
{
    ui->setupUi(this);
}

TransparentMaximizedWindow::~TransparentMaximizedWindow()
{
    delete ui;
}

void TransparentMaximizedWindow::drawSizeGrip()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(QMargins());
    layout->setSpacing(0);
    QLabel *label = new QLabel(this);
    layout->addWidget(label);

    auto sizeGrip = new QSizeGrip(this);
    layout->addWidget(sizeGrip, 0, Qt::AlignBottom | Qt::AlignRight);

#if defined(Win32) || defined(Win64)
    HWND hwnd = (HWND) label->winId();
    LONG styles = GetWindowLong(hwnd, GWL_EXSTYLE);
    SetWindowLong(hwnd, GWL_EXSTYLE, styles | WS_EX_TRANSPARENT);
#else
//    label->setWindowFlags(label->windowFlags() | Qt::WindowTransparentForInput);
#endif
}

void TransparentMaximizedWindow::moveToScreen(const QScreen* screen)
{
    QRect screen_geometry = screen->geometry();
    move(screen_geometry.x(), screen_geometry.y());
    resize(screen_geometry.width(), screen_geometry.height());
}

void TransparentMaximizedWindow::show(int width, int height, QScreen* screen)
{
    m_screen = screen;
    moveToScreen(screen);
    m_capturing = false;
    drawSizeGrip();
    setWindowState(Qt::WindowFullScreen);
    setWindowFlags(Qt::Window
                   | Qt::FramelessWindowHint
                   | Qt::WindowStaysOnTopHint
                   | Qt::X11BypassWindowManagerHint
                   );
    setCursor(Qt::CrossCursor);
#ifdef Linux
    setAttribute(Qt::WA_TranslucentBackground, true);
#elif defined(Win32) || defined(Win64)
    setWindowOpacity(WINDOW_OPACITY);
#endif
    setFixedSize(width, height);
    QWidget::showFullScreen();
}

QRect TransparentMaximizedWindow::getRectForBorder() const
{
    QRect rect;

    int x = (m_point_start.x() < m_point_stop.x()) ? m_point_start.x() : m_point_stop.x();
    int y = (m_point_start.y() < m_point_stop.y()) ? m_point_start.y() : m_point_stop.y();
    rect.setTopLeft(QPoint(x, y));

    x = (m_point_start.x() >= m_point_stop.x()) ? m_point_start.x() : m_point_stop.x();
    y = (m_point_start.y() >= m_point_stop.y()) ? m_point_start.y() : m_point_stop.y();
    rect.setBottomRight(QPoint(x, y));

    return rect;
}

QPoint TransparentMaximizedWindow::getTopLeft() const
{
    QPoint top_left;
    QPoint point_start(m_point_start.x()/* + m_screen->geometry().left()*/,
                       m_point_start.y()/* + m_screen->geometry().top()*/);
    QPoint point_stop(m_point_stop.x()/* + m_screen->geometry().left()*/,
                      m_point_stop.y()/* + m_screen->geometry().top()*/);
    point_start.rx() < point_stop.rx() ?
        top_left.setX(point_start.rx()) :
        top_left.setX(point_stop.rx());
    point_start.ry() < point_stop.ry() ?
        top_left.setY(point_start.ry()) :
        top_left.setY(point_stop.ry());
    return top_left;
}

static void drawRectBorder(QPainter &painter, const QRect &rect)
{
//    painter.setCompositionMode(QPainter::RasterOp_SourceXorDestination);
//    painter.setCompositionMode(QPainter::RasterOp_SourceAndDestination);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(Qt::green, BORDER_WIDTH, Qt::DashDotLine, Qt::FlatCap, Qt::MiterJoin));
    QRect r(rect);
    r.setX(rect.x() - BORDER_WIDTH);
    r.setY(rect.y() - BORDER_WIDTH);
    r.setWidth(rect.width() + BORDER_WIDTH*2);
    r.setHeight(rect.height() + BORDER_WIDTH*2);
    painter.drawRect(r);
    painter.end();
}

void TransparentMaximizedWindow::drawHole(const QRect rect)
{
    QRegion bg(this->rect());
    QRegion hole(QRect(rect.left(),
                       rect.top(),
                       rect.width(),
                       rect.height()),
                 QRegion::Rectangle);
    QRegion left(QRect(0,
                       0,
                       rect.left() - BORDER_WIDTH * 2,
                       height()),
                 QRegion::Rectangle);
    QRegion right(QRect(rect.right() + BORDER_WIDTH * 2,
                        0,
                        width() - rect.right() - BORDER_WIDTH * 2,
                        height()),
                  QRegion::Rectangle);
    QRegion topMiddle(QRect(rect.left() - BORDER_WIDTH * 2,
                            0,
                            rect.right() - BORDER_WIDTH * 2,
                            rect.top() - BORDER_WIDTH * 2),
                      QRegion::Rectangle);
    QRegion bottomMiddle(QRect(rect.left() - BORDER_WIDTH * 2,
                               rect.bottom() + BORDER_WIDTH * 2,
                               rect.right() - BORDER_WIDTH * 2,
                               height() - BORDER_WIDTH * 2),
                         QRegion::Rectangle);
    QRegion clipped = bg.subtracted(hole);
    clipped = clipped.subtracted(left);
    clipped = clipped.subtracted(right);
    clipped = clipped.subtracted(topMiddle);
    clipped = clipped.subtracted(bottomMiddle);
    setMask(clipped);
}

void TransparentMaximizedWindow::paintEvent(QPaintEvent *)
{
    const QRect rect = getRectForBorder();
    QPainter painter(this);
    if(m_capturing)
    {
        drawHole(rect);
    }
    drawRectBorder(painter, rect);
}

void TransparentMaximizedWindow::mousePressEvent(QMouseEvent *mouse_event)
{
    if(m_capturing) {return;}
    m_point_start = mouse_event->windowPos().toPoint();
    m_point_stop = mouse_event->windowPos().toPoint();
    emit captureRegionSizeChanged(m_point_start.x(), m_point_start.y(),
                                  m_point_stop.x(), m_point_stop.y());
}

void TransparentMaximizedWindow::mouseMoveEvent(QMouseEvent *mouse_event)
{
    if(m_capturing) {return;}
    m_point_stop = mouse_event->windowPos().toPoint();
    emit captureRegionSizeChanged(m_point_start.x(), m_point_start.y(),
                                  m_point_stop.x(), m_point_stop.y());
    update();
}

void TransparentMaximizedWindow::mouseReleaseEvent(QMouseEvent *mouse_event)
{
    if(m_capturing) {return;}
    startCapture(m_point_start, mouse_event->windowPos().toPoint());
    emit selectRegionForCapture(getTopLeft(), getRectForBorder());
}

void TransparentMaximizedWindow::startCapture(
        const QPoint &point_start,
        const QPoint &point_stop)
{
    m_point_start = point_start;
    m_point_stop = point_stop;
    m_capturing = true;
#if defined(Win32) || defined(Win64)
    HWND hwnd = (HWND) winId();
    LONG styles = GetWindowLong(hwnd, GWL_EXSTYLE);
    SetWindowLong(hwnd, GWL_EXSTYLE, styles | WS_EX_TRANSPARENT);
#elif defined Linux
//    setWindowFlags(windowFlags() | Qt::WindowTransparentForInput);
//    setAttribute(Qt::WA_TransparentForMouseEvents);
#elif defined Darwin
    setIgnoresMouseEvents(true);
#endif
    qDebug() << "startCapture"
             << "point_start" << m_point_start
             << "point_stop" << m_point_stop;

    update();
}


