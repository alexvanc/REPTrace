#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#define BUFFLEN 1024
#define SERVER_PORT 8888
#define BACKLOG 5
#define PIDNUMB 3
static void handle_connect(int s_s)
{
    int s_c;                                    /*客?~H?端?~W?~N??~W?~V~G件?~O~O述符*/
    struct sockaddr_in from;                    /*客?~H?端?~\??~]~@*/
    socklen_t len = sizeof(from);

    /*主?~D?~P~F?~G?~K*/
    while(1)
    {
        /*?~N??~T?客?~H?端?~^?~N?*/
        s_c = accept(s_s, (struct sockaddr*)&from, &len);
        time_t now;                             /*?~W??~W?*/
        char buff[BUFFLEN];                     /*?~T??~O~Q?~U??~M??~S?~F??~L?*/
        int n = 0;
        memset(buff, 0, BUFFLEN);               /*?~E?~[?*/
        n = recv(s_c, buff, BUFFLEN,0);     /*?~N??~T??~O~Q?~@~A?~V??~U??~M?*/
        // if(n > 0 && !strncmp(buff, "TIME", 4))  /*?~H??~V??~X??~P??~P~H?~U?~N??~T??~U??~M?*/
        // {
        //     memset(buff, 0, BUFFLEN);           /*?~E?~[?*/
        //     now = time(NULL);                   /*?~S?~I~M?~W??~W?*/
        //     sprintf(buff, "%24s\r\n",ctime(&now));  /*?~F?~W??~W??~M?~H??~E??~S?~F??~L?*/
        //     send(s_c, buff, strlen(buff),0);    /*?~O~Q?~@~A?~U??~M?*/
        // }
        if (n>0){
            printf("server receive 1:%s\n",buff);
        }

        memset(buff, 0, BUFFLEN);
        strcpy(buff, "server send 1\n");                       /*?~M?~H??~O~Q?~@~A?~W符串*/
        send(s_c, buff, strlen(buff), 0); 

        memset(buff, 0, BUFFLEN); 
        n = recv(s_c, buff, BUFFLEN,0);
        if (n>0){
            printf("server receive 2:%s\n",buff);
        }
        memset(buff, 0, BUFFLEN);
        strcpy(buff, "server send 2\n");                       /*?~M?~H??~O~Q?~@~A?~W符串*/
        send(s_c, buff, strlen(buff), 0); 

        memset(buff, 0, BUFFLEN); 
        n = recv(s_c, buff, BUFFLEN,0);
        if (n>0){
            printf("server receive 3:%s\n",buff);
        }
        memset(buff, 0, BUFFLEN);
        strcpy(buff, "server send 3\n");                       /*?~M?~H??~O~Q?~@~A?~W符串*/
        send(s_c, buff, strlen(buff), 0);

             
        /*?~E??~W?客?~H?端*/
        close(s_c);
    }

}


void sig_int(int num)
{
    exit(1);
}
int main(int argc, char *argv[])
{
    int s_s;                                    /*?~\~M?~J??~Y??~W?~N??~W?~V~G件?~O~O述符*/
    struct sockaddr_in local;                   /*?~\??~\??~\??~]~@*/
    signal(SIGINT,sig_int);

    /*建?~KTCP?~W?~N??~W*/
    s_s = socket(AF_INET, SOCK_STREAM, 0);

    /*?~H~]?~K?~L~V?~\??~]~@?~R~L端?~O?*/
    memset(&local, 0, sizeof(local));           /*?~E?~[?*/
    local.sin_family = AF_INET;                 /*AF_INET?~M~O议?~W~O*/
    local.sin_addr.s_addr = htonl(INADDR_ANY);  /*任?~D~O?~\??~\??~\??~]~@*/
    // local.sin_addr.s_addr = htonl(inet_addr("127.0.0.1"));  /*任?~D~O?~\??~\??~\??~]~@*/
    local.sin_port = htons(SERVER_PORT);        /*?~\~M?~J??~Y?端?~O?*/

    /*?~F?~W?~N??~W?~V~G件?~O~O述符?~Q?~Z?~H??~\??~\??~\??~]~@?~R~L端?~O?*/
    bind(s_s, (struct sockaddr*)&local, sizeof(local));
    listen(s_s, BACKLOG);                   /*侦?~P?*/

    /*?~D?~P~F客?~H?端?~^?~N?*/
    pid_t pid[PIDNUMB];
    int i =0;
    for(i=0;i<PIDNUMB;i++)
    {
        pid[i] = fork();
        if(pid[i] == 0)                         /*?~P?~[?~K*/
        {
            handle_connect(s_s);
        }
    }
    while(1);

    close(s_s);

    return 0;
}
