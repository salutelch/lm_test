// cli
#include <netinet/in.h>
#include <sys/epoll.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <string.h>

#include "../lm_def.h"
#include "../Json.h"
#include<iostream>

int epollfd;
int ui_ctrl;
int ui_ft;

void ui_init()
{
    // create epoll
    epollfd = epoll_create(512);
    // socket ctrl
    ui_ctrl = socket(AF_INET,SOCK_DGRAM, 0);
    struct sockaddr_in addr;
    addr.sin_addr.s_addr = 0;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(UI_CTRL_PORT);
    // bind
    bind(ui_ctrl, (struct sockaddr*)&addr, sizeof(addr));
    // ft socket
    ui_ft = socket(AF_INET,SOCK_DGRAM,0);
    addr.sin_port = htons(UI_FT_PORT);
    bind(ui_ft, (struct sockaddr*)&addr, sizeof(addr));

    // add socket in epoll
    struct epoll_event ev;
    ev.data.fd = ui_ctrl;
    ev.events = EPOLLIN;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, ui_ctrl, &ev);

    ev.data.fd = ui_ft;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, ui_ft, &ev);

    ev.data.fd = 0;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, 0, &ev);
}

void ui_send_ctrl(Json& obj)
{
    string packet = obj.print();
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(CTRL_UI_PORT);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    printf("ui send ctrl:%s\n",packet.c_str());
    sendto(ui_ctrl,packet.c_str(),packet.size(),0,(struct sockaddr*)&addr,sizeof(addr));
}

void ui_handle_cmd(char *buf)
{
    char* cmd = strtok(buf," ");
    if(strcmp(cmd, LM_SETNAME) == 0)
    {
        Json obj; // make json
        obj.add(LM_CMD, LM_SETNAME);
        obj.add(LM_NAME, strtok(NULL,"\0"));

        ui_send_ctrl(obj);
    }
}

void ui_handle_user_input()
{
    char cmd[1024];
    fgets(cmd,sizeof(cmd),stdin);
    cmd[strlen(cmd)-1] = 0;  // add \0
    if(strlen(cmd) == 0)
    {
        return;
    }
    printf("recv cmd %s\n",cmd);
    ui_handle_cmd(cmd);
}

void ui_handle_msg_from_ctrl()
{}

void ui_handle_msg_from_ft()
{}

void ui_run()
{
    struct epoll_event ev;
    while(1)
    {
        int ret = epoll_wait(epollfd, &ev, 1, 5000);
        if(ret > 0)
        {
            if(ev.data.fd == 0)
            {
                ui_handle_user_input();
            }
            else if(ev.data.fd == ui_ctrl)
            {
                ui_handle_msg_from_ctrl();
            }
            else if(ev.data.fd == ui_ft)
            {
                ui_handle_msg_from_ft();
            }
        }
    }
}

int main()
{
    ui_init();
    ui_run();

    return 0;
}
