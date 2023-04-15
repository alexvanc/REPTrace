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
#include <arpa/inet.h>

#define BUFFLEN 1024
#define SERVER_PORT 8888
#define BACKLOG 5
#define PIDNUMB 3

char * consumers[] = {"10.211.55.34", "10.211.55.37"};

static char message[BUFFLEN];
static char message_consumer[BUFFLEN];
char *wrong_message="Sorry,there is no message for you";

char * get_consumer(){
    int i,number;
    srand((unsigned) time(NULL)); 
    number= rand() % 2;
    return consumers[number];
}

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
        unsigned short int on_port;
        char *on_ip;
        on_port=ntohs(from.sin_port);
        on_ip=inet_ntoa(from.sin_addr);

        if(strcmp(on_ip,consumers[0])==0||strcmp(on_ip,consumers[1])==0){
            char buff2[BUFFLEN];
            memset(buff2, 0, BUFFLEN);  
            if(strlen(message_consumer)!=0&&strcmp(on_ip,message_consumer)==0){
                strcpy(buff2, message);                       /*?~M?~H??~O~Q?~@~A?~W符串*/
                /*?~O~Q?~@~A?~U??~M?*/
                send(s_c, buff2, strlen(buff2), 0);
                memset(buff2, 0, BUFFLEN);    
                printf("push message to consumer:%s\t%s\n",message_consumer,message);

                memset(message_consumer,0,BUFFLEN);
            }else{
                printf("%s\t%s\t%d\n",on_ip,message_consumer,strcmp(on_ip,message_consumer) );
                memset(buff2, 0, BUFFLEN);                   /*?~E?~[?*/
                strcpy(buff2, wrong_message);                       /*?~M?~H??~O~Q?~@~A?~W符串*/
                /*?~O~Q?~@~A?~U??~M?*/
                send(s_c, buff2, strlen(buff), 0);
                memset(buff2, 0, BUFFLEN); 
            }

        }else{
            printf("%s\t%s\n", "producer1",message_consumer);
            int n = 0;
            char *tmp_consumer;
            memset(buff, 0, BUFFLEN);               /*?~E?~[?*/
            n = recv(s_c, buff, BUFFLEN,0);
             /*?~N??~T??~O~Q?~@~A?~V??~U??~M?*/  
            if (n>0){ //push message to a consumer
                tmp_consumer=get_consumer();
                strcpy(message, buff);
                strcpy(message_consumer,tmp_consumer);  
                printf("%s\t%s\t%s\n", "producer2",message_consumer,tmp_consumer);     
            }
        }
        

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
    handle_connect(s_s);
    // while(1);

    close(s_s);

    return 0;
}
