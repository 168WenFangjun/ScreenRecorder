#include "SocketReader.h"

#include <QImage>
#include <fcntl.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>

extern "C" void mylog(const char *fmt, ...);

//osm
#include "Playback/ffplay3.h"

namespace  {

struct DecoderOneTimeInitDeinit
{
    DecoderOneTimeInitDeinit() {
        decoder_register();
    }
    ~DecoderOneTimeInitDeinit() {
        uninit_opts();
    }
};
static const DecoderOneTimeInitDeinit decoderOneTimeInitDeinit;
static int gotframe(void* cb_data, enum AVMediaType media_type, AVFrame* frame, int64_t dts, int pkt_size)
{
    SocketReader *pthis = (SocketReader*)cb_data;
    return pthis->GotFrame(frame, media_type);
}

//osm
static int readfn(void *opaque, uint8_t *buf, int buf_size)
{
    SocketReader *pthis = (SocketReader*)opaque;
    return pthis->Readfn(buf, buf_size);
}

static int64_t seekfn(void *opaque, int64_t offset, int whence)
{
    SocketReader *pthis = (SocketReader*)opaque;
    return pthis->Seekfn(offset, whence);
}
}//namespace


SocketReader::SocketReader()
{
    memset(&sa_, 0, sizeof sa_);
    sa_.sin_family = AF_INET;
    sa_.sin_addr.s_addr = htonl(INADDR_ANY);
    sa_.sin_port = htons(9000);
    fp_ = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (bind(fp_, (struct sockaddr *)&sa_, sizeof sa_) == -1)
    {
      close(fp_);
      exit(-1);
    }

}

SocketReader::~SocketReader()
{
    stop_ = true;
    reader_thread_.wait();
    playback_thread_.wait();
}

void SocketReader::RecieveData()
{
    int buf_size = 8192;
    static uint8_t * buf = (uint8_t*) malloc(buf_size);

    static FILE* fp=fopen("/home/dev/oosman/Documents/foo2.mkv", "wb");
    reader_thread_ = std::async(std::launch::async, [this, buf_size](){
        while(!stop_)
        {
            socklen_t fromlen = sizeof IN_CLASSA_HOST;
            ssize_t recsize = recvfrom(fp_, (void*)buf, buf_size, 0, (struct sockaddr*)&sa_, &fromlen);

//            mylog("recvd %i bytes", recsize);

            {
                std::lock_guard<std::mutex> lk(m_);
                for(int i = 0; i < recsize; ++i)
                {
                    buffer_.push_back(buf[i]);
                }
            }
            cv_.notify_one();

            fwrite((void*)buf, recsize, 1, fp);
            fflush(fp);

            static size_t total = 0;
            total += recsize;
            mylog("total recvd: %i", total);
        }
    });
}

int64_t SocketReader::Seekfn(int64_t offset, int whence)
{
    if(decoder_->fp_)
    {
        return lseek(decoder_->fp_, offset, whence);
    }
    return offset;
}

int SocketReader::Readfn(uint8_t *buf, int buf_size)
{
    if(decoder_->fp_)
    {
        return read(decoder_->fp_, buf, buf_size);
    }

    std::unique_lock<std::mutex> lk(m_);
    cv_.wait(lk, [this]{
        return (buffer_.size() > 0);
    });
    int i;
    for(i = 0; i < buf_size && buffer_.size() > 0; ++i)
    {
        buf[i] = buffer_.at(0);
        buffer_.erase(buffer_.begin());
    }
    return i;
}

bool SocketReader::Playback(const std::string &output_file, std::function<void(const QImage&img)> renderImageCb)
{
    if(decoder_)
    {
        decoder_close(decoder_);
        delete decoder_;
    }
    decoder_ = new Decoder;
    decoder_->filename = strdup(output_file.c_str());
    decoder_->fp_ = open(output_file.c_str(), O_RDONLY);
    decoder_->readfn_ = readfn;
    decoder_->seekfn_ = seekfn;
    decoder_->gotFrameCb = gotframe;
    decoder_->user_data = this;
    if(decoder_open(decoder_) != 0)
    {
        exit (-1);
        return false;
    }

    close(decoder_->fp_);
    decoder_->fp_ = 0;
    renderImageCb_ = renderImageCb;
    playback_thread_ = std::async(std::launch::async, [this](){
        int seek_ms = 0;
        decoder_play(decoder_, seek_ms);
    });
    return true;
}

int SocketReader::GotFrame(AVFrame* frame, enum AVMediaType media_type)
{
    if(!frame)
    {
        return 0;
    }
    if(media_type != AVMEDIA_TYPE_VIDEO)
    {
        return 0;
    }
    if(frame->pict_type != AV_PICTURE_TYPE_NONE &&
            frame->width > 0 &&
            frame->height > 0)
    {
        QImage image = AvframeToQImage(frame);
        renderImageCb_(image);
//        static int i = 0;
//        char filename[100];
//        sprintf(filename, "/tmp/foo%03d.jpg", i++);
//        image.save(filename);
    }
    av_frame_free(&frame);
    return !stop_;
};

QImage SocketReader::AvframeToQImage(const AVFrame *pFrame) const
{
    QImage image = QImage(pFrame->width, pFrame->height, QImage::Format_RGB32);

    int stream_index = decoder_->st_index[AVMEDIA_TYPE_VIDEO];
    SwsContext *sws_ctx = sws_getContext (
                pFrame->width,
                pFrame->height,
                decoder_->avctx[stream_index]->pix_fmt,
                pFrame->width,
                pFrame->height,
                AV_PIX_FMT_RGB32,
                SWS_BICUBIC,
                NULL, NULL, NULL );

    uint8_t *dstSlice[] = { image.bits() };
    int dstStride = image.width() * 4;
    int height = sws_scale( sws_ctx, pFrame->data, pFrame->linesize,
                0, pFrame->height, dstSlice, &dstStride );
    sws_freeContext(sws_ctx);

    return image;

//    char outfile[32];
//    sprintf(outfile, "out%02i.bmp", frameNum);
//    qimg.save(outfile);
}
