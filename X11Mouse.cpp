#include "X11Mouse.h"
#include <QDebug>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xfixes.h>
#include <QtX11Extras/QX11Info>

X11Mouse::X11Mouse(QObject *parent)
    : MouseInterface(parent)
    , m_dpy(XOpenDisplay(0))
{
}

X11Mouse::~X11Mouse()
{
    XCloseDisplay(m_dpy);
}


QImage X11Mouse::getMouseCursor(QPoint &pos) const
{
    QImage cursor;
    XFixesCursorImage *xfcursorImage = XFixesGetCursorImage(m_dpy);

    static quint32* buf = nullptr;
    static long len = 0;

    long leni = xfcursorImage->width * xfcursorImage->height;
    if(len != leni)
    {
        len = leni;
        delete [] buf;
        buf = new quint32[len];
    }

    for(long i = 0; i < len; ++i)
    {
        buf[i] = (quint32)xfcursorImage->pixels[i];
    }
    cursor = QImage((uchar*)buf,
                    xfcursorImage->width,
                    xfcursorImage->height,
                    QImage::Format_ARGB32_Premultiplied);
    pos = QPoint(xfcursorImage->x, xfcursorImage->y);
    pos -= QPoint(xfcursorImage->xhot, xfcursorImage->yhot);

    XFree(xfcursorImage);

//    cursor.save("/tmp/xcursor.png");
    return cursor;
}
