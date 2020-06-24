#include "ColorMap.h"

#include <gif_lib.h>
#include <QDebug>
#include "Bucket.h"

ColorMap::ColorMap()
    : m_color_map(nullptr)
{
    m_color_map = emptyColorMap();
}
ColorMap::ColorMap(const QImage &img)
    : m_color_map(nullptr)
{
    m_color_map = generateColorMap(img);
}
ColorMap::~ColorMap()
{
    if(m_color_map)
    {
        free(m_color_map);
    }
}

ColorMapObject *ColorMap::get() const
{
    return m_color_map;
}

QList<Bucket> ColorMap::quantize(const QImage &image) const
{
    Bucket bucket;
    bucket.fill(image);

    QList<Bucket> bucket_list;
    bucket_list << bucket;

    while(bucket_list.count() > 0 && bucket_list.count() < 256)
    {
        Bucket bucket;
        for(int i = 0; i < bucket_list.count(); ++i)
        {
            if(bucket_list.at(i).m_histogram.count() > 1)
            {
                bucket = bucket_list.at(i);
                bucket_list.removeAt(i);
                break;
            }
        }
        if(bucket.m_histogram.count() < 2)
        {
            qDebug() << "count" << bucket_list.count();
            break;
        }
        bucket_list.append(medianCut(bucket));
    }
    return bucket_list;
}

unsigned char ColorMap::colorIndex(unsigned int rgb) const
{
    if(m_cache.contains(rgb))
    {
        return m_cache.value(rgb);
    }
    int idx = 0;
    qint64 min_euclidean_distance_squared = 2147483647;//max int (2 power 32 -1)
    for(int i = 0; i < m_color_map->ColorCount; ++i)
    {
        int color = (m_color_map->Colors[i].Red << 16) |
                (m_color_map->Colors[i].Green << 8) |
                (m_color_map->Colors[i].Blue);
        if(color == (rgb & 0xffffff))
        {
            idx = i;
            break;
        }
        qint64 euclidean_distance_squared =
                (m_color_map->Colors[i].Red - RED(rgb)) * (m_color_map->Colors[i].Red - RED(rgb)) +
                (m_color_map->Colors[i].Green - GREEN(rgb)) * (m_color_map->Colors[i].Green - GREEN(rgb)) +
                (m_color_map->Colors[i].Blue - BLUE(rgb)) * (m_color_map->Colors[i].Blue - BLUE(rgb));
        if(euclidean_distance_squared < min_euclidean_distance_squared)
        {
            min_euclidean_distance_squared = euclidean_distance_squared;
            idx = i;
        }
    }
    m_cache[rgb] = idx;
    return idx;
}

QList<Bucket> ColorMap::medianCut(const Bucket& bucket) const
{
    //Put all the colors from bucket into a tree, sorted by channel value.
    //Start traversing the tree to get sorted list of colors from highest to lowest channel values,
    //while summing up the frequency from each color, and putting the color in a new bucket.
    //When sum of frequencies <= half the total frequency of all colors in the bucket,
    //then first bucket is complete. Put the rest of the points in second bucket.
    //Return the two buckets.
    Channel channel = bucket.channelWithBiggestRange();

    QList<unsigned int> colors = bucket.m_histogram.keys();
    std::sort(colors.begin(), colors.end(), [channel](unsigned int color1, unsigned int color2){
        switch(channel)
        {
        case Red:
            return RED(color1) > RED(color2);
        case Green:
            return GREEN(color1) > GREEN(color2);
        case Blue:
        default:
            return BLUE(color1) > BLUE(color2);
        }
    });
    unsigned int total_frequency = 0;
    for(auto color : colors)
    {
        total_frequency += bucket.m_histogram[color];
    }
    unsigned int frequency = 0;
    Bucket bucket1, bucket2, *bucket_p;
    bucket_p = &bucket1;
    for(auto color : colors)
    {
        bucket_p->addColor(color);
        frequency += bucket.m_histogram[color];
        if(frequency >= total_frequency/2 && bucket_p != &bucket2)
        {
            bucket_p = &bucket2;
        }
    }
    QList<Bucket> res;
    if(bucket1.m_histogram.count() > 0)
    {
        res.append(bucket1);
    }
    if(bucket2.m_histogram.count() > 0)
    {
        res.append(bucket2);
    }
    return res;
}

ColorMapObject *ColorMap::emptyColorMap()const
{
    ColorMapObject *color_map = (ColorMapObject *)malloc(
                sizeof(ColorMapObject) + 256 * sizeof(GifColorType));
    color_map->BitsPerPixel = 8;
    color_map->ColorCount = 256;
    color_map->SortFlag = 0;
    color_map->Colors = (GifColorType*)(&color_map[1]);
    memset(color_map->Colors, 0, 256 * sizeof(GifColorType));
    return color_map;

}

ColorMapObject *ColorMap::generateColorMap(const QImage &img) const
{
    QList<unsigned int> colors;
    auto bucket_list = quantize(img);
    int i = 0;
    for(auto &bucket : bucket_list)
    {
        unsigned int avg_color = bucket.averageColor();
        //            qDebug() << "bucket#" << i++
        //                     << "num colors" << bucket.m_histogram.count()
        //                     << "avg color" << QColor(RED(avg_color), GREEN(avg_color), BLUE(avg_color));
        colors.append(avg_color);
    }

    ColorMapObject *color_map = (ColorMapObject *)malloc(
                sizeof(ColorMapObject) + colors.count() * sizeof(GifColorType));
    color_map->BitsPerPixel = 8;
    color_map->ColorCount = colors.count();
    color_map->SortFlag = 0;
    color_map->Colors = (GifColorType*)(&color_map[1]);
    for(int i = 0; i < colors.count(); ++i)
    {
        color_map->Colors[i].Red = RED(colors.at(i));
        color_map->Colors[i].Green = GREEN(colors.at(i));
        color_map->Colors[i].Blue = BLUE(colors.at(i));
    }
    return color_map;
}
