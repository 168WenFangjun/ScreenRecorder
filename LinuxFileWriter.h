#pragma once

#include "BufferWriter.h"

class LinuxFileWriter : public BufferWriter
{
public:
    LinuxFileWriter();
    virtual ~LinuxFileWriter();
    virtual bool BfOpen(const std::string &file, size_t) override;
    virtual ssize_t BfWrite(uint8_t *buf, int buf_size) override;
    virtual size_t BfSeek(int64_t offset, int whence) override;
    virtual void BfClose() override;
protected:
    std::string bw_file_;
    int bw_fp_;
};
