#pragma once

#include "MouseInterface.h"

class WindowsMouse : public MouseInterface
{
public:
    WindowsMouse(QObject *parent);

    virtual QImage getMouseCursor(QPoint &pos) const override;
};
