#include "LinuxFileWriter.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

LinuxFileWriter::LinuxFileWriter()
    : BufferWriter()
    , bw_fp_(0)
{

}

LinuxFileWriter::~LinuxFileWriter()
{

}

bool LinuxFileWriter::BfOpen(const std::string &file, size_t)
{
    bw_fp_ = open(file.c_str(), O_WRONLY | O_CREAT, 0644);
    if(bw_fp_ != -1)
    {
        bw_file_ = file;
        return true;
    }
    return false;
}

ssize_t LinuxFileWriter::BfWrite(uint8_t *buf, int buf_size)
{
    return write(bw_fp_, buf, buf_size);
}

size_t LinuxFileWriter::BfSeek(int64_t offset, int whence)
{
    return lseek(bw_fp_, offset, whence);
}

void LinuxFileWriter::BfClose()
{
    close(bw_fp_);
    bw_fp_ = 0;
    bw_file_ = "";
}
