// ctrl
#include <stdio.h>
#include "../lm_def.h"
#include "../Json.h"
#include "../sock.h"
#include "../lm_util.h"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <map>
#include <list>
#include <iostream>
using namespace std;

int epollfd;
int ctrl_ui;
int ctrl_ft;
int ctrl_other;
string myname;

list<string> ips;

typedef struct user_t
{
    string name;
    string ip;
}user_t;

static map<string,user_t*> users;

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

void ctrl_add_user(string ip,string name)
{
    user_t* user = new user_t;
    user->ip= ip;
    user->name = name;
    users[ip] = user;
}

user_t* ctrl_find_user(string ip)
{
    auto it = users.find(ip);
    if(it == users.end())
        return NULL;
    return it->second;
}

void ctrl_init()
{
    ips = get_ip_addrs();
    srand(time(NULL));

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

void ctrl_send(Json& json,uint16_t port, string ip = "255.255.255.255")
{
    string buf = json.print();
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip.c_str()); // guangbo

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
        ctrl_send(json,CTRL_OTHER_PORT);
    }
    else if(cmd == LM_LIST)
    {
        for(map<string,user_t*>::iterator it = users.begin();it!=users.end();++it)
        {
            user_t* user = it->second;
            Json resp;
            resp.add(LM_CMD,LM_LIST_ACK);
            resp.add(LM_IP,user->ip);
            resp.add(LM_NAME,user->name);
            ctrl_send(resp,UI_CTRL_PORT,"127.0.0.1");
        }
    }
    else if(cmd == LM_TO)
    {
        string toip = json.value(LM_RECV);
        json.add(LM_FROM_NAME,myname);

        ctrl_send(json,CTRL_OTHER_PORT,toip);
    }
    else if(cmd == LM_FT)
    {
        ctrl_send(json,FT_CTRL_PORT,"127.0.0.1");
    }
    else if(cmd == LM_SENDFILE)
    {
        string toip = json.value(LM_RECV);
        string path = json.value(LM_FILEPATH);

        struct stat stat_buf;
        stat(path.c_str(),&stat_buf);

        int token = rand();
        Json task_json;
        task_json.add(LM_CMD,LM_FT);
        task_json.add(LM_FILEPATH,path);
        task_json.add(LM_FILELEN,(const char*)(int)stat_buf.st_size);
        task_json.add(LM_RECV,toip);
        task_json.add(LM_TOKEN,(const char*)token);

        if(toip == "255.255.255.255")
        {
            task_json.add(LM_REF,(const char*)users.size());
        }
        else
        {
            task_json.add(LM_REF,(const char*)1);
        }

        ctrl_send(task_json,FT_CTRL_PORT,"127.0.0.1");

        usleep(1000);

        // tell get file user
        Json notify_json;
        notify_json.add(LM_CMD,LM_FT);
        notify_json.add(LM_TOKEN,(const char*)token);
        notify_json.add(LM_FILELEN,(const char*)(int)stat_buf.st_size);
        notify_json.add(LM_FROM_NAME,myname);
        ctrl_send(notify_json,CTRL_OTHER_PORT,toip);
    }
}
void ctrl_handle_ft(){}
void ctrl_handle_other()
{
    struct sockaddr_in addr;
    socklen_t socklen = sizeof(addr);
    char buf[2048];
    recvfrom(ctrl_other,buf,sizeof(buf),0,(struct sockaddr*)&addr,&socklen);
    string ip = inet_ntoa(addr.sin_addr);
    printf("ip=%s\n",ip.c_str());

    // if is me, return
    if(find(ips.begin(),ips.end(),ip)!=ips.end())
        return;

    Json json;
    json.parse(buf);

    // get cmd
    string cmd = json.value(LM_CMD);
    if(cmd == LM_TO)
    {
        string msg = json.value(LM_MSG);
        string by = json.value(LM_FROM_NAME);

        Json tocli;
        tocli.add(LM_CMD,LM_MSG);
        tocli.add(LM_FROM_NAME,by);
        tocli.add(LM_FROM_IP,ip);
        tocli.add(LM_MSG,msg);

        ctrl_send(tocli,UI_CTRL_PORT,"127.0.0.1");
    }
    else if(cmd == LM_SETNAME)
    {
        // have new user
        string name = json.value(LM_NAME);

        user_t* user = ctrl_find_user(ip);
        if(user == NULL)
        {
            // add user
            ctrl_add_user(ip,name);

            // write me to him
            Json json_resp;
            json_resp.add(LM_CMD,LM_SETNAME_ACK);
            json_resp.add(LM_NAME,myname);
            ctrl_send(json_resp,CTRL_OTHER_PORT,ip);
        }
        else
        {
            user->name = name;
        }
    }
    else if(cmd == LM_SETNAME_ACK)
    {
        string name = json.value(LM_NAME);

        user_t* user = ctrl_find_user(ip);
        if(user == NULL)
        {
            ctrl_add_user(ip,name);
        }
        else
        {
            user->name=name;
        }
    }
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
