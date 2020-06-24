#pragma once

#include "WindowEnumeratorInterface.h"

#include <QMap>
#include <QRect>

struct _XDisplay;
class X11WindowEnumerator : public WindowEnumeratorInterface
{
public:
    X11WindowEnumerator(QObject *parent = nullptr);
    virtual ~X11WindowEnumerator() override;
    virtual void enumTopLevelWindows() override;
    virtual QRect windowRectWithPoint(int x, int y) override;
protected:
    _XDisplay* m_dpy;
    struct WindowProperties
    {
        QRect m_geometry;
        QString m_name;
        WindowProperties(const QRect& rect, const QString &name)
            : m_geometry(rect)
            , m_name(name){}
        WindowProperties() {}
    };
    QMap<qint64/*hwnd*/, WindowProperties> m_map;
};
