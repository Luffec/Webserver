#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <unordered_map>
#include <fcntl.h>  //open
#include <unistd.h> //close
#include <sys/stat.h> //stat
#include <sys/mman.h> //mmap,munmap
#include <assert.h>

#include "buffer.h"

class HTTPresponse
{  
public:
    HTTPresponse();
    ~HTTPresponse();

    void init(const std::string& srcDir,std::string& path,bool isKeepAlive=false,int code=-1);
    void makeResponse(Buffer& buffer);//生成响应报文
    void unmapFile_();//共享内存的扫尾
    char* file();//返回文件信息
    size_t fileLen() const;//返回文件信息
    void errorContent(Buffer& buffer,std::string message);//在添加数据体的函数中，如果所请求的文件打不开，返回相应的错误信息
    int code() const {return code_;}//返回状态码

private:
    void addStateLine_(Buffer& buffer);
    void addResponseHeader_(Buffer& buffer);
    void addResponseContent_(Buffer& buffer);

    void errorHTML_();//对于4XX的状态码是分开考虑
    std::string getFileType_();//得到文件类型信息

    int code_;//HTTP状态
    bool isKeepAlive_;

    std::string path_;//解析得到的路径
    std::string srcDir_;//根目录

    char* mmFile_;
    struct stat mmFileStat_;

    static const std::unordered_map<std::string,std::string> SUFFIX_TYPE;//后缀名到文件类型的映射关系
    static const std::unordered_map<int,std::string> CODE_STATUS;//状态码到相应状态(字符串类型)的映射
    static const std::unordered_map<int,std::string> CODE_PATH;//4XX状态码到响应文件路径的映射
};

#endif