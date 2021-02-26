#pragma once

#include <future>
#include <memory>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <QList>

class QScreen;
class SocketWriter;
class ScreenStreamer
{
public:
    ScreenStreamer();
    ~ScreenStreamer();
    void StartStreaming();
protected:
    void InitAvailableScreens();
    int ActiveScreen() const;

    std::unique_ptr<SocketWriter> fp_;
    std::future<void> thread_;
    QList<QScreen *> m_screens;
    bool stop_ = false;
};
