// ft
#include "../lm_def.h"
#include "../lm_util.h"
#include "../sock.h"
#include "../Json.h"
#include <list>

#include<iostream>
using namespace std;

int ft_server;
int ft_ctrl;
int ft_ui;

fd_set set;
int maxfd;

typedef struct lm_task_t
{
    string path;
    string ip; // send who, recv from who
    string type; // send or recv
    string token;
    int filelen;
    int file_trans_len;
    int reference;  // shuliang
}lm_task_t;

list<lm_task_t*> lm_tasks;
list<lm_task_t*> complete_tasks;

void ft_init()
{
    ft_ctrl = create_udp_socket(FT_CTRL_PORT);
    ft_ui = create_udp_socket(FT_UI_PORT);
    ft_server = create_server(FT_SERVER_PORT,"0.0.0.0",250);

    maxfd = ft_server;

    // push select
    FD_ZERO(&set);
    FD_SET(ft_ctrl, &set);
    FD_SET(ft_ui, &set);
    FD_SET(ft_server, &set);
}

void ft_handle_ctrl()
{
    char buf[2048];
    recv(ft_ctrl,buf,sizeof(buf),0);

    Json json;
    json.parse(buf);

    string cmd = json.value(LM_CMD);
    if(cmd == LM_FT)
    {
        string type = json.value(LM_TYPE);
        if(type == LM_SEND)
        {
            string path = json.value(LM_FILEPATH);
            string toip = json.value(LM_RECV);

            string token = json.value(LM_TOKEN);
            int filelen = atoi(json.value(LM_FILELEN).c_str());
            int ref = atoi(json.value(LM_REF).c_str());

            // create a task
            lm_task_t* task = new lm_task_t;
            task->filelen = filelen;
            task->ip = toip;
            task->path = path;
            task->token = token;
            task->type = type;
            task->reference = ref;
            task->file_trans_len = 0;

            printf("task->ref=%d, toip=%s, type=%s\n",ref,task->ip.c_str(),type.c_str());

            lm_tasks.push_back(task);
        }
        else if(type == LM_RECV)
        {
            lm_task_t* task = new lm_task_t;
            task->filelen = atoi(json.value(LM_FILELEN).c_str());
            task->ip = json.value(LM_SEND);
            task->path = json.value(LM_LOCAL_PATH);
            task->token = json.value(LM_TOKEN);
            task->type = type;
            task->reference = 1;
            task->file_trans_len = 0;

            printf("ready to recv file: %s\n", task->path.c_str());
            lm_tasks.push_back(task);
        }
    }
}

void ft_run()
{
    while(1)
    {
        fd_set check_set;
        memcpy(&check_set,&set,sizeof(set));

        struct timeval tv;
        tv.tv_sec = 5;
        tv.tv_usec = 0;
        int ret = select(maxfd + 1,&check_set,NULL,NULL,&tv);
        if(ret > 0)
        {
            if(FD_ISSET(ft_ctrl,&check_set))
            {
                ft_handle_ctrl();
            }

            if(FD_ISSET(ft_ui,&check_set))
            {

            }

            if(FD_ISSET(ft_server,&check_set))
            {

            }
        }
    }
}

int main()
{
    ft_init();
    ft_run();
    return 0;
}
