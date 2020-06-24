#include "WatermarkOpacityCalculator.h"


#include <QDebug>

WatermarkOpacityCalculator::WatermarkOpacityCalculator()
{

}

void WatermarkOpacityCalculator::reset(int start_time)
{
    m_state = 0;
    m_elapsed.restart();
}

qreal WatermarkOpacityCalculator::getWatermarkOpacity()
{
    enum {
        STATE_1 = 0,
        STATE_APPEARING,
        STATE_VISIBLE,
        STATE_DISAPPEARING,
        STATE_INVISIBLE
    };
    enum {
        STATE_1_MS = 60 * 2 * 1000,
        STATE_APPEARING_MS = 3 * 1000,
        STATE_VISIBLE_MS = 12 * 1000,
        STATE_DISAPPEARING_MS = 3 * 1000,
        STATE_INVISIBLE_MS = 42 * 1000
    };

    qreal opacity = 0.0;
    switch(m_state)
    {
    case STATE_1:
        opacity = 0.0;
        if(m_elapsed.elapsed() > STATE_1_MS)
        {
            m_state = STATE_APPEARING;
            m_elapsed.restart();
        }
        break;
    case STATE_APPEARING:
        opacity = m_elapsed.elapsed() / (qreal)STATE_APPEARING_MS;
        if(m_elapsed.elapsed() > STATE_APPEARING_MS)
        {
            m_state = STATE_VISIBLE;
            m_elapsed.restart();
        }
        break;
    case STATE_VISIBLE:
        opacity = 1.0;
        if((m_elapsed.elapsed() > STATE_VISIBLE_MS))
        {
            m_state = STATE_DISAPPEARING;
            m_elapsed.restart();
        }
        break;
    case STATE_DISAPPEARING:
        opacity = 1 - (m_elapsed.elapsed()) / (qreal)STATE_DISAPPEARING_MS;
        if((m_elapsed.elapsed() > STATE_DISAPPEARING_MS))
        {
            m_state = STATE_INVISIBLE;
            m_elapsed.restart();
        }
        break;
    case STATE_INVISIBLE:
        opacity = 0.0;
        if((m_elapsed.elapsed() > STATE_INVISIBLE_MS))
        {
            m_state = STATE_APPEARING;
            m_elapsed.restart();
        }
        break;
    }
    return opacity;
}
