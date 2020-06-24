#pragma once

#include <QDebug>
#include <QList>
#include <QImage>

struct ColorMap;
struct GifFileType;
class GifMaker
{
public:
    GifMaker();
    ~GifMaker();
    bool open(const QString &out_file, const QImage *img_for_generating_color_map);
    bool close();
    bool addFrame(const QImage &img, int loop_delay_ms);
    bool write(const QList<QImage> png_files, const QString &out_file, int loop_delay_ms) const;
protected:
    bool addFirstFrame(const QImage &img, int loop_delay_ms);
    bool addLoop(GifFileType *gifh) const;
    int addGifFrame(GifFileType *gifh, const QImage &img, int delay_msec) const;

    QString m_out_file;
    GifFileType *m_gifh;
    int m_width;
    int m_height;
    mutable int m_frame;
    ColorMap *m_color_map;
};
