#include "SocketWriter.h"

#include <memory.h>

SocketWriter::SocketWriter()
    : BufferWriter()
{
}

SocketWriter::~SocketWriter()
{
    BfClose();
}

bool SocketWriter::BfOpen(const std::string &ip, size_t port)
{
    sock_ = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock_ == -1) {return false;}
    memset(&sa_, 0, sizeof sa_);
    sa_.sin_family = AF_INET;
    sa_.sin_addr.s_addr = inet_addr(ip.c_str());
    sa_.sin_port = htons(port);
    return true;
}

ssize_t SocketWriter::BfWrite(uint8_t *buf, int buf_size)
{
    int total_sent = 0;
    int datagram_size = 8192;
    for(int i =0 ; i < buf_size; i+= datagram_size)
    {
        int bytes_sent = sendto(sock_, buf + i, std::min(datagram_size, buf_size - i), 0,(struct sockaddr*)&sa_, sizeof sa_);
        if(bytes_sent < 1)
        {
            return -1;
            break;
        }
        total_sent += bytes_sent;
        usleep(10000);//osm
    }
    return total_sent;
}

size_t SocketWriter::BfSeek(int64_t offset, int whence)
{
    return true;
}

void SocketWriter::BfClose()
{
    if(sock_)
    {
        close(sock_);
        sock_ = 0;
    }
}
