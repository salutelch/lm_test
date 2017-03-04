#ifndef __SOCK_H__
#define __SOCK_H__

#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
static int connect_server(const char* ip, unsigned short port)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);

    int ret = connect(fd, (struct sockaddr*)&addr, sizeof(addr));
    if(ret < 0)
    {
        close(fd);
        return -1;
    }
    return fd;
}

static int create_server(unsigned short port, const char* ip, int backlog)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd < 0)
    {
        perror("socket");
        return fd;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip); // 监听的ip地址

    int ret = bind(fd, (struct sockaddr*)&addr, sizeof(addr));
    if(ret < 0)
    {
        perror("bind");
        return -2;
    }

    listen(fd, backlog);

    return fd;
}

static int doAccept(int fd, struct sockaddr* addr, socklen_t* addrlen){
    while(1)
    {
        int newfd = accept(fd, addr, addrlen);
        if(newfd < 0 && errno == EINTR)
        {
            continue;
        }

        return newfd;
    }
    return -1;
}

// 这个函数要读取size个字节再返回
static int doRecv(int fd, char* buf, int size)
{
    int alreadyRead = 0;
    while(size > 0)
    {
        int ret = read(fd, buf + alreadyRead, size);
        if(ret > 0)
        { 
            size -= ret;
            alreadyRead += ret;
        }
        else if(ret == 0) // 对方关闭socket
        {
            break;
        }
        else if(ret < 0)
        {
            if(errno == EINTR)
                continue;
            break;
        }
    }

    return alreadyRead;
}

static int doSend(int fd, const char* buf, int size)
{
    int alreadySend = 0;
    while(size > 0)
    {
        int ret = write(fd, buf + alreadySend, size);
        if(ret > 0)
        {
            alreadySend += ret;
            size -= ret;
        }
        else 
        {
            if(errno == EINTR)
                continue;
            break;
        }
    }

    return alreadySend;
}

static void set_nonblock(int fd)
{
    uint32_t flags = fcntl(fd, F_GETFL);
    flags |= O_NONBLOCK;
    fcntl(fd, F_SETFL, flags);
}

#endif
#if 0
// 简单容易懂，代码太多不好维护，效率低
int add()
{
    int i=0;
    i = i+1;
    i = i+2;
    i = i+3;

    return i;
}

// 简单容易懂，维护容易，效率低
int add()
{
    result = 0;
    for(i=0; i<100; ++i)
        result += i;
}

// 效率高，没有啥维护性，效率高
int add()
{
    return 5050;
}

// 效率高，维护性很好，效率高
int add()
{
    return i*(i+1)/2;
}
#endif





















