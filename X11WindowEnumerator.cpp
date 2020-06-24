#include "X11WindowEnumerator.h"

#include <QDebug>
#include <QRect>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xfixes.h>
#include <QtX11Extras/QX11Info>

namespace {

QRect windowRect(_XDisplay *dpy, Window w)
{
    XWindowAttributes xwa;//map_state map_state
    XGetWindowAttributes(dpy, w, &xwa);
    Window child;
    int x = xwa.x;
    int y = xwa.y;
    XTranslateCoordinates(dpy, w, DefaultRootWindow(dpy), 0, 0,
                          &x, &y, &child);
    return QRect(x, y, xwa.width, xwa.height);
}

QRect windowRect2(_XDisplay *dpy, Window w)
{
    Window root_return;
    int x;
    int y;
    unsigned int width;
    unsigned int height;
    unsigned int border_width;
    unsigned int depth;
    XGetGeometry(dpy, w, &root_return, &x, &y, &width, &height, &border_width, &depth);
    return QRect(x, y, width, height);
}

    Window foo(_XDisplay *dpy, Window w, int x, int y)
    {
        if(w == 0)
        {
            return w;
        }
        Window root_return;
        Window child_return = w;
//        int root_x_return;
//        int root_y_return;
//        int win_x_return;
//        int win_y_return;
//        unsigned int mask_return;
//        if(XQueryPointer(dpy,
//                      w,
//                      &root_return,
//                      &child_return,
//                      &root_x_return,
//                      &root_y_return,
//                      &win_x_return,
//                      &win_y_return,
//                      &mask_return))
        {
            Window parent_return;
            Window *children_return;
            Window next = w;
            unsigned int nchildren_return = 0;
            XQueryTree(dpy, child_return, &root_return, &parent_return, &children_return, &nchildren_return);
            for(int i = 0; i < nchildren_return; ++i)
            {
                QRect rect = windowRect2(dpy, children_return[i]);
                char* name = "\0";
                XFetchName(dpy, children_return[i], &name);
                XWindowAttributes xwa;
                XGetWindowAttributes(dpy, w, &xwa);
                if(QString(name) == "ScreenRecorder" ||
                        QString(name) == "Screen Recorder")
                {
                    qDebug() << QString("#########ScreenRecorder, (%1,%2)x(%3x%4)").
                                                    arg(rect.x()).
                                                    arg(rect.y()).
                                                    arg(rect.width()).
                                                    arg(rect.height());
                }
                if(x >= rect.x() && y > rect.y() && x < rect.x() + rect.width() && y < rect.y() + rect.height()
                        && QString(name) != "FullScreenWindow"
//                        && xwa.map_state == IsViewable
                        )
                {
                    qDebug() << "possible candidate window " << name
                             << QString(", (%1,%2)x(%3x%4)").
                                arg(rect.x()).
                                arg(rect.y()).
                                arg(rect.width()).
                                arg(rect.height());
                    next = children_return[i];
                }
            }
            if(next != w)
            {
                char* name = "\0";
                QRect rect = windowRect2(dpy, next);
                XFetchName(dpy, next, &name);
                qDebug() << "go deeper into " << name
                         << "mouse" << QString("(%1,%2)").arg(x).arg(y)
                         << QString("(%1,%2)x(%3x%4)").
                            arg(rect.x()).
                            arg(rect.y()).
                            arg(rect.width()).
                            arg(rect.height());
                return foo(dpy, next, x, y);
            }
            return w;
        }
        return w;
    }

//    https://github.com/ckaiser/Lightscreen/blob/master/tools/os.cpp
    Window findRealWindow(Window w, int depth = 0)
    {
        if (depth > 50) {
            return None;
        }

        static Atom wm_state = XInternAtom(QX11Info::display(), "WM_STATE", False);
        Atom type;
        int format;
        unsigned long nitems, after;
        unsigned char *prop;

        if (XGetWindowProperty(QX11Info::display(), w, wm_state, 0, 0, False, AnyPropertyType,
                               &type, &format, &nitems, &after, &prop) == Success) {
            if (prop != NULL) {
                XFree(prop);
            }

            if (type != None) {
                return w;
            }
        }

        Window root, parent;
        Window *children;
        unsigned int nchildren;
        Window ret = None;

        if (XQueryTree(QX11Info::display(), w, &root, &parent, &children, &nchildren) != 0) {
            for (unsigned int i = 0;
                    i < nchildren && ret == None;
                    ++i) {
                ret = findRealWindow(children[ i ], depth + 1);
            }

            if (children != NULL) {
                XFree(children);
            }
        }

        return ret;
    }
    Window windowUnderCursor(bool includeDecorations)
    {
        Window root;
        Window child;
        uint mask;
        int rootX, rootY, winX, winY;

        XQueryPointer(QX11Info::display(), QX11Info::appRootWindow(), &root, &child,
                      &rootX, &rootY, &winX, &winY, &mask);

        if (child == None) {
            child = QX11Info::appRootWindow();
        }

        char* name = "\0";
        XFetchName(QX11Info::display(), child, &name);
        if(QString(name) == "FullScreenWindow")
        {
//            int i = 0;
//            i = 1;
//            XQueryPointer(QX11Info::display(), child, &root, &child,
//                          &rootX, &rootY, &winX, &winY, &mask);
//            XFetchName(QX11Info::display(), child, &name);
//            i = 2;
        }
        if (!includeDecorations) {
            Window real_child = findRealWindow(child);

            if (real_child != None) {  // test just in case
                child = real_child;
            }
        }

        return child;
    }
}//namespace

X11WindowEnumerator::X11WindowEnumerator(QObject *parent)
    : WindowEnumeratorInterface(parent)
    , m_dpy(XOpenDisplay(0))
{

}

X11WindowEnumerator::~X11WindowEnumerator()
{

}

void X11WindowEnumerator::enumTopLevelWindows()
{
//    m_map.clear();

//    Atom a = XInternAtom(m_dpy, "_NET_CLIENT_LIST" , true);
//    Atom actualType;
//    int format;
//    unsigned long numItems, bytesAfter;
//    unsigned char *data =0;
//    int status = XGetWindowProperty(m_dpy,
//                                DefaultRootWindow(m_dpy),
//                                a,
//                                0L,
//                                (~0L),
//                                false,
//                                AnyPropertyType,
//                                &actualType,
//                                &format,
//                                &numItems,
//                                &bytesAfter,
//                                &data);

//    if (status >= Success && numItems)
//    {
//        // success - we have data: Format should always be 32:
//        Q_ASSERT(format == 32);
//        // cast to proper format, and iterate through values:
//        long *array = (long*) data;
//        for (quint32 k = 0; k < numItems; k++)
//        {
//            // get window Id:
//            Window w = (Window) array[k];
//            QRect rect = windowRect(m_dpy, w);
//            char* name = '\0';
//            status = XFetchName(m_dpy, w, &name);
//            qDebug() << "Scanned client window:" << w
//                     << name
//                     << QString("(%1,%2)x(%3x%4)").
//                        arg(rect.left()).
//                        arg(rect.right()).
//                        arg(rect.width()).
//                        arg(rect.height());
//            m_map.insert((qint64)w,
//                         WindowProperties(
//                             rect,
//                             QString(name)));
//        }
//        XFree(data);
//    }
}

QRect X11WindowEnumerator::windowRectWithPoint(int x, int y)
{
    Window w = windowUnderCursor(true);
    char* name = "\0";
    XFetchName(m_dpy, w, &name);
    QRect rect = windowRect2(m_dpy, w);
    qDebug() << "selecting window"
             << name
             << QString("(%1,%2)x(%3x%4)").
                arg(rect.left()).
                arg(rect.top()).
                arg(rect.width()).
                arg(rect.height());
    return rect;

//    for(auto w : m_map.keys())
//    {
//        QRect rect = m_map[w].m_geometry;
//        if(rect.contains(x, y) &&
//                rect.left()!=0 &&
//                rect.top()!=0 &&
//                !m_map[w].m_name.isEmpty())
//        {
//            qDebug() << "selecting window"
//                     << m_map[w].m_name
//                     << QString("(%1,%2)x(%3x%4)").
//                        arg(rect.left()).
//                        arg(rect.top()).
//                        arg(rect.width()).
//                        arg(rect.height());
//            return rect;
//        }
//    }
//    return QRect();
}
