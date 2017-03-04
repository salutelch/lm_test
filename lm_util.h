#ifndef LM_UTIL_H
#define LM_UTIL_H


#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

#include <list>
#include <string>
using namespace  std;

static int create_udp_socket(uint16_t port)
{
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = 0;
    bind(fd, (struct sockaddr*)&addr, sizeof(addr));
    return fd;
}

static list<string> get_ip_addrs()
{
    list<string> ret;
    FILE* fp = popen("ifconfig | grep inet | grep -v inet6 | awk '{print $2}'| awk -F \":\" '{print $2}'", "r");
    char buf[1024];
    while(fgets(buf, sizeof(buf), fp))
    {
        buf[strlen(buf)-1] = 0;
        ret.push_back(buf);
        printf("ip_list: %s\n", buf);
    }
   // fclose(fp);
    pclose(fp);

    return ret;
}


#endif // LM_UTIL_H
