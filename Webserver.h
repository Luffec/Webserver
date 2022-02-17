#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <unordered_map>
#include <fcntl.h>       // fcntl()
#include <unistd.h>      // close()
#include <assert.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <functional>

#include "Epoller.h"
#include "timer.h"
#include "threadpool.h"
#include "HTTPconnection.h"

class Webserver
{
public:
    Webserver(int port,int trigMode,int timeoutMS,bool optLinger,int threadNum);
    ~Webserver();

    void Start();

private:
    //对服务端的socket进行设置，最后可以得到listenFd
    bool initSocket_();

    void initEventMode_(int trigMode);

    void addClientConnection(int fd,sockaddr_in addr);//添加一个HTTP连接
    void closeConn_(HTTPconnection* client);         //关闭一个HTTP连接

    void handleListen_();
    void handleWrite_(HTTPconnection* client);
    void handleRead_(HTTPconnection* client);

    void onRead_(HTTPconnection* client);
    void onWrite_(HTTPconnection* client);
    void onProcess_(HTTPconnection* client);

    void sendError_(int fd,const char* info);
    void extentTime_(HTTPconnection* client);

    static const int MAX_FD=65536;
    static int setFdNonblock(int fd);

    int port_;
    int timeoutMS_;
    bool isClose_;
    int listenFd_;
    bool openLinger_;
    char* srcDir_;//获取需要的路径

    uint32_t listenEvent_;
    uint32_t connectionEvent_;

    std::unique_ptr<TimerManager> timer_;
    std::unique_ptr<ThreadPool> threadpool_;
    std::unique_ptr<Epoller> epoller_;
    std::unordered_map<int,HTTPconnection> users_;
};

#endif