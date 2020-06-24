#include "Bucket.h"

#include "ColorMap.h"

Bucket::Bucket()
{}

Channel Bucket::channelWithBiggestRange() const
{
    int red_range = m_red_range.m_max - m_red_range.m_min;
    int green_range = m_green_range.m_max - m_green_range.m_min;
    int blue_range = m_blue_range.m_max - m_blue_range.m_min;

    return red_range > green_range ? Red :
                                     green_range > blue_range ?
                                         Green : Blue;

}
void Bucket::addColor(unsigned int color)
{
    if(m_histogram.contains(color))
    {
        m_histogram[color]++;
    }
    else
    {
        m_histogram.insert(color, 1);
    }
    if(RED(color) < m_red_range.m_min
            || m_red_range.m_min == 0)
    {
        m_red_range.m_min = RED(color);
    }
    if(GREEN(color) < m_green_range.m_min
            || m_green_range.m_min == 0)
    {
        m_green_range.m_min = GREEN(color);
    }
    if(BLUE(color) < m_blue_range.m_min
            || m_blue_range.m_min == 0)
    {
        m_blue_range.m_min = BLUE(color);
    }
    if(RED(color) > m_red_range.m_max)
    {
        m_red_range.m_max = RED(color);
    }
    if(GREEN(color) > m_green_range.m_max)
    {
        m_green_range.m_max = GREEN(color);
    }
    if(BLUE(color) > m_blue_range.m_max)
    {
        m_blue_range.m_max = BLUE(color);
    }
}
unsigned int Bucket::averageColor() const
{
    if(m_histogram.count() == 0)
    {
        return 0;
    }
    unsigned int sum_color[TotalChannels] = {0};
    unsigned int total_frequency = 0;
    for(unsigned int color: m_histogram.keys())
    {
        unsigned int frequency = m_histogram[color];
        total_frequency += frequency;
        sum_color[Red] += (RED(color) * frequency);
        sum_color[Green] += (GREEN(color) * frequency);
        sum_color[Blue] += (BLUE(color) * frequency);
    }
    unsigned int avg_color[TotalChannels] = {0};
    avg_color[Red] = sum_color[Red] / total_frequency;
    avg_color[Green] = sum_color[Green] / total_frequency;
    avg_color[Blue] = sum_color[Blue] / total_frequency;

    unsigned int result = 0;
    result |= avg_color[Blue];
    result |= (avg_color[Green] << 8);
    result |= (avg_color[Red] << 16);

    return result;
}
void  Bucket::fill(const QImage &image)
{
    const unsigned int *rgb = (const unsigned int *)image.constBits();
    for(int i = 0; i < image.width() * image.height(); ++i)
    {
        unsigned int color = rgb[i];
        addColor(color);
    }
}

