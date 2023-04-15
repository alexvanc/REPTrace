#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <netdb.h>

#define BUFFLEN 1024
#define SERVER_PORT 8888
#define BACKLOG 5
#define PIDNUMB 3

char * subscribers[] = {"10.211.55.34", "10.211.55.37"};

char message[BUFFLEN];


static void handle_connect(int s_s)
{
    int s_c;                                    /*客?~H?端?~W?~N??~W?~V~G件?~O~O述符*/
    struct sockaddr_in from;                    /*客?~H?端?~\??~]~@*/
    socklen_t len = sizeof(from);
    char buff[BUFFLEN]; 

    /*主?~D?~P~F?~G?~K*/
    while(1)
    {
        /*?~N??~T?客?~H?端?~^?~N?*/
        s_c = accept(s_s, (struct sockaddr*)&from, &len);
        int n = 0;
        memset(buff, 0, BUFFLEN);               /*?~E?~[?*/
        n = recv(s_c, buff, BUFFLEN,0);     /*?~N??~T??~O~Q?~@~A?~V??~U??~M?*/
        strcpy(message, buff);  
        if (n>0){ //push message to a subscriber
            char * subscriber=subscribers[0];
            char buff2[BUFFLEN]; 
            int s;  
                
            struct sockaddr_in server;                  
            struct hostent *he;
            he = gethostbyname(subscriber);                                 /*?~N??~T??~W符串?~U?度*/

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
            memset(buff2, 0, BUFFLEN);                   /*?~E?~[?*/
            strcpy(buff2, message);                       /*?~M?~H??~O~Q?~@~A?~W符串*/
            /*?~O~Q?~@~A?~U??~M?*/
            send(s, buff2, strlen(buff2), 0);
            memset(buff2, 0, BUFFLEN);    
            printf("push message to subscriber:%s\t%s\n",subscriber,message);
            close(s);
            //the second subscriber
            subscriber=subscribers[1];
            // int s;  
                
            // struct sockaddr_in server;                  
            // struct hostent *he;
            he = gethostbyname(subscriber);                                 /*?~N??~T??~W符串?~U?度*/

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
            memset(buff2, 0, BUFFLEN);                   /*?~E?~[?*/
            strcpy(buff2, message);                       /*?~M?~H??~O~Q?~@~A?~W符串*/
            /*?~O~Q?~@~A?~U??~M?*/
            send(s, buff2, strlen(buff2), 0);
            memset(buff2, 0, BUFFLEN);    
            printf("push message to subscriber:%s\t%s\n",subscriber,message);
            close(s);      


        }

        memset(buff, 0, BUFFLEN);
        
             

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
    handle_connect(s_s);
  
    // while(1);

    close(s_s);

    return 0;
}
