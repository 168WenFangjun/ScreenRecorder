#pragma once

#include <QList>
#include <QImage>
#include <QMap>
struct Bucket;
struct ColorMapObject;
class ColorMap
{
public:
    ColorMap();
    ColorMap(const QImage &img);
    ~ColorMap();

    ColorMapObject *get() const;
    QList<Bucket> quantize(const QImage &image) const;
    unsigned char colorIndex(unsigned int rgb) const;

protected:
    QList<Bucket> medianCut(const Bucket& bucket) const;
    ColorMapObject *emptyColorMap()const;
    ColorMapObject *generateColorMap(const QImage &img) const;

protected:
    ColorMapObject *m_color_map;
    mutable QMap<unsigned int/*rgb*/, unsigned char/*index*/>m_cache;
};
