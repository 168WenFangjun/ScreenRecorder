#pragma once

#include <sys/types.h>
#include <stdio.h>

#if defined(WIN32) || defined(WIN64)
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;
typedef long long int64_t;
#endif

struct AVRational;
class YuvWriter
{
public:
    YuvWriter();

    bool open(int width, int height, AVRational &&time_base, int fps);
    void close();
    void write(uint8_t *plane[4], int width, int height, int64_t pts);

protected:
    FILE* m_yuv_fp;
    int64_t m_time_stamp_of_prev_frame;
};

