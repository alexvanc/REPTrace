#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <netdb.h>
#define BUFFLEN 1024
#define SERVER_PORT 8888
int main(int argc, char *argv[])
{
    int s;                                      /*?~\~M?~J??~Y??~W?~N??~W?~V~G件?~O~O述符*/
    struct sockaddr_in server;                  /*?~\??~\??~\??~]~@*/
    char buff[BUFFLEN];                         /*?~T??~O~Q?~U??~M??~S?~F??~L?*/
    int n = 0; 

    char *message="I am a message from consumer\n";

    struct hostent *he;
    he = gethostbyname("10.211.55.36");                                 /*?~N??~T??~W符串?~U?度*/

    /*建?~KTCP?~W?~N??~W*/
    s = socket(AF_INET, SOCK_STREAM, 0);

    /*?~H~]?~K?~L~V?~\??~]~@*/
    memset(&server, 0, sizeof(server));     /*?~E?~[?*/
    server.sin_family = AF_INET;                /*AF_INET?~M~O议?~W~O*/
    // server.sin_addr.s_addr = htonl(INADDR_ANY);/*任?~D~O?~\??~\??~\??~]~@*/
    // sesrver.sin_addr.s_addr = htonl(inet_addr("127.0.0.1"));/*任?~D~O?~\??~\??~\??~]~@*/
    server.sin_addr=*((struct in_addr *)he->h_addr);
    server.sin_port = htons(SERVER_PORT);       /*?~\~M?~J??~Y?端?~O?*/

    /*?~^?~N??~\~M?~J??~Y?*/
    connect(s, (struct sockaddr*)&server,sizeof(server));
    memset(buff, 0, BUFFLEN);                   /*?~E?~[?*/
    strcpy(buff, message);                       /*?~M?~H??~O~Q?~@~A?~W符串*/
    /*?~O~Q?~@~A?~U??~M?*/
    send(s, buff, strlen(buff), 0);
    memset(buff, 0, BUFFLEN);                   /*?~E?~[?*/
    /*?~N??~T??~U??~M?*/
    // n = recv(s, buff, BUFFLEN, 0);
    // /*?~I~S?~M??~H?~A?*/
    // if(n >0){
    //     printf("client receive 1:%s\n",buff);
    // }

    close(s);

    return 0;
}
