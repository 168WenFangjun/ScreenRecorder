#pragma once

#include <QImage>
#include <condition_variable>
#include <future>
#include <mutex>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

#include "libavutil/avutil.h"

struct AVFrame;
struct Decoder;
class SocketReader
{
public:
    SocketReader();
    ~SocketReader();
    void RecieveData();
    int Readfn(uint8_t *buf, int buf_size);
    int64_t Seekfn(int64_t offset, int whence);
    bool Playback(const std::string &output_file, std::function<void(const QImage&img)> renderImageCb);
    int GotFrame(AVFrame* frame, enum AVMediaType media_type);
    QImage AvframeToQImage(const AVFrame *pFrame) const;

    Decoder* decoder_ = nullptr;
protected:
    int fp_;
    struct sockaddr_in sa_;
    std::future<void> reader_thread_;
    std::future<void> playback_thread_;
    std::mutex m_;
    std::condition_variable cv_;
    std::vector<uint8_t> buffer_;
    std::function<void(const QImage&img)> renderImageCb_ = nullptr;
    bool stop_ = false;
};
