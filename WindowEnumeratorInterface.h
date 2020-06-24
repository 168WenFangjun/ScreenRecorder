#pragma once

#include <QObject>

class WindowEnumeratorInterface : public QObject
{
    Q_OBJECT
public:
    WindowEnumeratorInterface(QObject *parent) {}
    virtual ~WindowEnumeratorInterface() {}
    virtual void enumTopLevelWindows() = 0;
    virtual QRect windowRectWithPoint(int x, int y) = 0;
};
