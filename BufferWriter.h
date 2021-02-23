#pragma once

#include <string>

class BufferWriter
{
public:
    BufferWriter() {}
    virtual ~BufferWriter() {}
    virtual bool BfOpen(const std::string &, size_t) = 0;
    virtual ssize_t BfWrite(uint8_t *buf, int buf_size) = 0;
    virtual size_t BfSeek(int64_t offset, int whence) = 0;
    virtual void BfClose() = 0;
};
