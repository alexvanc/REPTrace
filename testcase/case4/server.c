//
// Created by xueping on 2017/4/6.
//
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define	UDP_TEST_PORT 50001

int main(int argC, char* arg[])
{
    struct sockaddr_in addr;
    int sockfd, len = 0;
    int addr_len = sizeof(struct sockaddr_in);
    char buffer[256];

    /* 建立socket，注意必须是SOCK_DGRAM */
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror ("socket");
        exit(1);
    }

    /* 填写sockaddr_in 结构 */
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(UDP_TEST_PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);// 接收任意IP发来的数据

    /* 绑定socket */
    if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr))<0) {
        perror("connect");
        exit(1);
    }

    while(1) {
        bzero(buffer, sizeof(buffer));
        len = recvfrom(sockfd, buffer, sizeof(buffer), 0,
                       (struct sockaddr *)&addr ,&addr_len);
        /* 显示client端的网络地址和收到的字符串消息 */
        printf("Received a string from client %s, string is: %s\n",
               inet_ntoa(addr.sin_addr), buffer);
        /* 将收到的字符串消息返回给client端 */
        sendto(sockfd, buffer, len, 0, (struct sockaddr *)&addr, addr_len);
    }

    return 0;
}

// ----------------------------------------------------------------------------
// End of udp_server.c
