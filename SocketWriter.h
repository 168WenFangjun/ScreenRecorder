#pragma once

#include "BufferWriter.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

class SocketWriter : public BufferWriter
{
public:
    SocketWriter();
    virtual ~SocketWriter();
    virtual bool BfOpen(const std::string &ip, size_t port) override;
    virtual ssize_t BfWrite(uint8_t *buf, int buf_size) override;
    virtual size_t BfSeek(int64_t offset, int whence) override;
    virtual void BfClose() override;
protected:
    int sock_;
    struct sockaddr_in sa_;
};
