#include "buffer.h"

Buffer::Buffer(int initBuffersize):buffer_(initBuffersize),readPos_(0),writePos_(0){}

size_t Buffer::readableBytes() const
{
    return writePos_-readPos_;
}

size_t Buffer::writeableBytes() const
{
    return buffer_.size()-writePos_;
}

size_t Buffer::readBytes() const
{
    return readPos_;
}

const char* Buffer::curReadPtr() const
{
    return BeginPtr_()+readPos_;
}
 
const char* Buffer::curWritePtrConst() const
{
    return BeginPtr_()+writePos_;
}

char* Buffer::curWritePtr()
{
    return BeginPtr_()+writePos_;
}

void Buffer::updateReadPtr(size_t len)
{
    assert(len<=readableBytes());
    readPos_+=len;
}

void Buffer::updateReadPtrUntilEnd(const char* end)
{
    assert(end>=curReadPtr());
    updateReadPtr(end-curReadPtr());
}

void Buffer::updateWritePtr(size_t len){
    assert(len<=writeableBytes());
    writePos_+=len;
}

void Buffer::initPtr(){
    bzero(&buffer_[0],buffer_.size());
    readPos_=0;
    writePos_=0;
}

void Buffer::allocateSpace(size_t len)
{
    //如果buffer_里面剩余的空间有len就进行调整，否则需要申请空间
    //剩余空间包括write指针之前的空间和可写的空间
    if(writeableBytes()+readableBytes()<len)
    {
        buffer_.resize(writePos_+len+1);
    }
    else
    {
        //将读指针置为0，调整一下
        size_t readable=readableBytes();
        std::copy(BeginPtr_()+readPos_,BeginPtr_()+writePos_,BeginPtr_());
        readPos_=0;
        writePos_=readable;
        assert(readable==readableBytes());
    }
}

void Buffer::ensureWriteable(size_t len)
{
    if(writeableBytes()<len)
    {
        allocateSpace(len);
    }
    assert(writeableBytes()>=len);
}

void Buffer::append(const char* str,size_t len)
{
    assert(str);
    ensureWriteable(len);
    std::copy(str,str+len,curWritePtr());
    updateWritePtr(len);
}

void Buffer::append(const std::string& str)
{
    append(str.data(),str.length());
}

void Buffer::append(const void* data,size_t len)
{
    assert(data);
    append(static_cast<const char*>(data),len);
}

void Buffer::append(const Buffer& buffer)
{
    append(buffer.curReadPtr(),buffer.readableBytes());
}

ssize_t Buffer::readFd(int fd,int* Errno)//从fd中读出来写入到我们的缓冲区中，即writebuffer
{
    char buff[65525];//暂时的缓冲区
    struct iovec iov[2];//struct iovec 结构体定义了一个向量元素，通常这个 iovec 结构体用于一个多元素的数组，
                        //对于每一个元素，iovec 结构体的字段 iov_base 指向一个缓冲区，这个缓冲区存放的是网络接收的数据（read），
                        //或者网络将要发送的数据（write）。iovec 结构体的字段 iov_len 存放的是接收数据的最大长度（read），或者实际写入的数据长度（write）。
    const size_t writable=writeableBytes();

    iov[0].iov_base=BeginPtr_()+writePos_;
    iov[0].iov_len=writable;
    iov[1].iov_base=buff;
    iov[1].iov_len=sizeof(buff);

    const ssize_t len=readv(fd,iov,2);//readv和writev的第一个参数fd是个文件描述符，第二个参数是指向iovec数据结构的一个指针，
                                      //其中iov_base为缓冲区首地址，iov_len为缓冲区长度，参数iovcnt指定了iovec的个数。
    if(len<0)
    {
        std::cout<<"从fd读取数据失败"<<std::endl;
        *Errno=errno;
    }
    else if(static_cast<size_t>(len)<=writable)
    {
        writePos_+=len;
    }
    else
    {
        writePos_=buffer_.size();
        append(buff,len-writable);
    }
    return len;
}

ssize_t Buffer::writeFd(int fd,int* Errno)
{
    size_t readSize=readableBytes();
    ssize_t len=write(fd,curReadPtr(),readSize);
    if(len<0)
    {
        std::cout<<"往fd写入数据失败"<<std::endl;
        *Errno=errno;
        return len;
    }
    readPos_+=len;
    return len;
}

std::string Buffer::AlltoStr()
{
    std::string str(curReadPtr(),readableBytes());
    initPtr();
    return str;
}

char* Buffer::BeginPtr_()
{
    return &*buffer_.begin();
}

const char* Buffer::BeginPtr_() const
{
    return &*buffer_.begin();
}