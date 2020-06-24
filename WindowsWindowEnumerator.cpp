#include "WindowsWindowEnumerator.h"

#include <QDebug>
#include <QRect>

WindowsWindowEnumerator::WindowsWindowEnumerator(QObject *parent)
    : WindowEnumeratorInterface(parent)
{
}

BOOL WindowsWindowEnumerator::EnumWindowsProc(HWND   hwnd, LPARAM lParam)
{
//    if(IsIconic(hwnd) ||
//            !IsWindowVisible(hwnd) ||
//            GetDesktopWindow() != GetParent(hwnd) ||
//            (GetWindowLong(hwnd, GWL_STYLE) & WS_VISIBLE != WS_VISIBLE)) {
//       return true;
//    }
//    RECT rect;
//    GetWindowRect(hwnd, &rect);
//    char title[1024] = {0};
//    GetWindowText(hwnd, title, sizeof(title));
//    int width = rect.right - rect.left;
//    int height = rect.bottom - rect.top;
//    if(width <= 0 || height <=0)
//    {
//        return true;
//    }
//    qDebug() << "enumerating window" << hwnd
//             << QString("(%1,%2)x(%3,%4)").arg(rect.left).arg(rect.top).
//                arg(width).arg(height)
//             << title;
//    auto pthis = (WindowsWindowEnumerator*)lParam;
//    pthis->m_map.insert((qint64)hwnd, QRect(rect.left,
//                                            rect.top,
//                                            rect.right - rect.left,
//                                            rect.bottom - rect.top));
    return true;
}

void WindowsWindowEnumerator::enumTopLevelWindows()
{
//    m_map.clear();
//    EnumWindows(WindowsWindowEnumerator::EnumWindowsProc, (LPARAM)this);
}

QRect WindowsWindowEnumerator::windowRectWithPoint(int x, int y)
{
    POINT p;
    p.x = x;
    p.y = y;
    HWND hwnd = WindowFromPoint(p);
    if(!hwnd)
    {
        qDebug() << "no wnd";
        return QRect();
    }
    RECT rect;
    GetWindowRect(hwnd, &rect);
    int border_thickness = GetSystemMetrics(SM_CXSIZEFRAME);
    int width = rect.right - rect.left - 2 * border_thickness;
    int height = rect.bottom - rect.top - border_thickness;
    rect.left += border_thickness;

    char title[1024] = {0};
    GetWindowText(hwnd, title, sizeof(title));
    qDebug() << "point in " << title
             << QString("(%1,%2)x(%3,%4)").arg(rect.left).arg(rect.top).
                arg(width).arg(height);
    return QRect(rect.left, rect.top, width, height);
}

