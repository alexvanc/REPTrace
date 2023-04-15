#include "tracer.h"





//0 controller off
//1 controller on
const short with_uuid=1;

const char* controllerip="10.211.55.38";
const char* controller_service="10.211.55.38/controller.php";
const char* filePath="/tmp/tracelog.txt";
const char* filePath2="/tmp/tracelog2.txt";
const char* logFolder="/tmp/tlogs/";

const char* idenv="UUID_PASS";
static pthread_key_t uuid_key=12345;
static pthread_key_t id_rest=23456;
extern char **environ;



ssize_t recv(int sockfd, void *buf, size_t len, int flags)
{
    static void *handle = NULL;
    static RECV old_recv = NULL;
    if( !handle )
    {
        handle = dlopen("libc.so.6", RTLD_LAZY);
        old_recv = (RECV)dlsym(handle, "recv");
    }
    if(!uuid_key){
        pthread_key_create(&uuid_key,NULL);
    }
    
#ifdef DEBUG
    char log_text[LOG_LENGTH]="init";
    sprintf(log_text,"%s\t","in recv");
#endif
    //check socket type first
    sa_family_t socket_family=get_socket_family(sockfd);
    if (socket_family==AF_INET){
#ifdef DEBUG
        sprintf(log_text,"%s%s\t",log_text,"AF_INET socket");
#endif
        struct sockaddr_in sin; //local socket info
        struct sockaddr_in son; //remote socket into
        socklen_t s_len = sizeof(sin);
        if ((getsockname(sockfd, (struct sockaddr *)&sin, &s_len) != -1) &&(getpeername(sockfd, (struct sockaddr *)&son, &s_len) != -1))
        {
            
            unsigned short int in_port;
            unsigned short int on_port;
            char in_ip[256];
            char on_ip[256];
            char *tmp_in_ip;
            char *tmp_on_ip;
            in_port=ntohs(sin.sin_port);
            on_port=ntohs(son.sin_port);
            tmp_in_ip=inet_ntoa(sin.sin_addr);
            memmove(in_ip,tmp_in_ip,strlen(tmp_in_ip)+1);
            tmp_on_ip=inet_ntoa(son.sin_addr);
            memmove(on_ip,tmp_on_ip,strlen(tmp_on_ip)+1);
            
#ifdef IP_PORT
            sprintf(log_text,"%srecv from %s:%d with %s:%d\t",log_text,on_ip,on_port,in_ip,in_port);
#endif
            
#ifdef DATA_INFO
            //get socket buff size
            int recv_size = 0;    /* socket接收缓冲区大小 */
            int send_size=0;
            socklen_t optlen;
            optlen = sizeof(send_size);
            if(getsockopt(sockfd, SOL_SOCKET, SO_SNDBUF,&send_size, &optlen)!=0){
                sprintf(log_text,"%s%s\t",log_text,"get socket send buff failed!");
            }
            optlen = sizeof(recv_size);
            if(getsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &recv_size, &optlen)!=0){
                sprintf(log_text,"%s%s\t",log_text,"get socket recv buff failed!");
            }
#endif
            
            ssize_t n=0;
            if(check_filter(on_ip,in_ip,on_port,in_port)==0){
#ifdef FILTER
                sprintf(log_text,"%s%s\n",log_text,"recv done with filter!");
                log_event(log_text);
#endif
                n=old_recv(sockfd,buf,len,flags);
                if((on_port==80)||(in_port==80)){
                    return n;
                }
                int tmp_erro=errno;
                push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),"",gettime(),F_RECV,n,len,n,sockfd,RECV_FILTER,buf);
                if(n<=0){
                    errno=tmp_erro;
                }
                return n;
            }
            
            
            if (with_uuid==0){
                n=old_recv(sockfd,buf,len,flags);
#ifdef DATA_INFO
                sprintf(log_text,"%srecv buff length:%zu\t recv data length:%ld\t supposed recv data length:%ld\t socket recv buff length:%d\t socket send buff length:%d\t",log_text,len,n,n-ID_LENGTH,recv_size,send_size);
                sprintf(log_text,"%srecv process:%u\t thread:%lu\t socketid:%d\t",log_text,getpid(),pthread_self(),sockfd);
#endif
                //if errors happened during recv, then return the error result directly
                if(n<=0){
#ifdef DEBUG
                    sprintf(log_text,"%s%s\t%s\n",log_text,strerror(errno),"recv done with recv error!");
                    log_event(log_text);
#endif
                    int tmp_erro=errno;
                    push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),"",gettime(),F_RECV,n,len,n,sockfd,RECV_ERROR,buf);
                    errno=tmp_erro;
                    return n;
                }
#ifdef MESSAGE
                log_message(buf,n);
#endif
#ifdef DEBUG
                sprintf(log_text,"%s%s\n",log_text,"recv done without uuid!");
                log_event(log_text);
#endif
                push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),"",gettime(),F_RECV,n,len,n,sockfd,RECV_NORMALLY,buf);
                return n;
            }else{
                
                char uuids[LOG_LENGTH];
                ssize_t length=0;
                char status;
                n=check_read(uuids,buf,len,sockfd,&length,&status);
                //                log_sock_message(buf,n,on_port,in_port);
#ifdef DEBUG
                sprintf(log_text,"real recv length:%ld\tsupposed length:%ld\treturn length:%ld\n",log_text,"recv done without uuid!");
                log_event(log_text);
#endif
                push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),uuids,gettime(),F_RECV,&length,len,n,sockfd,&status,buf);
                return n;
            }
            
        }else{
#ifdef DEBUG
            sprintf(log_text,"%s%s\n",log_text,"recv done with getsocketname errors!");
            log_event(log_text);
#endif
        }
    }
    
    
    return old_recv(sockfd, buf, len, flags);
    
}

ssize_t send(int sockfd, const void *buf, size_t len, int flags)
{
    
    static void *handle = NULL;
    static SEND old_send = NULL;
    if( !handle )
    {
        handle = dlopen("libc.so.6", RTLD_LAZY);
        old_send = (SEND)dlsym(handle, "send");
    }
    if(!uuid_key){
        pthread_key_create(&uuid_key,NULL);
    }
#ifdef DEBUG
    char log_text[LOG_LENGTH]="init";
    sprintf(log_text,"%s\t","in send");
#endif
    
    //check socket type first
    sa_family_t socket_family=get_socket_family(sockfd);
    if (socket_family==AF_INET){
#ifdef DEBUG
        sprintf(log_text,"%s%s\t",log_text,"AF_INET socket");
#endif
        struct sockaddr_in sin;
        struct sockaddr_in son;
        socklen_t s_len = sizeof(son);
        if  ((getsockname(sockfd, (struct sockaddr *)&sin, &s_len) != -1) &&(getpeername(sockfd, (struct sockaddr *)&son, &s_len) != -1)){
            unsigned short int in_port;
            unsigned short int on_port;
            char in_ip[256];
            char on_ip[256];
            char *tmp_in_ip;
            char *tmp_on_ip;
            in_port=ntohs(sin.sin_port);
            on_port=ntohs(son.sin_port);
            tmp_in_ip=inet_ntoa(sin.sin_addr);
            memmove(in_ip,tmp_in_ip,strlen(tmp_in_ip)+1);
            tmp_on_ip=inet_ntoa(son.sin_addr);
            memmove(on_ip,tmp_on_ip,strlen(tmp_on_ip)+1);
            
            
#ifdef IP_PORT
            sprintf(log_text,"%ssend to %s:%d with %s:%d\t",log_text,on_ip,on_port,in_ip,in_port);
#endif
            
#ifdef DATA_INFO
            //get socket buff size
            int recv_size = 0;    /* socket接收缓冲区大小 */
            int send_size=0;
            socklen_t optlen;
            optlen = sizeof(send_size);
            if(getsockopt(sockfd, SOL_SOCKET, SO_SNDBUF,&send_size, &optlen)!=0){
                sprintf(log_text,"%s%s\t",log_text,"get socket send buff failed!");
            }
            optlen = sizeof(recv_size);
            if(getsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &recv_size, &optlen)!=0){
                sprintf(log_text,"%s%s\t",log_text,"get socket recv buff failed!");
            }
#endif
            
            size_t new_len=len+ID_LENGTH;
            
            //        sprintf(log_text,"buf size:%lu\t target size:%d\t",len,new_len);
            //        log_event(log_text);
            ssize_t n=0;
            if((SIZE_MAX-len)<MAX_SPACE){
#ifdef DEBUG
                sprintf(log_text,"%s%s\t%s\n",log_text,"too large buf size, cannot add uuid to it","write done!");
                log_event(log_text);
#endif
                n=old_send(sockfd, buf, len, flags);
                int tmp_erro=errno;
                push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),"",gettime(),F_SEND,n,len,n,sockfd,SEND_ERROR,buf);
                if(n<=0){
                    errno=tmp_erro;
                }
                return n;
            }
            
            if(with_uuid==0){//just for log
#ifdef DATA_INFO
                sprintf(log_text,"%ssend data length:%zu\t supposed send data length:%zu\t socket recv buff length:%d\t socket send buff length:%d\t",log_text,new_len,len,recv_size,send_size);
                sprintf(log_text,"%ssend process:%u\t thread:%lu\t socketid:%d\t",log_text,getpid(),pthread_self(),sockfd);
#endif
            }
            
            if(check_filter(on_ip,in_ip,on_port,in_port)==0){
#ifdef FILTER
                sprintf(log_text,"%s%s\n",log_text,"send done with filter");
                log_event(log_text);
#endif
                
                n=old_send(sockfd, buf, len, flags);
                if((on_port==80)||(in_port==80)){
                    return n;
                }
                int tmp_erro=errno;
                push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),"",gettime(),F_SEND,n,len,n,sockfd,SEND_FILTER,buf);
                if(n<=0){
                    errno=tmp_erro;
                }
                
                return n;
            }
            
            if(with_uuid==0){//no uuid, recv this message
                
#ifdef MESSAGE
                log_message((const char*)buf,len);
#endif
#ifdef DEBUG
                sprintf(log_text,"%s%s\n",log_text,"send done without uuid!");
                log_event(log_text);
#endif
                n=old_send(sockfd, buf, len, flags);
                if((on_port==80)||(in_port==80)){
                    return n;
                }
                int tmp_erro=errno;
                push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),"",gettime(),F_SEND,n,len,n,sockfd,SEND_NORMALLY,buf);
                if(n<=0){
                    errno=tmp_erro;
                }
                return n;
            }
            
            char target[new_len];
            //            char id[ID_LENGTH];
            char *id=malloc(ID_LENGTH);//memory collected by the pthread_key_create
            random_uuid(id);
            int i;
            for (i=0;i<ID_LENGTH;i++){
                target[i]=id[i];
            }
            memmove(&target[ID_LENGTH],buf,len);
            
            
            long long ttime;
            ttime=gettime();
            n=old_send(sockfd, target, new_len, flags);
            int tmp_erro=errno;
            //            log_message3(target,new_len);
#ifdef DATA_INFO
            sprintf(log_text,"%sreal send length:%ld\t send data length:%zu\t supposed send data length:%zu\t socket recv buff length:%d\t socket send buff length:%d\t",log_text,n,new_len,len,recv_size,send_size);
            sprintf(log_text,"%ssend process:%u\t thread:%lu\t socketid:%d\t",log_text,getpid(),pthread_self(),sockfd);
#endif
#ifdef MESSAGE
            log_message(target,new_len);
#endif
#ifdef DEBUG
            sprintf(log_text,"%s%s\n",log_text,"send done!");
            log_event(log_text);
#endif
            ssize_t rvalue=0;
            if(n==new_len){
                rvalue=len;
                //                return len;
            }else if((n<=len)&&(n>=ID_LENGTH)){
                rvalue=n-39;
                //                return n-39;
            }else if((n==0)||(n==-1)){
                //                return n;
                rvalue=n;
            }else{
                //                return 0;//maybe wrong
                rvalue=0;
            }
            push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),id,gettime(),F_SEND,n,len,rvalue,sockfd,SEND_ID,buf);
            if(n!=new_len){
                errno=tmp_erro;
            }
            return rvalue;
        }else{
#ifdef DEBUG
            sprintf(log_text,"%s%s\n",log_text,"send done with get socket error!");
            log_event(log_text);
#endif
        }
    }
    return old_send(sockfd,buf,len,flags);
}

ssize_t write(int fd, const void *buf, size_t count)
{
    static void *handle = NULL;
    static WRITE old_write = NULL;
    if( !handle )
    {
        handle = dlopen("libc.so.6", RTLD_LAZY);
        old_write = (WRITE)dlsym(handle, "write");
    }
    if(!uuid_key){
        pthread_key_create(&uuid_key,NULL);
    }
    //if fd is not a socket, return directly
    if (!is_socket(fd)){
        return old_write(fd, buf, count);
    }
#ifdef DEBUG
    char log_text[LOG_LENGTH]="init";
    sprintf(log_text,"%s\t","in write");
#endif
    //check socket type first
    sa_family_t socket_family=get_socket_family(fd);
    if (socket_family==AF_INET){
#ifdef DEBUG
        sprintf(log_text,"%s%s\t",log_text,"AF_INET socket");
#endif
        struct sockaddr_in sin;
        struct sockaddr_in son;
        socklen_t s_len = sizeof(son);
        if  ((getsockname(fd, (struct sockaddr *)&sin, &s_len) != -1) &&(getpeername(fd, (struct sockaddr *)&son, &s_len) != -1)){
            unsigned short int in_port;
            unsigned short int on_port;
            char in_ip[256];
            char on_ip[256];
            char *tmp_in_ip;
            char *tmp_on_ip;
            in_port=ntohs(sin.sin_port);
            on_port=ntohs(son.sin_port);
            tmp_in_ip=inet_ntoa(sin.sin_addr);
            memmove(in_ip,tmp_in_ip,strlen(tmp_in_ip)+1);
            tmp_on_ip=inet_ntoa(son.sin_addr);
            memmove(on_ip,tmp_on_ip,strlen(tmp_on_ip)+1);
            
#ifdef IP_PORT
            sprintf(log_text,"%swrite to %s:%d with %s:%d\t",log_text,on_ip,on_port,in_ip,in_port);
#endif
            
#ifdef DATA_INFO
            //get socket buff size
            int recv_size = 0;    /* socket接收缓冲区大小 */
            int send_size=0;
            socklen_t optlen;
            optlen = sizeof(send_size);
            if(getsockopt(fd, SOL_SOCKET, SO_SNDBUF,&send_size, &optlen)!=0){
                sprintf(log_text,"%s%s\t",log_text,"get socket send buff failed!");
            }
            optlen = sizeof(recv_size);
            if(getsockopt(fd, SOL_SOCKET, SO_RCVBUF, &recv_size, &optlen)!=0){
                sprintf(log_text,"%s%s\t",log_text,"get socket recv buff failed!");
            }
#endif
            size_t new_len=count+ID_LENGTH;
            ssize_t n=0;
            if((SIZE_MAX-count)<MAX_SPACE){
#ifdef DEBUG
                sprintf(log_text,"%s%s\t%s\n",log_text,"get socket recv buff failed!","write done!");
                log_event(log_text);
#endif
                n=old_write(fd, buf, count);
                int tmp_erro=errno;
                push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),"",gettime(),F_WRITE,n,count,n,fd,SEND_ERROR,buf);
                if(n<=0){
                    errno=tmp_erro;
                }
                return n;
            }
            
            if(with_uuid==0){//just for log
#ifdef DATA_INFO
                sprintf(log_text,"%swrite data length:%zu\t supposed write data length:%zu\t socket recv buff length:%d\t socket send buff length:%d\t",log_text,new_len,count,recv_size,send_size);
                
                sprintf(log_text,"%swrite process:%u\t thread:%lu\t socketid:%d\t",log_text,getpid(),pthread_self(),fd);
#endif
            }
            if(check_filter(on_ip,in_ip,on_port,in_port)==0){
#ifdef FILTER
                sprintf(log_text,"%s%s\n",log_text,"write done with filter!");
                log_event(log_text);
#endif
                n=old_write(fd, buf, count);
                if((on_port==80)||(in_port==80)){
                    return n;
                }
                int tmp_erro=errno;
                push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),"",gettime(),F_WRITE,n,count,n,fd,SEND_FILTER,buf);
                if(n<=0){
                    errno=tmp_erro;
                }
                return n;
            }
            
            if(with_uuid==0){//no uuid, write this message
#ifdef MESSAGE
                log_message( (char*)buf,count);
#endif
#ifdef DEBUG
                sprintf(log_text,"%s%s\n",log_text,"write done without uuid!");
                log_event(log_text);
#endif
                n=old_write(fd, buf, count);
                int tmp_erro=errno;
                push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),"",gettime(),F_WRITE,n,count,n,fd,SEND_NORMALLY,buf);
                if(n<=0){
                    errno=tmp_erro;
                }
                return n;
            }
            char target[new_len];
            //            char id[ID_LENGTH];
            char *id=malloc(ID_LENGTH);//memory collected by the pthread_key_create
            random_uuid(id);
            int i;
            for (i=0;i<ID_LENGTH;i++){
                target[i]=id[i];
            }
            
            //add uuid at the beginning of data
            memmove(&target[ID_LENGTH],buf,count);
            
            long long ttime;
            ttime=gettime();
            n=old_write(fd, target, new_len);
            //            log_sock_message(target,n,in_port,on_port);
            int tmp_erro=0;
            //            if(n<=0){
            tmp_erro=errno;
            //            }
            
            //            log_message3(target,new_len);
#ifdef DATA_INFO
            sprintf(log_text,"%sreal write length:%ld\t write data length:%zu\t supposed write data length:%zu\t socket recv buff length:%d\t socket send buff length:%d\t",log_text,n,new_len,count,recv_size,send_size);
            sprintf(log_text,"%swrite process:%u\t thread:%lu\t socketid:%d\t",log_text,getpid(),pthread_self(),fd);
#endif
            
#ifdef MESSAGE
            log_message(target,new_len);
#endif
#ifdef DEBUG
            sprintf(log_text,"%s%s\n",log_text,"write done!");
            log_event(log_text);
#endif
            ssize_t rvalue=0;
            if(n==new_len){
                rvalue=count;
                //                return count;
            }else if((n<=count)&&(n>=ID_LENGTH)){
                rvalue=n-39;
                //                return n-39;
            }else if((n==0)||(n==-1)){
                rvalue=n;
                //                return n;
            }else{
                rvalue=0;
                //                return 0;//maybe wrong
            }
            push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),id,gettime(),F_WRITE,n,count,rvalue,fd,SEND_ID,buf);
            if(n!=new_len){
                errno=tmp_erro;
            }
            return rvalue;
            
        }else{
#ifdef DEBUG
            sprintf(log_text,"%s%s\n",log_text,"write done with get socket error!");
            log_event(log_text);
#endif
        }
    }
    
    return old_write(fd, buf, count);
}



ssize_t read(int fd, void *buf, size_t count)
{
    static void *handle = NULL;
    static READ old_read = NULL;
    if( !handle )
    {
        handle = dlopen("libc.so.6", RTLD_LAZY);
        old_read = (READ)dlsym(handle, "read");
    }
    if (!is_socket(fd)){
        return old_read(fd, buf, count);
    }
    if(!uuid_key){
        pthread_key_create(&uuid_key,NULL);
    }
#ifdef DEBUG
    char log_text[LOG_LENGTH]="init";
    sprintf(log_text,"%s\t","in read");
#endif
    //check socket type first
    sa_family_t socket_family=get_socket_family(fd);
    if (socket_family==AF_INET){
#ifdef DEBUG
        sprintf(log_text,"%s%s\t",log_text,"AF_INET socket");
#endif
        struct sockaddr_in sin; //local socket info
        struct sockaddr_in son; //remote socket into
        socklen_t s_len = sizeof(sin);
        if ((getsockname(fd, (struct sockaddr *)&sin, &s_len) != -1) &&(getpeername(fd, (struct sockaddr *)&son, &s_len) != -1))
        {
            //https://linux.die.net/man/3/inet_ntoa
            unsigned short int in_port;
            unsigned short int on_port;
            char in_ip[256];
            char on_ip[256];
            char *tmp_in_ip;
            char *tmp_on_ip;
            in_port=ntohs(sin.sin_port);
            on_port=ntohs(son.sin_port);
            tmp_in_ip=inet_ntoa(sin.sin_addr);
            memmove(in_ip,tmp_in_ip,strlen(tmp_in_ip)+1);
            tmp_on_ip=inet_ntoa(son.sin_addr);
            memmove(on_ip,tmp_on_ip,strlen(tmp_on_ip)+1);
            
#ifdef IP_PORT
            sprintf(log_text,"%sread from %s:%d with %s:%d\t",log_text,on_ip,on_port,in_ip,in_port);
#endif
#ifdef DATA_INFO
            //get socket buff size
            int recv_size = 0;    /* socket接收缓冲区大小 */
            int send_size=0;
            socklen_t optlen;
            optlen = sizeof(send_size);
            if(getsockopt(fd, SOL_SOCKET, SO_SNDBUF,&send_size, &optlen)!=0){
                sprintf(log_text,"%s%s\t",log_text,"get socket send buff failed!");
            }
            optlen = sizeof(recv_size);
            if(getsockopt(fd, SOL_SOCKET, SO_RCVBUF, &recv_size, &optlen)!=0){
                sprintf(log_text,"%s%s\t",log_text,"get socket recv buff failed!");
            }
#endif
            ssize_t n=0;
            if(check_filter(on_ip,in_ip,on_port,in_port)==0){
#ifdef FILTER
                sprintf(log_text,"%s%s\n",log_text,"read done with filter!");
                log_event(log_text);
#endif
                n=old_read(fd,buf,count);
                if((on_port==80)||(in_port==80)){
                    return n;
                }
                int tmp_erro=errno;
                push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),"",gettime(),F_READ,n,count,0,fd,RECV_FILTER,buf);
                if(n<=0){
                    errno=tmp_erro;
                }
                return n;
            }
            
            
            if(with_uuid==0){//no uuid, read this message
                
                n=old_read(fd, buf, count);
#ifdef DATA_INFO
                sprintf(log_text,"%sread buff length:%zu\t real read data length:%ld\t socket recv buff length:%d\t socket send buff length:%d\t",log_text,count,n,recv_size,send_size);
                sprintf(log_text,"%sread process:%u\t thread:%lu\t socketid:%d\t",log_text,getpid(),pthread_self(),fd);
#endif
                if(n<=0){
#ifdef RW_ERR
                    sprintf(log_text,"%serrorno:%d\t%s\t%s\n",log_text,errno,strerror(errno),"read done with read error!");
                    log_event(log_text);
#endif
                    int tmp_erro=errno;
                    push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),"",gettime(),F_READ,n,count,n,fd,RECV_ERROR,"");
                    errno=tmp_erro;
                    return n;
                }
#ifdef MESSAGE
                log_message(buf,n);
#endif
#ifdef DEBUG
                sprintf(log_text,"%s%s\t%s\n",log_text,strerror(errno),"read done without uuid!");
                log_event(log_text);
#endif
                push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),"",gettime(),F_READ,n,count,n,fd,RECV_NORMALLY,buf);
                return n;
            }else{
                char uuids[LOG_LENGTH];
                ssize_t length=0;
                char status;
                n=check_read(uuids,buf,count,fd,&length,&status);
                //                log_sock_message(buf,n,on_port,in_port);
                push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),uuids,gettime(),F_READ,&length,count,n,fd,&status,buf);
                return n;
            }
            
        }else{
#ifdef DEBUG
            sprintf(log_text,"%s%s\n",log_text,"read donw with get socket error!");
            log_event(log_text);
#endif
        }
        
    }
    
    return old_read(fd, buf, count);
}





void *intermedia(void * arg){
    
    struct thread_param *temp;
    
    void *(*start_routine) (void *);
    temp=(struct thread_param *)arg;
    
    pthread_setspecific(uuid_key,(void *)temp->uuid);
    
    //for storage
    //    pthread_setspecific(id_rest,(void *)temp->storage);
    
    
    push_thread_db(getpid(),syscall(SYS_gettid),pthread_self(),temp->ppid,temp->pktid,temp->ptid,temp->ttime);
    
    return temp->start_routine(temp->args);
    //    return (*start_routine)(args);
    
    
}



//extern int sendmmsg(int sockfd, struct mmsghdr *msgvec, unsigned int vlen,unsigned int flags){
//    static void *handle = NULL;
//    static SENDMMSG old_sendmmsg = NULL;
//    struct sockaddr_in sin;
//    struct sockaddr_in son;
//    socklen_t s_len = sizeof(son);
//    if( !handle )
//    {
//        handle = dlopen("libc.so.6", RTLD_LAZY);
//        old_sendmmsg = (SENDMMSG)dlsym(handle, "sendmmsg");
//    }
//    if(!uuid_key){
//        pthread_key_create(&uuid_key,NULL);
//    }
//    if  ((getsockname(sockfd, (struct sockaddr *)&sin, &s_len) != -1) &&(getpeername(sockfd, (struct sockaddr *)&son, &s_len) != -1)){
//        log_event("in sendmmsg\n");
//        return old_sendmmsg(sockfd,msgvec,vlen,flags);
//    }
//    return old_sendmmsg(sockfd,msgvec,vlen,flags);
//
//}
//extern int recvmmsg(int sockfd, struct mmsghdr *msgvec, unsigned int vlen,unsigned int flags, struct timespec *timeout){
//    static void *handle = NULL;
//    static RECVMMSG old_recvmmsg = NULL;
//    struct sockaddr_in sin;
//    struct sockaddr_in son;
//    socklen_t s_len = sizeof(son);
//    if( !handle )
//    {
//        handle = dlopen("libc.so.6", RTLD_LAZY);
//        old_recvmmsg = (RECVMMSG)dlsym(handle, "recvmmsg");
//    }
//    if(!uuid_key){
//        pthread_key_create(&uuid_key,NULL);
//    }
//    if  ((getsockname(sockfd, (struct sockaddr *)&sin, &s_len) != -1) &&(getpeername(sockfd, (struct sockaddr *)&son, &s_len) != -1)){
//        log_event("in recvmmsg\n");
//        return old_recvmmsg(sockfd,msgvec,vlen,flags,timeout);
//    }
//    return old_recvmmsg(sockfd,msgvec,vlen,flags,timeout);
//}
ssize_t readv(int fd, const struct iovec *iov, int iovcnt){
    static void *handle = NULL;
    static READV old_readv = NULL;
    if( !handle )
    {
        handle = dlopen("libc.so.6", RTLD_LAZY);
        old_readv = (READV)dlsym(handle, "readv");
    }
    if(!uuid_key){
        pthread_key_create(&uuid_key,NULL);
    }
    if (is_socket(fd)){
        log_event("in readv\n");
    }
    return old_readv(fd,iov,iovcnt);
    
}
ssize_t writev(int fd, const struct iovec *iov, int iovcnt){
    static void *handle = NULL;
    static WRITEV old_writev = NULL;
    if( !handle )
    {
        handle = dlopen("libc.so.6", RTLD_LAZY);
        old_writev = (WRITEV)dlsym(handle, "writev");
    }
    if(!uuid_key){
        pthread_key_create(&uuid_key,NULL);
    }
    if (is_socket(fd)){
        log_event("in writev\n");
    }
    return old_writev(fd,iov,iovcnt);
    
}


int  pthread_create(pthread_t  *thread,  const pthread_attr_t  *attr,  void  *(*start_routine)(void  *),  void  *arg){
    static void *handle = NULL;
    static P_CREATE old_create=NULL;
    if( !handle )
    {
        handle = dlopen("libpthread.so.0", RTLD_LAZY);
        old_create = (P_CREATE)dlsym(handle, "pthread_create");
    }
    
    pthread_t tmp=pthread_self();
    
    //for log
    int i=0;
    for(;environ[i]!=NULL;i++){}
    char test[1024]="";
    sprintf(test,"create pid:%d\tptid:%ld\ttid:%lu,key:%u\t,value:%s\tenv:%s\tenv str:%s\t",getpid(),syscall(SYS_gettid),tmp,uuid_key,(char *)pthread_getspecific(uuid_key),getenv(idenv),environ[i-1]);
    
    if(uuid_key==12345){
        pthread_key_create(&uuid_key,NULL);
        char * tmp_env=(char *)malloc(1024); //never free
        int env_null=get_own_env(tmp_env);
        if(!env_null){
            pthread_setspecific(uuid_key,tmp_env);
        }
    }
    
    
    
    struct thread_param *temp=malloc(sizeof(struct thread_param));
    temp->uuid=(char *)pthread_getspecific(uuid_key);
    temp->args=arg;
    temp->start_routine=start_routine;
    temp->ppid=getpid();
    temp->pktid=syscall(SYS_gettid);
    temp->ptid=tmp;
    //    temp->ttime=gettime();
    
    int result=old_create(thread,attr,intermedia,(void *)temp);
    
    
    return result;
}

int pthread_join(pthread_t thread, void **retval){
    static void *handle = NULL;
    static P_JOIN old_join=NULL;
    if( !handle )
    {
        handle = dlopen("libpthread.so.0", RTLD_LAZY);
        old_join = (P_JOIN)dlsym(handle, "pthread_join");
    }
    
    pthread_t tmp=pthread_self();
    push_thread_dep(getpid(),syscall(SYS_gettid),tmp,thread,gettime());
    return old_join(thread,retval);
    
}

pid_t fork(void){
    static void *handle = NULL;
    static FORK old_fork=NULL;
    if( !handle )
    {
        handle = dlopen("libc.so.6", RTLD_LAZY);
        old_fork = (FORK)dlsym(handle, "fork");
    }
    //    pid_t ppid=getpid();
    //    pid_t pktid=syscall(SYS_gettid);
    //    //    long int tpktid=pktid;
    //    pthread_t ptid=pthread_self();
    //for log
    //    char test[1024]="";
    ////    sprintf(test,"fork pid:%d\tptid:%ld\ttid:%lu, key:%u\tvalue:%s\t",getpid(),syscall(SYS_gettid),pthread_self(),uuid_key,(char*)pthread_getspecific(uuid_key));
    //    sprintf(test,"fork pid:%d\tptid:%ld\ttid:%lu, key:%u\tvalue:%s\t",getpid(),pktid,pthread_self(),uuid_key,(char*)pthread_getspecific(uuid_key));
    
    
    
    if(uuid_key==12345){
        pthread_key_create(&uuid_key,NULL);
        char * tmp_env=(char *)malloc(1024); //never free
        int env_null=get_own_env(tmp_env);
        if(!env_null){
            pthread_setspecific(uuid_key,tmp_env);
        }
        
    }
    char *uuid=(char *)pthread_getspecific(uuid_key);
    char parameter[1024]="";
    sprintf(parameter,"ppid=%ld&pktid=%ld&ptid=%lu&ttime=%lld&rtype=%d&",getpid(),syscall(SYS_gettid),pthread_self(),gettime(),R_THREAD);
    
    pid_t result=old_fork();
    if(result==0){
        sprintf(parameter,"%spid=%ld&ktid=%ld&tid=%lu",parameter,getpid(),syscall(SYS_gettid),pthread_self());
        pthread_setspecific(uuid_key, (void *)uuid);
        //        log_message(parameter,strlen(parameter));
        push_thread_db2(parameter);
        //        push_thread_db(getpid(),syscall(SYS_gettid),pthread_self(),ppid,pktid,ptid,gettime());
        //        sprintf(test,"%s\tnew pid:%d\tptid:%ld\ttid:%lu, key:%uvalue:%s\n",test,getpid(),syscall(SYS_gettid),pthread_self(),uuid_key,(char*)pthread_getspecific(uuid_key));
        //        log_message(test,strlen(test));
        
    }
    return result;
}
//
pid_t vfork(void){
    //    long desti;
    static void *handle = NULL;
    //    static VFORK old_vfork=NULL;
    static FORK old_fork=NULL;
    if( !handle )
    {
        handle = dlopen("libc.so.6", RTLD_LAZY);
        old_fork=(FORK)dlsym(handle, "fork");
    }
    
    //    pid_t ppid=getpid();
    //    pid_t pktid=syscall(SYS_gettid);
    //    long int tpktid=pktid;
    //    pthread_t ptid=pthread_self();
    //    char test[1024]="";
    //    sprintf(test,"3original pid:%d\tptid:%ld\ttid:%lu, key:%u\tvalue:%s\tenvtest:%s\t",getpid(),syscall(SYS_gettid),pthread_self(),uuid_key,(char*)pthread_getspecific(uuid_key),getenv(idenv));
    
    if(uuid_key==12345){
        pthread_key_create(&uuid_key,NULL);
        char * tmp_env=(char *)malloc(1024); //never free
        int env_null=get_own_env(tmp_env);
        if(!env_null){
            pthread_setspecific(uuid_key,tmp_env);
        }
    }
    char *uuid=(char *)pthread_getspecific(uuid_key);
    char parameter[1024]="";
    sprintf(parameter,"ppid=%ld&pktid=%ld&ptid=%lu&ttime=%lld&rtype=%d&",getpid(),syscall(SYS_gettid),pthread_self(),gettime(),R_THREAD);
    
    
    pid_t result=old_fork();
    if(result==0){
        sprintf(parameter,"%spid=%ld&ktid=%ld&tid=%lu",parameter,getpid(),syscall(SYS_gettid),pthread_self());
        pthread_setspecific(uuid_key, (void *)uuid);
        //        log_message(parameter,strlen(parameter));
        push_thread_db2(parameter);
        //        push_thread_db(getpid(),syscall(SYS_gettid),pthread_self(),ppid,pktid,ptid,gettime());
        //        sprintf(test,"%s\tnew pid:%d\tptid:%ld\ttid:%lu, key:%uvalue:%s\n",test,getpid(),syscall(SYS_gettid),pthread_self(),uuid_key,(char*)pthread_getspecific(uuid_key));
        //        log_message(test,strlen(test));
    }
    return result;
    //    asm("movq %%rsp, %0;"
    //    :"=r"(desti)
    //    :
    //    :);
    //
    //    log_message("vfork",5);
    //    pid_t ppid=getpid();
    //    pthread_t ptid=pthread_self();
    //    pid_t result=old_vfork();
    //    if(result==0){
    ////        push_thread_db(getpid(),pthread_self(),ptid,ppid,gettime());
    //        asm volatile("jmp *%0;"
    //                     :
    //                     :"r"(desti)
    //                     :);
    //
    //    }else{
    //        return result;
    //    }
    //    return old_vfork();
}

int execl(const char *path, const char *arg, ...){
    char test[1014]="";
    sprintf(test,"in execl pid:%d\tptid:%ld\ttid:%lu\n",getpid(),syscall(SYS_gettid),pthread_self());
    log_message(test,strlen(test));
}

int execlp(const char *file, const char *arg, ...){
    char test[1014]="";
    sprintf(test,"in execlp pid:%d\tptid:%ld\ttid:%lu\n",getpid(),syscall(SYS_gettid),pthread_self());
    log_message(test,strlen(test));
}
// int execlpe(const char *file, const char *arg0, ..., const char *argn, (char *)0, char *const envp[]);

// int execle(const char *path, const char *arg, ..., char *const envp[]){
//     char test[1014]="";
//     sprintf(test,"in execle pid:%d\tptid:%ld\ttid:%lu\n",getpid(),syscall(SYS_gettid),pthread_self());
//     log_message(test,strlen(test));
// }

int execv(const char *path, char *const argv[]){
    char test[1014]="";
    sprintf(test,"in execv pid:%d\tptid:%ld\ttid:%lu\n",getpid(),syscall(SYS_gettid),pthread_self());
    log_message(test,strlen(test));
}

int execve(const char *path, char *const argv[], char *const envp[]){
    char ex_env[1024]="";
    if(uuid_key==12345){
        char *envtemp=(char *)malloc(1024); //never free;
        //        char envtemp[1024];
        pthread_key_create(&uuid_key,NULL);
        int env_result=get_own_env(envtemp);
        sprintf(ex_env,"%s=%s",idenv,envtemp);
        //not necessary
        if(env_result==0){
            pthread_setspecific(uuid_key,envtemp);
        }else{
            free(envtemp);
        }
        //until here
    }else{
        char *context=(char *)pthread_getspecific(uuid_key);
        if(!is_null(context)){
            sprintf(ex_env,"%s=%s",idenv,context);
        }else{
            sprintf(ex_env,"%s=%s",idenv,"(null)");
        }
    }
    
    //    //for log
    //    int k=0;
    //    for(;environ[k]!=NULL;k++);
    ////    char test[1024]="";
    ////    sprintf(test,"in execve path:%s\tpid:%d\tptid:%ld\ttid:%lu\tenvtest:%s\tTLD:%s\tenviron:%s\n",path,getpid(),syscall(SYS_gettid),pthread_self(),getenv(idenv),(char*)pthread_getspecific(uuid_key),environ[k-1]);
    ////    log_message(test,strlen(test));
    
    int i=0;
    //    char ex_env[1024]="";
    //    char *context=(char *)pthread_getspecific(uuid_key);
    //    if(is_null(context)){//get from env
    //        char envtmp[1024];
    //        int env_result=get_own_env(envtmp);
    //        sprintf(ex_env,"%s=%s",idenv,envtmp);
    //    }else{
    //        sprintf(ex_env,"%s=%s",idenv,(char*)pthread_getspecific(uuid_key));
    //    }
    
    for(;environ[i]!=NULL;i++){}
    char * tenvp[i+2];
    int j=0;
    for (;j<i;j++){
        tenvp[j]=environ[j];
    }
    tenvp[j++]=ex_env;
    tenvp[j]=NULL;
    //to do
    //to decide whether to add UUID to environ variable
    
    static void *handle = NULL;
    static EXECVE old_execve=NULL;
    if( !handle )
    {
        handle = dlopen("libc.so.6", RTLD_LAZY);
        old_execve = (EXECVE)dlsym(handle, "execve");
    }
    return old_execve(path,argv,tenvp);
    
}

int execvp(const char *file, char *const argv[]){
    
    char ex_env[1024]="";
    if(uuid_key==12345){
        char *envtemp=(char *)malloc(1024); //never free;
        //        char envtemp[1024];
        pthread_key_create(&uuid_key,NULL);
        int env_result=get_own_env(envtemp);
        sprintf(ex_env,"%s=%s",idenv,envtemp);
        //not necessary
        if(env_result==0){
            pthread_setspecific(uuid_key,envtemp);
        }else{
            free(envtemp);
        }
        //until here
    }else{
        char *context=(char *)pthread_getspecific(uuid_key);
        if(!is_null(context)){
            sprintf(ex_env,"%s=%s",idenv,context);
        }else{
            sprintf(ex_env,"%s=%s",idenv,"(null)");
        }
        
    }
    
    //    //for log
    //    int k=0;
    //    for(;environ[k]!=NULL;k++);
    //    char test[1024]="";
    //    sprintf(test,"in execvp file:%s\tpid:%d\tptid:%ld\ttid:%lu\tenvtest:%s\tTLD:%s\tenviron:%s\n",file,getpid(),syscall(SYS_gettid),pthread_self(),getenv(idenv),(char*)pthread_getspecific(uuid_key),environ[k-1]);
    //
    //    log_message(test,strlen(test));
    
    //construct env list
    int i=0;
    //    char ex_env[1024]="";
    //    char *context=(char *)pthread_getspecific(uuid_key);
    //    if(is_null(context)){//get from env
    //        char envtmp[1024];
    //        int env_result=get_own_env(envtmp);
    //        sprintf(ex_env,"%s=%s",idenv,envtmp);
    //    }else{
    //        sprintf(ex_env,"%s=%s",idenv,(char*)pthread_getspecific(uuid_key));
    //    }
    //    int i=0;
    for(;environ[i]!=NULL;i++){}
    char * tenvp[i+2];
    int j=0;
    for (;j<i;j++){
        tenvp[j]=environ[j];
    }
    tenvp[j++]=ex_env;
    tenvp[j]=NULL;
    //to do
    //to decide whether to add UUID to environ variable
    
    static void *handle = NULL;
    static EXECVPE old_execvpe=NULL;
    if( !handle )
    {
        handle = dlopen("libc.so.6", RTLD_LAZY);
        old_execvpe = (EXECVPE)dlsym(handle, "execvpe");
        
    }
    return old_execvpe(file,argv,tenvp);
    
}

int execvpe(const char *file, char *const argv[], char *const envp[]){
    char test[4096]="";
    sprintf(test,"in execvpe file:%s\tpid:%d\tptid:%ld\ttid:%lu\tenvtest:%s\tTLD:%s\n",file,getpid(),syscall(SYS_gettid),pthread_self(),getenv(idenv),(char*)pthread_getspecific(uuid_key));
    log_message(test,strlen(test));
    
    static void *handle = NULL;
    static EXECVPE old_execvpe=NULL;
    if( !handle )
    {
        handle = dlopen("libc.so.6", RTLD_LAZY);
        old_execvpe = (EXECVPE)dlsym(handle, "execvpe");
    }
    return old_execvpe(file,argv,envp);
}

//wether a environment variable is null or unset
int is_null(char *env){
    if((!env)||(strcmp(env,"(null)")==0)||(strcmp(env,"null")==0)||(env=="")||(env=="(null)")||(env=="null")){
        return 1;
    }else{
        return 0;
    }
}

int get_own_env(char *env){
    int i=0;
    for(;environ[i]!=NULL;i++){}
    
    char *tmp=getenv(idenv);
    if (!is_null(tmp)){
        strcpy(env,tmp);
        char test[1024]="";
        return 0;
    }
    if(i==0){
        strcpy(env,"(null)");
        return 1;
    }
    
    char temp[1024];
    strcpy(temp,environ[i-1]);
    char *name=strtok(temp,"=");
    char *value=strtok(NULL,"=");
    if(strcmp(name,idenv)!=0){
        strcpy(env,"(null)");
        return 1;
    }else{
        strcpy(env,value);
        return is_null(env);
    }
    
}

//
//int clone(int (*fn)(void *), void *child_stack,int flags, void *arg, ... /* pid_t *ptid, struct user_desc *tls, pid_t *ctid */ ){
//    static void *handle = NULL;
//    static CLONE old_clone=NULL;
//    if( !handle )
//    {
//        handle = dlopen("libc.so.6", RTLD_LAZY);
//        old_clone = (CLONE)dlsym(handle, "clone");
//    }
//    log_message("clone",5);
//    return old_clone(fn,child_stack,flags,arg);
//}
//int posix_spawn(pid_t *pid, const char *path,const posix_spawn_file_actions_t *file_actions,const posix_spawnattr_t *attrp,
//                char *const argv[], char *const envp[]){
//    static void *handle = NULL;
//    static SPAWN old_spawn=NULL;
//    if( !handle )
//    {
//        handle = dlopen("libc.so.6", RTLD_LAZY);
//        old_spawn = (SPAWN)dlsym(handle, "posix_spawn");
//    }
//    log_message("spawn",5);
//
//    return old_spawn(pid,path,file_actions,attrp,argv,envp);
//
//}
//
//int posix_spawnp(pid_t *pid, const char *file,const posix_spawn_file_actions_t *file_actions,const posix_spawnattr_t *attrp,
//                 char *const argv[], char *const envp[]){
//    static void *handle = NULL;
//    static SPAWN old_spawnp=NULL;
//    if( !handle )
//    {
//        handle = dlopen("libc.so.6", RTLD_LAZY);
//        old_spawnp = (SPAWNP)dlsym(handle, "posix_spawnp");
//    }
//    log_message("spawnp",6);
//    return old_spawnp(pid,file,file_actions,attrp,argv,envp);
//
//}


//int socket(int domain, int type, int protocol){
//    static void *handle = NULL;
//    static SOCKET old_socket=NULL;
//    if( !handle )
//    {
//        handle = dlopen("libc.so.6", RTLD_LAZY);
//        old_socket = (SOCKET)dlsym(handle, "socket");
//    }
//    //http://man7.org/linux/man-pages/man2/socket.2.html
//    //http://pubs.opengroup.org/onlinepubs/009695399/basedefs/sys/socket.h.html
//    //https://www.gnu.org/software/libc/manual/html_node/Creating-a-Socket.html
//
//    char log_text[LOG_LENGTH]="init";
//
//    sprintf(log_text,"%s\t","in socket");
//    sprintf(log_text,"%s\t%s",log_text,"domain:");
//    switch (domain) {
//        case AF_INET:
//            sprintf(log_text,"%s\t%s\t",log_text,"AF_INET");
//            break;
//        case AF_INET6:
//            sprintf(log_text,"%s\t%s\t",log_text,"AF_INET6");
//            break;
//        case AF_IPX:
//            sprintf(log_text,"%s\t%s\t",log_text,"AF_IPX");
//            break;
//        case AF_UNIX:
//            sprintf(log_text,"%s\t%s\t",log_text,"AF_UNIX");
//            break;
//        default:
//            sprintf(log_text,"%s\t%s\t",log_text,"Unknown domain type");
//            break;
//    }
//    sprintf(log_text,"%s\t%s",log_text,"type:");
//    switch (type) {
//        case SOCK_STREAM:
//            sprintf(log_text,"%s\t%s\t",log_text,"SOCK_STREAM");
//            break;
//        case SOCK_DGRAM:
//            sprintf(log_text,"%s\t%s\t",log_text,"SOCK_DGRAM");
//            break;
//        case SOCK_RAW:
//            sprintf(log_text,"%s\t%s\t",log_text,"SOCK_RAW");
//            break;
//        case SOCK_PACKET:
//            sprintf(log_text,"%s\t%s\t",log_text,"SOCK_PACKET");
//            break;
//        case SOCK_SEQPACKET:
//            sprintf(log_text,"%s\t%s\t",log_text,"SOCK_SEQPACKET");
//            break;
//        case SOCK_RDM:
//            sprintf(log_text,"%s\t%s\t",log_text,"SOCK_RDM");
//            break;
//        default:
//            sprintf(log_text,"%s\t%s\t",log_text,"Unknown type");
//            break;
//    }
//    sprintf(log_text,"%s\t%s",log_text,"protocol:");
//    switch (protocol) {
//        case IPPROTO_TCP:
//            sprintf(log_text,"%s\t%s\t",log_text,"IPPROTO_TCP");
//            break;
//            //        case IPPTOTO_UDP:
//            //            log_event("IPPTOTO_UDP\t");
//            //            break;
//        case IPPROTO_SCTP:
//            sprintf(log_text,"%s\t%s\t",log_text,"IPPROTO_SCTP");
//            break;
//            //        case IPPROTO_TIPC:
//            //            log_event("IPPROTO_TIPC\t");
//            //            break;
//        default:
//            sprintf(log_text,"%s\t%s\t",log_text,"Unknown protocol type");
//            break;
//    }
//    int socketid=old_socket(domain,type,protocol);
//    sprintf(log_text,"%ssocketid:%d\tdomain:%d\ttype:%d\tprotocol:%d\n",log_text,socketid,domain,type,protocol);
//#ifdef SOCK
//    log_event(log_text);
//#endif
//    return socketid;
//}


//result in unknown fault, resource manager cannot start
//int connect(int socket, const struct sockaddr *addr, socklen_t length)
//{
//    static void *handle = NULL;
//    static CONN old_conn = NULL;
//    struct sockaddr_in sin;
//    struct sockaddr_in son;
//    socklen_t s_len = sizeof(sin);
//    if( !handle )
//    {
//        handle = dlopen("libc.so.6", RTLD_LAZY);
//        old_conn = (CONN)dlsym(handle, "connect");
//    }
//    if  ((getsockname(socket, (struct sockaddr *)&sin, &s_len) != -1) &&(getpeername(socket, (struct sockaddr *)&son, &s_len) != -1)){
//        unsigned short int in_port;
//        unsigned short int on_port;
//        char in_ip[256];
//        char on_ip[256];
//        char *tmp_in_ip;
//        char *tmp_on_ip;
//        in_port=ntohs(sin.sin_port);
//        on_port=ntohs(son.sin_port);
//        tmp_in_ip=inet_ntoa(sin.sin_addr);
//        memmove(in_ip,tmp_in_ip,strlen(tmp_in_ip)+1);
//        tmp_on_ip=inet_ntoa(son.sin_addr);
//        memmove(on_ip,tmp_on_ip,strlen(tmp_on_ip)+1);
//        char log_text[LOG_LENGTH]="init";
//#ifdef CONNECT
//        sprintf(log_text,"%s\t","in connect");
//        sprintf(log_text,"connect to %s:%d with %s:%d\t",on_ip,on_port,in_ip,in_port);
//#endif
//
//        // filter our own service, db and controller
//        if (check_filter(on_ip,in_ip,on_port,in_port)==0){
//#ifdef CONNECT
//            sprintf(log_text,"%s\t%s\n",log_text,"connect done with filter!");
//            log_event(log_text);
//#endif
//            return old_conn(socket, addr, length);
//        }else{
//
//            sprintf(log_text,"%s\t%s\n",log_text,"connect done!");
//            log_event(log_text);
//            //            printf("%s\n", "in conn filter1");
//            //            mark_socket_connect(socket);
//            //            printf("%s\n", "in conn filter2");
//            return old_conn(socket, addr, length);
//
//        }
//
//    }
//    return old_conn(socket, addr, length);
//}

//result in *** stack smashing detected ***: ssh terminated
int close(int fd)
{
    static void *handle = NULL;
    static CLOSE old_close = NULL;
    struct sockaddr_in sin;
    struct sockaddr_in son;
    socklen_t s_len = sizeof(sin);
    if( !handle )
    {
        handle = dlopen("libc.so.6", RTLD_LAZY);
        old_close = (CLOSE)dlsym(handle, "close");
    }
    //if fd is not a socket, return directly

    if (!is_socket(fd)){
        return old_close(fd);
    }
    
    sa_family_t socket_family=get_socket_family(fd);
    
    if (socket_family==AF_INET){

        struct sockaddr_in sin; //local socket info
        struct sockaddr_in son; //remote socket into
        socklen_t s_len = sizeof(sin);
        if ((getsockname(fd, (struct sockaddr *)&sin, &s_len) != -1) &&(getpeername(fd, (struct sockaddr *)&son, &s_len) != -1))
        {
            //https://linux.die.net/man/3/inet_ntoa
            unsigned short int in_port;
            unsigned short int on_port;
            char in_ip[256];
            char on_ip[256];
            char *tmp_in_ip;
            char *tmp_on_ip;
            in_port=ntohs(sin.sin_port);
            on_port=ntohs(son.sin_port);
            tmp_in_ip=inet_ntoa(sin.sin_addr);
            memmove(in_ip,tmp_in_ip,strlen(tmp_in_ip)+1);
            tmp_on_ip=inet_ntoa(son.sin_addr);
            memmove(on_ip,tmp_on_ip,strlen(tmp_on_ip)+1);
            ssize_t n=0;
            if(check_filter(on_ip,in_ip,on_port,in_port)==0){//controller or outer service
                return old_close(fd);
            }else{
//                mark_socket_close(fd);
                op_storage(S_RELEASE,fd,NULL,0,0);
                return old_close(fd);
            }
        }else{
            return old_close(fd);
        }
    }else{
       return old_close(fd);
    }

}

sa_family_t get_socket_family(int sockfd){
    struct sockaddr_storage tt_in;
    struct sockaddr_storage tt_on;
    struct sockaddr *t_in=(struct sockaddr *)&tt_in;
    struct sockaddr *t_on=(struct sockaddr *)&tt_on;
    socklen_t s_len = sizeof(tt_in);
    if ((getsockname(sockfd, t_in, &s_len) != -1) &&(getpeername(sockfd, t_on, &s_len) != -1))
    {
        return t_in->sa_family;
    }else{
        return UNKNOWN_FAMILY;
    }
    
}

char ** init_uuids(ssize_t m){
    char **a=(char **)malloc(10 * sizeof(char *));
    int i=0;
    for (i=0;i<10;i++){
        a[i]=malloc(ID_LENGTH);
    }
    return a;
}

void free_uuids(char ** a){
    int i;
    for(i = 0; i < 10; i++){
        free(a[i]);
    }
    free(a);
    
}

//int get_uuid(char result[][ID_LENGTH],const char* buf,int len,char* after_uuid){
int get_uuid(char **result,const char* buf,ssize_t len,char* after_uuid,int sockfd){
    int id_count=0;
    int i;
    int j=0;
    short start=0;
    
    int inner_flag=1;
    int test_flag=0;
    int k=0;
    
    for(i=0;i<len;i++){
        if ((buf[i]=='@')&&((i+1)<len)&&(buf[i+1]=='@')){
            if((i+ID_LENGTH)>len){
                char log_text[1024]="test";
                for(k=i+2;k<len;k++){
                    sprintf(log_text,"%s\t%c ",log_text,buf[k]);
                    if(((buf[k]>=97)&&(buf[k]<=122))||((buf[k]>=48)&&(buf[k]<=57))||(buf[k]=='-')||(buf[k]=='@')){
                        //log_message
                        sprintf(log_text,"%strue ",log_text);
                        inner_flag=1;
                    }else{
                        sprintf(log_text,"%sfalse ",log_text);
                        inner_flag=0;
                        break;
                    }
                }
                test_flag=1;
                sprintf(log_text,"%sstart:%d\tpointer:%d\tlength:%d\tresult:%d\tflag:%d\n",log_text,i,j,len,inner_flag,test_flag);
                //                log_message(log_text,strlen(log_text));
                //log_message(buf,len);
                
                if (inner_flag==0){//normal case
                    after_uuid[j++]=buf[i];
                    inner_flag=1;
                }else{
                    
                    //possiblely broken uuids, currently processed as broken uuid
                    memmove(result[id_count++],&buf[i],len-i);
                    int rflag=readid(sockfd,len-i,result[id_count-1]);
                    if (rflag!=0){
                        log_event("here2");
                        log_event("prefix broken uuid\n");
                        memmove(after_uuid+j,&buf[i],len-i);
                        i=len-1;
                        j=j+len-i;//maybe
                        
                        if(rflag!=-1){
                            //                            test_storage(len-i,rflag,result[id_count-1]);
                            op_storage(S_PUT,sockfd,result[id_count-1],rflag,len-i);
                        }else{
                            log_event("alarm3");
                        }
                        id_count=id_count-1;
                        
                        //                        after_uuid[j++]=buf[i];
                    }else{
                        log_event("here3");
                        //                    log_message(result[id_count-1],ID_LENGTH);
                        i=len-1;
                        inner_flag=1;
                    }
                }
                
            }else if((buf[i+ID_LENGTH-2]=='@')&&(buf[i+ID_LENGTH-3]=='@')&&(buf[i+ID_LENGTH-1]=='\0')){
                memmove(result[id_count++],&buf[i],ID_LENGTH);
                //log_message(buf+i,ID_LENGTH);
                i=i+ID_LENGTH-1;
            }else{
                //normal ``
                after_uuid[j++]=buf[i];
            }
            
        }else if((buf[i]=='@') &&(len==1)){
            memmove(result[id_count++],&buf[i],1);
            int rflag=readid(sockfd,1,result[id_count-1]);
            if (rflag!=0){
                log_event("here6");
                after_uuid[j++]=buf[i];
                if (rflag!=-1){
                    //                    test_storage(1,rflag,result[id_count-1]);
                    op_storage(S_PUT,sockfd,result[id_count-1],rflag,1);
                }else{
                    log_event("alarm5");
                }
                char tmp[1024]="";
                sprintf(tmp,"length:%ld\ti:%d\tj:%d\n",len,i,j);
                id_count=id_count-1;
                
                //                log_event("prefix broken uuid2\n");
                //                log_message(result[id_count-1],rflag+1);
            }else{
                log_event("here7");
                //                log_message(result[id_count-1],ID_LENGTH);
            }
        }else if((buf[i]=='@') &&((i+1)==len)){
            memmove(result[id_count++],&buf[i],1);
            int rflag=readid(sockfd,1,result[id_count-1]);
            if (rflag!=0){
                log_event("here8");
                after_uuid[j++]=buf[i];
                if (rflag!=-1){
                    //                    test_storage(1,rflag,result[id_count-1]);
                    op_storage(S_PUT,sockfd,result[id_count-1],rflag,1);
                }else{
                    log_event("alarm6");
                }
                char tmp[1024]="";
                sprintf(tmp,"length:%ld\ti:%d\tj:%d\n",len,i,j);
                id_count=id_count-1;
                
                //                log_event("prefix broken uuid2\n");
                //                log_message(result[id_count-1],rflag+1);
            }else{
                log_event("here9");
                log_message(result[id_count-1],ID_LENGTH);
            }
        }else{
            after_uuid[j++]=buf[i];
        }
    }
    
    return j;
}

ssize_t op_storage(int type,int sockfd,char * buf, ssize_t count,ssize_t already){
    
    static struct storage* buffer_storage[S_SIZE];
    static short initialized=0;
    if(initialized==0){
        int i;
        for(i=0;i<S_SIZE;i++){
            buffer_storage[i]=malloc(sizeof(struct storage));
            buffer_storage[i]->sockfd=0;
            buffer_storage[i]->used=0;
            buffer_storage[i]->buffer=(char *)malloc(ID_LENGTH);
        }
        initialized=1;
    }
    if(type==S_PUT){//put
        int  i;
        for(i=0;i<S_SIZE;i++){
            if (buffer_storage[i]->used==0){
                buffer_storage[i]->used=1;
                buffer_storage[i]->sockfd=sockfd;
                buffer_storage[i]->buffer[0]=(char)count;
                memmove(buffer_storage[i]->buffer+1,buf+already,count);
                log_message("save",5);
                log_message(buffer_storage[i]->buffer+1,count);
                char tmp[1024]="";
                sprintf(tmp,"socket:%d\tput!\n",sockfd);
                log_message3(tmp,strlen(tmp));
                return 0;
            }
        }
        log_message("No space!",10);
        return -1;
        
    }else if(type==S_GET){//get
        int i;
        char tmp[1024]="";
        sprintf(tmp,"socket:%d\tget\t",sockfd);
        for(i=0;i<S_SIZE;i++){
            if(buffer_storage[i]->sockfd==sockfd){
                sprintf(tmp,"%s\tfound:%d!",tmp,i);
                int len=(int)buffer_storage[i]->buffer[0];
                
                if (len>count){
                    log_event("alarm!");
                    int rest=len-count;
                    memmove(buf,buffer_storage[i]->buffer+1,count);
                    buffer_storage[i]->buffer[0]=(char)rest;
                    memmove(buffer_storage[i]->buffer+1,buffer_storage[i]->buffer+count+1,rest);
                    log_message("load1",5);
                    log_message(buf,count);
                    sprintf(tmp,"%s\treturn:%d\n!",tmp,count);
                    log_message3(tmp,strlen(tmp));
                    return count;
                }else{
                    memmove(buf,buffer_storage[i]->buffer+1,len);
                    buffer_storage[i]->used=0;
                    buffer_storage[i]->sockfd=0;
                    log_message("load2",5);
                    log_message(buf,len);
                    sprintf(tmp,"%s\treturn:%d\n!",tmp,len);
                    log_message3(tmp,strlen(tmp));
                    return len;
                }
            }
        }
        sprintf(tmp,"%s\t not found!\n",tmp);
        log_message3(tmp,strlen(tmp));
        return 0;
        
    }else if(type==S_RELEASE){//release
        int i;
        for(i=0;i<S_SIZE;i++){
            if(buffer_storage[i]->sockfd==sockfd){
                log_message("release",8);
                buffer_storage[i]->used=0;
                buffer_storage[i]->sockfd=0;
                return 0;
            }
        }
        return 0;
        
    }else{
        return -1;
    }
}


void init_string(struct string *s) {
    s->len = 0;
    s->ptr = malloc(s->len+1);
    if (s->ptr == NULL) {
        fprintf(stderr, "malloc() failed\n");
        exit(EXIT_FAILURE);
    }
    s->ptr[0] = '\0';
}

size_t writefunc(void *ptr, size_t size, size_t nmemb, struct string *s)
{
    size_t new_len = s->len + size*nmemb;
    s->ptr = realloc(s->ptr, new_len+1);
    if (s->ptr == NULL) {
        fprintf(stderr, "realloc() failed\n");
        exit(EXIT_FAILURE);
    }
    memcpy(s->ptr+s->len, ptr, size*nmemb);
    s->ptr[new_len] = '\0';
    s->len = new_len;
    
    return size*nmemb;
}

// to start post and return the response
int getresponse(char* post_parameter){
    CURL *curl;
    CURLcode res;
    
    curl = curl_easy_init();
    if(curl) {
        struct string s;
        init_string(&s);
        
        curl_easy_setopt(curl, CURLOPT_URL, controller_service);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_parameter);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
        res = curl_easy_perform(curl);
        
        int result_flag;
        //printf("%s\n", s.ptr);
        if(strcmp(s.ptr, "0") == 0){
            result_flag=0;
        }else{
            result_flag=1;
            
        }
        
        free(s.ptr);
        
        /* always cleanup */
        curl_easy_cleanup(curl);
        return result_flag;
    }
}

/**
 * Create random UUID
 *
 * @param buf - buffer to be filled with the uuid string
 */
char *random_uuid( char buf[ID_LENGTH] )
{
    //    char * orinigal=(char *)pthread_getspecific(uuid_key);
    //    if (orinigal){
    //        int i;
    //        for(i=0;i<ID_LENGTH;i++){
    //            buf[i]=orinigal[i];
    //        }
    //    }else{
    uuid_t uuid;
    uuid_generate(uuid);
    uuid_unparse(uuid, buf);
    //    }
    
    buf[ID_LENGTH-3]='@';
    buf[ID_LENGTH-2]='@';
    buf[ID_LENGTH-1]='\0';
    buf[0]='@';
    buf[1]='@';
    return buf;
}
long long gettime(){
    struct timeb t;
    ftime(&t);
    return 1000 * t.time + t.millitm;
}

int readid(int socket,int start,char * buf){
    static void *handle = NULL;
    static READ old_read = NULL;
    if( !handle )
    {
        handle = dlopen("libc.so.6", RTLD_LAZY);
        old_read = (READ)dlsym(handle, "read");
    }
    
    int length=ID_LENGTH-start;
    int len=0;
    int numLeft=length;
    int numBytes;
    do {
        numBytes = old_read(socket, buf + start+len, numLeft);
        if(numBytes<=0){
            if(len==0){
                return -1;
            }else{
                return len;
            }
            
        }
        len += numBytes;
        numLeft -= numBytes;
    } while(numLeft > 0 && len < length);
    
    if ((len==length)&&(buf[ID_LENGTH-1]=='\0')){
        int flag=1;
        int i;
        for(i=start;i<(ID_LENGTH-1);i++){
            if(((buf[i]>=97)&&(buf[i]<=122))||((buf[i]>=48)&&(buf[i]<=57))||(buf[i]=='-')||(buf[i]=='@')){
                flag=1;
            }else{
                flag=0;
            }
            
        }
        if (flag==1){
            return 0;
        }else{
            return len;
        }
        
    }else{
        return len;
    }
}

//wether a file descriptor is a socket descriptor
int is_socket(int fd){
    struct stat statbuf;
    fstat(fd, &statbuf);
    if (S_ISSOCK(statbuf.st_mode)){
        return 1;
    }else{
        return 0;
    }
}


void log_event(char* info){
    FILE *logFile;
    if((logFile=fopen(filePath,"a+"))!=NULL){
        fprintf(logFile,"%s\n", info);
        fclose(logFile);
        
    }else{
        printf("Cannot open file!");
        
    }
    
    //    char fileName[1024];
    //    sprintf(fileName,"/tmp/tracelog%d.log",syscall(SYS_gettid));
    //    FILE *logFileThread;
    //    if((logFileThread=fopen(fileName,"a+"))!=NULL){
    //        fprintf(logFileThread,"%s\n", info);
    //        fclose(logFileThread);
    //
    //    }else{
    //        printf("Cannot open file!");
    //
    //    }
}

int log_message3(char message[],size_t length){
    if(length<1){
        return 0;
    }
    if(length>1000){
        return 0;
    }
    FILE *logFile;
    char result[LOG_LENGTH]="";
    
    if((logFile=fopen("/tmp/tracelog3.txt","a+"))!=NULL){
        sprintf(result,"message:\tpid:%ld\tktid:%ld\ttid:%lu\t",getpid(),syscall(SYS_gettid),pthread_self());
        
        int i=0;
        //        if(length>ID_LENGTH){
        //            i=length-10; //only show the last ten char
        //        }
        for(;i<length;i++){
            sprintf(result,"%s%c",result,message[i]);
            //            sprintf(result,"%s%d-",result,message[i]);
        }
        sprintf(result,"%s%c",result,'\0');
        fprintf(logFile,"%s\n", result);
        fclose(logFile);
    }else{
        printf("Cannot open file!");
    }
    return 0;
}

void log_message(char message[],int length){
    //#ifndef MESSAGE
    //    return;
    //#endif
    int controll=length;
    if(length<1){
        return;
    }
    if(length>600){
        controll=600;
    }//
    FILE *logFile;
    char result[LOG_LENGTH]="";
    
    if((logFile=fopen(filePath2,"a+"))!=NULL){
        sprintf(result,"message:\t");
        
        int i=length-controll;
        //        if(length>ID_LENGTH){
        //            i=length-10; //only show the last ten char
        //        }
        for(length-controll;i<length;i++){
            sprintf(result,"%s%c",result,message[i]);
            //            sprintf(result,"%s%d-",result,message[i]);
        }
        sprintf(result,"%s%c",result,'\0');
        fprintf(logFile,"%s\n", result);
        fclose(logFile);
    }else{
        printf("Cannot open file!");
    }
    
}

//return 0, if this message is recv/send from outer host/port and it should be recv/send directly
//return 1, if this message is internal message
int check_filter(char* on_ip,char* in_ip,int on_port,int in_port){
    int ip_list_length=3;
    int port_list_length=3;
    char *legal_ip_list[]={"127.0.0.1","10.211.55.36","10.211.55.38"};
    int illegal_port_list[]={22,53,80};
    int i=0;
    short internal_on_ip=0;
    short internal_in_ip=0;//0  means it's outer ip
    for(i=0;i<ip_list_length;i++){
        if(strcmp(on_ip,legal_ip_list[i])==0){
            internal_on_ip=1;
        }
        if(strcmp(in_ip,legal_ip_list[i])==0){
            internal_in_ip=1;
        }
    }
    if((internal_in_ip==0)||(internal_on_ip==0)){
        return 0;
    }
    
    for(i=0;i<port_list_length;i++){
        if((on_port==illegal_port_list[i])||(in_port==illegal_port_list[i])){
            return 0;
        }
    }
    char log[1024];
    //    sprintf(log,"on_ip:%s\tin_ip:%s\ton_port:%d\tin_port:%d\t",on_ip,in_ip,on_port,in_port);
    //    log_event(log);
    return 1;
}

//return 1, this message contains a job_id
//return 0, this message doesn't contain a job_id
int find_job(char* content, char *message,int length){
    
    int flag=0;
    int i;
    for(i=0;i<length-4;i++){
        if(message[i]=='j'){
            if((message[i+1]=='o')&&(message[i+2]=='b')&&(message[i+3]=='_')){
                flag=1;
                
                // add job_id prefix
                int prefix=60;
                if (i<60){
                    prefix=i;
                }
                //                int j=i-prefix;
                int j=0;
                for(;j<prefix;j++){
                    if ((message[i-prefix+j]<=0)||(message[i-prefix+j]==34)||(message[i-prefix+j]==39)){
                        content[j]='-';
                    }else{
                        content[j]=message[i-prefix+j];
                    }
                }
                //add prefix done
                
                
                //add job_id and suffix
                int suffix=100;
                if((length-i)<100){
                    suffix=length-i;
                }
                int k=0;
                for(;k<suffix;k++){
                    if ((message[i+k]<=0)||(message[i+k]==34)||(message[i+k]==39)){
                        content[j+k]='-';
                    }else{
                        content[j+k]=message[i+k];
                    }
                }
                //add suffix done
                content[j+k+1]='\0';
                log_event(content);
                return flag;
            }
        }
    }
    return flag;
}

void translate(char * content,char *message,int length){
    if (length<=0){
        content[0]='\0';
        return;
    }else {
        int flag=length;
        if (length>1024){
            flag=1024;
        }
        int i=0;
        for(;i<flag;i++){
            if ((message[i]<=0)||(message[i]==34)||(message[i]==39)){
                content[i]='-';
            }else{
                content[i]=message[i];
            }
        }
        content[flag-1]='\0';
        return;
    }
}


//type=24
int push_to_database(char* on_ip,int on_port,char* in_ip, int in_port,pid_t pid,pthread_t tid,char* uuid,long long time,char ftype,long length,long supposed_length,long rlength,int socketid,char dtype,char * message){
    if(uuid_key==12345){
        pthread_key_create(&uuid_key,NULL);
        char * tmp_env=(char *)malloc(1024); //never free
        int env_null=get_own_env(tmp_env);
        log_message(tmp_env,strlen(tmp_env));
        if(!env_null){
            pthread_setspecific(uuid_key,tmp_env);
        }
        
    }
//
    char * unit_uuid;
    if ((ftype%2)==0){
        if (uuid!=""){
            unit_uuid=malloc(ID_LENGTH);
            random_uuid(unit_uuid);
            pthread_setspecific(uuid_key,(void *)unit_uuid);
            //            unit_uuid=uuid;
        }else{
            unit_uuid=(char *)pthread_getspecific(uuid_key);
            //            if(((!unit_uuid)||(unit_uuid==""))&&(length==-1)){
            if((is_null(unit_uuid))&&(length==-1)){
                //            to do
                //            decide how to deal with this special case
                
                unit_uuid=malloc(ID_LENGTH);
                random_uuid(unit_uuid);
                pthread_setspecific(uuid_key,(void *)unit_uuid);
            }
        }
    }else{
        if (uuid!=""){
            unit_uuid=(char *)pthread_getspecific(uuid_key);
            pthread_setspecific(uuid_key,(void *)uuid);
        }else{
            unit_uuid=(char *)pthread_getspecific(uuid_key);
        }
    }
    if (!unit_uuid){
        unit_uuid="";
    }
//    // To do
//    
    char *tmpMsg=(char *)malloc(1024);
    char *tmpMsg2=(char *)malloc(1024);
    if((ftype==F_SEND)||(ftype==F_WRITE)){
        
        //for job ID
        if((supposed_length>0)&&(supposed_length<200000)){
            if(!find_job(tmpMsg,message,rlength)){
                tmpMsg[0]='\0';
            }
        }else{
            tmpMsg[0]='\0';
        }
        
        // for message
        translate(tmpMsg2,message,rlength);
    }else{
        tmpMsg[0]='\0';
        tmpMsg2[0]='\0';
    }
    
    char * parameter_tmp="on_ip=%s&on_port=%d&in_ip=%s&in_port=%ld&pid=%u&ktid=%ld&tid=%lu&uuid=%s&unit_uuid=%s&ttime=%lld&ftype=%d&length=%ld&supposed_length=%ld&rlength=%ld&rtype=%d&socket=%d&dtype=%d&message=%s&message2=%s";
    char parameter[4096];
    sprintf(parameter,parameter_tmp,on_ip,on_port,in_ip,in_port,pid,syscall(SYS_gettid),tid,uuid,unit_uuid,time,ftype,length,supposed_length,rlength,R_DATABASE,socketid,dtype,tmpMsg,tmpMsg2);
    free(tmpMsg);
    free(tmpMsg2);
//
    return getresponse(parameter);
//     return 0;
}

//type=27
int push_thread_db(pid_t pid,pid_t ktid,pthread_t tid,pid_t ppid,pid_t pktid, pthread_t ptid,long long time){
    // To do
    char * parameter_tmp="pid=%ld&ktid=%ld&tid=%lu&ppid=%ld&pktid=%ld&ptid=%lu&ttime=%lld&rtype=%d";
    char parameter[1024];
    sprintf(parameter,parameter_tmp,pid,ktid,tid,ppid,pktid,ptid,time,R_THREAD);
    //    log_message(parameter,strlen(parameter));
    return getresponse(parameter);
}
//type=28
int push_thread_dep(pid_t pid,pid_t ktid,pthread_t dtid,pthread_t jtid,long long time){
    char * parameter_tmp="pid=%ld&ktid=%ld&dtid=%lu&jtid=%lu&ttime=%lld&rtype=%d";
    char parameter[1024];
    sprintf(parameter,parameter_tmp,pid,ktid,dtid,jtid,time,R_THREAD_DEP);
    //    log_message(parameter,strlen(parameter));
    return getresponse(parameter);
    
}

//type=27
int push_thread_db2(char *parameter){
    // To do
    //    char * parameter_tmp="pid=%ld&ktid=%ld&tid=%lu&ppid=%ld&pktid=%ld&ptid=%lu&ttime=%lld&rtype=%d";
    //    char parameter[1024];
    //    sprintf(parameter,parameter_tmp,pid,ktid,tid,ppid,pktid,ptid,time,R_THREAD);
    //    log_message(parameter,strlen(parameter));
    return getresponse(parameter);
}


////type=25
//int mark_socket_connect(int socketid){
//    char * parameter_tmp="uuid=%s&rtype=%d&socket=%d";
//    char parameter[1024];
//    sprintf(parameter,parameter_tmp,"socket",25,socketid);
//    return getresponse(parameter);
//}
//
////type=26
//int mark_socket_close(int socketid){
//    char * parameter_tmp="uuid=%s&rtype=%d&socket=%d";
//    char parameter[1024];
//    sprintf(parameter,parameter_tmp,"socket",26,socketid);
//    return getresponse(parameter);
//}


ssize_t check_read(char *uuids, char* buf,size_t count,int sockfd, ssize_t* length,char * status){
    static void *handle = NULL;
    static READ old_read = NULL;
    if( !handle )
    {
        handle = dlopen("libc.so.6", RTLD_LAZY);
        old_read = (READ)dlsym(handle, "read");
    }
    char log_text[LOG_LENGTH]="read\t";
    sprintf(log_text,"%scount:%d\t",log_text,count);
    
    
    
    
    char id[ID_LENGTH];
    char temp[count];
    
    //    ssize_t last_time=get_storage(temp,count);
    ssize_t last_time=op_storage(S_GET,sockfd,temp,count,0);
    *length=last_time;
    if (last_time==count){
        log_event("alarm2!");
        memmove(buf,temp,last_time);
        *status=RECV_NORMAL_ST;
        
        return count;
    }
    
    ssize_t m=old_read(sockfd,temp+last_time,count-last_time);
    
    sprintf(log_text,"%sget:%d\tlasttime:%d\t",log_text,m,last_time);
    if (m<=0){
        if(last_time==0){
            sprintf(log_text,"%sreturn:%d\terror1.1\n",log_text,m);
            log_event(log_text);
            *status=RECV_ERROR;
            return m;
        }else{
            sprintf(log_text,"%sreturn:%d\terror1.2\n",log_text,last_time);
            memmove(buf,temp,last_time);
            log_event(log_text);
            *status=RECV_NORMAL_ST_ERR;
            return last_time;
        }
    }else{
        *length=*length+m;
        char ** result=init_uuids(m+last_time);//remember to freee
        ssize_t data_len=get_uuid(result,temp,m+last_time,buf,sockfd);
        sprintf(log_text,"%safter:%d\t",log_text,data_len);
        if (data_len==0){// it's all uuid
            m=old_read(sockfd,buf,count);
            //            log_message(buf,m);
            if(m>=ID_LENGTH){//actually need to check wether it contains uuid again
                log_event("alarm4");
            }
            *status=RECV_NORMAL_ERR;
            if(m>=0){
              *length=*length+m;
                *status=RECV_NORMAL1;
            }
            memmove(uuids,result[0],ID_LENGTH);
            sprintf(log_text,"%sreturn:%d\tuuid:%s\tnormal1\n",log_text,m,uuids);
            log_event(log_text);
            return m;
        }else if((m+last_time)==data_len){
            //            memmove(buf,temp)
            //            free_uuids(result);
            //            log_message(buf,m+last_time);
            sprintf(log_text,"%sreturn:%d\tnormal2\n",log_text,m+last_time);
            log_event(log_text);
            *status=RECV_NORMAL2;
            return m+last_time;
        }else{
            memset(uuids,0,ID_LENGTH);
            int num=(m+last_time-data_len)/ID_LENGTH;
            int rest=(m+last_time-data_len)%ID_LENGTH;
            if (rest!=0){
                num=num+1;
            }
            int t=0;
            for(;t<num;t++){
                strcat(uuids,result[t]);
            }
            //            log_message(buf,data_len);
            sprintf(log_text,"%sreturn:%d\tuuids:%s\tless\n",log_text,data_len,uuids);
            log_event(log_text);
            *status=RECV_NORMAL_ID;
            return data_len;
        }
        
    }
    
}

