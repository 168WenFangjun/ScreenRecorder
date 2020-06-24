#pragma once

#include <QObject>
#include <QElapsedTimer>

class WatermarkOpacityCalculator
{
public:
    WatermarkOpacityCalculator();
    void reset(int start_time);
    qreal getWatermarkOpacity();

protected:
    int m_state = 0;
    QElapsedTimer m_elapsed;
};
