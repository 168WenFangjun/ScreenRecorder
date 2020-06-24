#pragma once

#include <QMap>
#include <QImage>

#define RED(rgb) ((rgb & 0xff0000) >> 16)
#define GREEN(rgb) ((rgb & 0x00ff00) >> 8)
#define BLUE(rgb) ((rgb & 0x0000ff))

const int MAX_COLORS = 256;
enum Channel
{
    Red,
    Green,
    Blue,
    TotalChannels
};

struct Range
{
    Range()
        : Range(0, 0)
    {}
    Range(unsigned int min, unsigned int max)
        : m_min(min)
        , m_max(max)
    {}
    unsigned int m_min;
    unsigned int m_max;
};

class Bucket
{
public:
    struct Range
    {
        Range()
            : Range(0, 0)
        {}
        Range(unsigned int min, unsigned int max)
            : m_min(min)
            , m_max(max)
        {}
        unsigned int m_min;
        unsigned int m_max;
    };
    Bucket();

    Channel channelWithBiggestRange() const;
    void addColor(unsigned int color);
    unsigned int averageColor() const;
    void fill(const QImage &image);

    QMap<unsigned int /*color*/, unsigned int/*frequency*/> m_histogram;
    Range m_red_range;
    Range m_green_range;
    Range m_blue_range;
};
