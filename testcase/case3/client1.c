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
 
    struct hostent *he;
    he = gethostbyname("10.0.0.41");                                 /*?~N??~T??~W符串?~U?度*/
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
    while(connect(s, (struct sockaddr*)&server,sizeof(server))==-1){
		printf("Connect Error!\n");
	}
	//printf("Connect Success!\n");
                   
    memset(buff, 0, BUFFLEN);
    strcpy(buff, "client send 1");  
   	struct msghdr msg;
   	struct iovec iov;
   	msg.msg_name = &server;
   	msg.msg_namelen = sizeof(server);
   	msg.msg_iov = &iov;
   	msg.msg_iovlen = 1;
   	msg.msg_iov->iov_base = buff;
   	msg.msg_iov->iov_len = strlen(buff);
   	msg.msg_control = 0;
   	msg.msg_controllen = 0;
   	msg.msg_flags = 0;
   	sendmsg(s,&msg,0);
	
	memset(buff,0,BUFFLEN);
	msg.msg_iov->iov_base = buff;
   	msg.msg_iov->iov_len = BUFFLEN;
	ssize_t size = recvmsg(s,&msg,0);
	char *str=msg.msg_iov[0].iov_base;
	str[size] = '\0';
    if (size>0){
		printf("client receive 1:%s\n",str);
    }
    
    close(s);

    return 0;
}
