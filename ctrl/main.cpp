// ctrl
#include <stdio.h>
#include "../lm_def.h"
#include "../Json.h"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include<iostream>
using namespace std;

int epollfd;
int ctrl_ui;
int ctrl_ft;
int ctrl_other;
string myname;

int ctrl_creat_socket(uint16_t port)
{
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in addr;
    addr.sin_addr.s_addr = 0;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    bind(fd, (struct sockaddr*)&addr, sizeof(addr));
    return fd;
}

void ctrl_init()
{
    // create epoll
    epollfd = epoll_create(512);

    ctrl_ui = ctrl_creat_socket(CTRL_UI_PORT);
    ctrl_ft = ctrl_creat_socket(CTRL_FT_PORT);
    ctrl_other = ctrl_creat_socket(CTRL_OTHER_PORT);

    // set guangbo to other
    int optval = 1;
    setsockopt(ctrl_other,SOL_SOCKET,SO_BROADCAST,&optval,sizeof(optval));

    struct epoll_event ev;
    ev.data.fd = ctrl_ui;
    ev.events = EPOLLIN;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, ctrl_ui, &ev);

    ev.data.fd = ctrl_ft;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, ctrl_ft, &ev);

    ev.data.fd = ctrl_other;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, ctrl_other, &ev);
}

void ctrl_broadcast(Json& json)
{
    string buf = json.print();
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(CTRL_OTHER_PORT);
    addr.sin_addr.s_addr = inet_addr("255.255.255.255"); // guangbo

    // send everyone
    sendto(ctrl_other,buf.c_str(),buf.size(),0,(struct sockaddr*)&addr,sizeof(addr));
}

void ctrl_handle_ui()
{
    char buf[2048];
    recv(ctrl_ui,buf,sizeof(buf),0);

    Json json;
    json.parse(buf); // parse json

    string cmd = json.value(LM_CMD);
    if(cmd == LM_SETNAME)
    {
        string name = json.value(LM_NAME);
        myname = name;
        printf("setname is %s\n",name.c_str());

        // save the name
        // broadcast my name
        ctrl_broadcast(json);
    }
}
void ctrl_handle_ft(){}
void ctrl_handle_other()
{
    char buf[2048];
    recv(ctrl_other,buf,sizeof(buf),0);

    printf("recv from other:%s\n",buf);
}

void ctrl_run()
{
    struct epoll_event ev;
    while(1)
    {
        int ret = epoll_wait(epollfd,&ev,1,5000);
        if(ret > 0)
        {
            if(ev.data.fd == ctrl_ui)
            {
                ctrl_handle_ui();
            }
            else if(ev.data.fd == ctrl_ft)
            {
                ctrl_handle_ft();
            }
            else if(ev.data.fd == ctrl_other)
            {
                ctrl_handle_other();
            }
        }
    }
}

int main()
{
    ctrl_init();
    ctrl_run();
    return 0;
}
