#include "ScreenStreamer.h"

#include <QCursor>
#include <QDebug>
#include <QGuiApplication>
#include <QImage>
#include <QPixmap>
#include <QScreen>

#include "SocketWriter.h"

#include <jpeglib.h>

namespace  {
//https://www.ridgesolutions.ie/index.php/2019/12/10/libjpeg-example-encode-jpeg-to-memory-buffer-instead-of-file/
// Encodes a 32bpp image to JPEG directly to a memory buffer
// libJEPG will malloc() the buffer so the caller must free() it when
// they are finished with it.
//
// image    - the input 32 bpp bgra image, 4 byte is 1 pixel.
// width    - the width of the input image
// height   - the height of the input image
// quality  - target JPEG 'quality' factor (max 100)
// comment  - optional JPEG NULL-termoinated comment, pass NULL for no comment.
// jpegSize - output, the number of bytes in the output JPEG buffer
// jpegBuf  - output, a pointer to the output JPEG buffer, must call free() when finished with it.
//
void ToJpeg(unsigned char* image, int width, int height, int quality,
            const char* comment, unsigned long* jpegSize, unsigned char** jpegBuf)
{
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;

    JSAMPROW row_pointer[1];
    int bytesperpixel = 4;
    int row_stride;

    cinfo.err = jpeg_std_error(&jerr);

    jpeg_create_compress(&cinfo);
    cinfo.image_width = width;
    cinfo.image_height = height;

    // Input is greyscale, 1 byte per pixel
    cinfo.input_components = bytesperpixel;
    cinfo.in_color_space = JCS_EXT_BGRX;

    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, quality, TRUE);

    //
    //
    // Tell libJpeg to encode to memory, this is the bit that's different!
    // Lib will alloc buffer.
    //
    jpeg_mem_dest(&cinfo, jpegBuf, jpegSize);

    jpeg_start_compress(&cinfo, TRUE);

    // Add comment section if any..
    if (comment) {
        jpeg_write_marker(&cinfo, JPEG_COM, (const JOCTET*)comment, strlen(comment));
    }

    // 1 BPP
    row_stride = width * bytesperpixel;

    // Encode
    while (cinfo.next_scanline < cinfo.image_height) {
        row_pointer[0] = &image[cinfo.next_scanline * row_stride];
        jpeg_write_scanlines(&cinfo, row_pointer, 1);
    }

    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);
}

}//namespace
ScreenStreamer::ScreenStreamer()
{
    InitAvailableScreens();
}

ScreenStreamer::~ScreenStreamer()
{
    stop_ = true;
    thread_.wait();
}

void ScreenStreamer::InitAvailableScreens()
{
    m_screens = QGuiApplication::screens();
    std::sort(m_screens.begin(), m_screens.end(),
              [](QScreen *a, QScreen *b){
        return (a->geometry().x() != b->geometry().x()) ?
                    a->geometry().x() < b->geometry().x() :
                    a->geometry().y() < b->geometry().y();
    });
}

int ScreenStreamer::ActiveScreen() const
{
    int screen_idx = 0;
    for(auto screen : m_screens)
    {
        if(screen->geometry().contains(QCursor::pos()))
        {
            break;
        }
        screen_idx++;
    }
    return screen_idx;

}


void ScreenStreamer::StartStreaming()
{
    fp_ = std::make_unique<SocketWriter>();
    fp_->BfOpen("127.0.0.1", 9000);
    thread_ = std::async(std::launch::async, [this](){
        while(!stop_){
            std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

            QImage img = m_screens[ActiveScreen()]->grabWindow(0).toImage();
            unsigned long jpegSize = 0;
            unsigned char* jpegBuf = nullptr;
            ToJpeg(img.bits(), img.width(), img.height(), 85, "osletek", &jpegSize, &jpegBuf);
            std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
            fp_->BfWrite(jpegBuf, jpegSize);
            free(jpegBuf);
        }
    });
}
