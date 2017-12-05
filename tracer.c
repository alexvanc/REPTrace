#include "tracer.h"





//0 controller off
//1 controller on
const short with_uuid=1;

const char* controllerip="10.211.55.38";
const char* controller_service="10.211.55.38/controller.php";
const char* filePath="/tmp/tracelog.txt";
const char* filePath2="/tmp/tracelog2.txt";

const char* idenv="UUID_PASS";
static pthread_key_t uuid_key=12345;
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
                push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),"",gettime(),F_RECV,n,len,sockfd,RECV_FILTER);
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
                    push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),"",gettime(),F_RECV,n,len,sockfd,RECV_ERROR);
                    errno=tmp_erro;
                    return n;
                }
#ifdef MESSAGE
                log_message(result,n);
#endif
#ifdef DEBUG
                sprintf(log_text,"%s%s\n",log_text,"recv done without uuid!");
                log_event(log_text);
#endif
                push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),"",gettime(),F_RECV,n,len,sockfd,RECV_NORMALLY);
                return n;
            }else{
#ifdef DATA_INFO
                sprintf(log_text,"%srecv buff length:%zu\t socket recv buff length:%d\t socket send buff length:%d\t",log_text,len,recv_size,send_size);
                sprintf(log_text,"%srecv process:%u\t thread:%lu\t socketid:%d\t",log_text,getpid(),pthread_self(),sockfd);
#endif
                char result[len];
                //                char id[ID_LENGTH];
                char *id=malloc(ID_LENGTH);//memory collected by the pthread_key_create
                long long ttime;
                ssize_t m=old_recv(sockfd, id, 1, flags);
                if (m<=0){//no message left or errors happened!
#ifdef RW_ERR
                    sprintf(log_text,"%sreal recv data length:%ld\terrorno:%d\t%s\t%s\n",log_text,m,errno,strerror(errno),"recv done with read error!");
                    log_event(log_text);
#endif
                    int tmp_erro=errno;
                    push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),"",gettime(),F_RECV,m,len,sockfd,RECV_ERROR);
                    errno=tmp_erro;
                    return m;
                }
                if(id[0]=='^'){
                    m=old_recv(sockfd,&id[1],1,flags);
                    if(m<=0){
                        memmove(buf,id,1);
                        sprintf(log_text,"%sreal recv data length:%ld\t%s\t%s\n",log_text,1L,strerror(errno),"recv done 2 with read error!");
                        log_event(log_text);
                        push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),"",gettime(),F_RECV,1L,len,sockfd,RECV_ERROR);
                        return 1L;
                    }
                    if(id[1]=='^'){//get uuid start
                        m=old_recv(sockfd,&id[2],ID_LENGTH-2,flags);
                        if(m==(ID_LENGTH-2)){
                            if((id[ID_LENGTH-2]=='^')&&(id[ID_LENGTH-3]=='^')){
                                // get complete uuid
#ifdef DEBUG
                                sprintf(log_text,"%s%s\t",log_text,"id extracted");
#endif
                                //for multi-write to one read
                                m=old_recv(sockfd,result,len,flags);
                                //                                char uuids[m/ID_LENGTH][ID_LENGTH];
                                char **uuids=init_uuids(m);
                                char after_uuid[m];
                                size_t data_len=0;
                                data_len=get_uuid(uuids,result,m,after_uuid);
                                if(data_len==m){
                                    memmove(buf,result,m);
#ifdef DEBUG
                                    sprintf(log_text,"%sreal recv data length:%ld\t%s\n",log_text,m,"recv done!");
                                    log_event(log_text);
#endif
                                    push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),id,gettime(),F_RECV,m+ID_LENGTH,len,sockfd,RECV_SINGLE);
                                    return m;
                                }else{
                                    memmove(buf,after_uuid,data_len);
#ifdef DEBUG
                                    sprintf(log_text,"%sreal recv data length:%zu\t%s\n",log_text,data_len,"multi uuid !recv done!");
                                    log_event(log_text);
#endif
                                    int num=(m-data_len)/ID_LENGTH;
                                    //                                    char mul_id[LOG_LENGTH];
                                    char *mul_id=(char *)malloc(ID_LENGTH*num+ID_LENGTH);
                                    strcat(mul_id,id);
                                    int t=0;
                                    for(;t<num;t++){
                                        strcat(mul_id,uuids[t]);
                                    }
                                    push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),mul_id,gettime(),F_RECV,data_len+ID_LENGTH,len,sockfd,RECV_MULTI_BRO);
                                    return data_len;
                                }
                                //for multi-write to one read
                            }else{
                                
                                //for multi-write to one read
                                memmove(buf,id,ID_LENGTH);
                                m=old_recv(sockfd,result,len-ID_LENGTH,flags);
                                //                                char uuids[m/ID_LENGTH][ID_LENGTH];
                                char **uuids=init_uuids(m);;
                                char after_uuid[m];
                                size_t data_len=0;
                                data_len=get_uuid(uuids,result,m,after_uuid);
                                if(data_len==m){
                                    memmove((char *)buf+ID_LENGTH,result,m);
#ifdef DEBUG
                                    sprintf(log_text,"%sreal recv data length:%ld\t%s\n",log_text,m+ID_LENGTH,"recv done with similar uuid!");
                                    log_event(log_text);
#endif
                                    push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),"",gettime(),F_RECV,m+ID_LENGTH,len,sockfd,RECV_SIM1);
                                    return m+ID_LENGTH;
                                }else{
                                    memmove((char *)buf+ID_LENGTH,after_uuid,data_len);
#ifdef DEBUG
                                    sprintf(log_text,"%sreal recv data length:%zu\t%s\n",log_text,data_len+ID_LENGTH,"recv done with multi and similar uuid!");
                                    log_event(log_text);
#endif
                                    int num=(m-data_len)/ID_LENGTH;
                                    //                                    char mul_id[LOG_LENGTH];
                                    char *mul_id=(char *)malloc(ID_LENGTH*num+ID_LENGTH);
                                    int t=0;
                                    for(;t<num;t++){
                                        strcat(mul_id,uuids[t]);
                                    }
                                    push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),mul_id,gettime(),F_RECV,data_len+ID_LENGTH,len,sockfd,RECV_SIM1_ID);
                                    //                                    pthread_setspecific(uuid_key,(void *)uuids[0]);
                                    return data_len+ID_LENGTH;
                                }
                                //for multi-write to one read
                            }
                        }else{
#ifdef DEBUG
                            sprintf(log_text,"%sreal recv data length:%ld\t%s\n",log_text,m+2,"recv done with 2 starting symbols!");
                            log_event(log_text);
#endif
                            push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),"",gettime(),F_RECV,m+2L,len,sockfd,RECV_SIM2);
                            //not a uuid
                            if(len==(m+2)){
                                memmove(buf,id,len);
                                
                                return len;
                            }else{
                                //not a uuid or network error and will result in application error
                                memmove(buf,id,m+2);
                                return m+2L;
                            }
                        }
                    }else{//not a uuid
                        
                        memmove(buf,id,2);
                        //                        m=old_recv(sockfd,&buf[2],len-2,flags);
                        
                        //for multi-write to one read
                        m=old_recv(sockfd,result,len-2,flags);
                        //                        char uuids[m/ID_LENGTH][ID_LENGTH];
                        char **uuids=init_uuids(m);
                        char after_uuid[m];
                        size_t data_len=0;
                        data_len=get_uuid(uuids,result,m,after_uuid);
                        if(data_len==m){
                            memmove((char *)buf+2,result,m);
#ifdef DEBUG
                            sprintf(log_text,"%sreal recv data length:%ld\t%s\n",log_text,m+2,"recv done with 1 starting symbols!");
                            log_event(log_text);
#endif
                            push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),"",gettime(),F_RECV,m+2L,len,sockfd,RECV_SIM3);
                            return m+2L;
                        }else{
                            memmove((char *)buf+2,after_uuid,data_len);
#ifdef DEBUG
                            sprintf(log_text,"%sreal recv data length:%ld\t%s\n",log_text,2+data_len,"recv done with 1 starting symbols and multi uuid!");
                            log_event(log_text);
#endif
                            int num=(m-data_len)/ID_LENGTH;
                            //                            char mul_id[LOG_LENGTH];
                            char *mul_id=(char *)malloc(ID_LENGTH*num+ID_LENGTH);
                            int t=0;
                            for(;t<num;t++){
                                strcat(mul_id,uuids[t]);
                            }
                            push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),mul_id,gettime(),F_RECV,2L+data_len,len,sockfd,RECV_SIM3_ID);
                            //                            pthread_setspecific(uuid_key,(void *)uuids[0]);
                            return 2L+data_len;
                        }
                        //for multi-write to one read
                    }
                    
                }else{
                    
                    memmove(buf,id,1);
                    //                    m=old_recv(sockfd,&buf[1],len-1,flags);
                    
                    //for multi-write to one read
                    m=old_recv(sockfd,result,len-1,flags);
                    //                    char uuids[m/ID_LENGTH][ID_LENGTH];
                    char **uuids=init_uuids(m);
                    char after_uuid[m];
                    size_t data_len=0;
                    data_len=get_uuid(uuids,result,m,after_uuid);
                    if(data_len==m){
                        memmove((char *)buf+1,result,m);
#ifdef DEBUG
                        sprintf(log_text,"%sreal recv data length:%ld\t%s\n",log_text,m+1,"recv done without uuid!");
                        log_event(log_text);
#endif
                        push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),"",gettime(),F_RECV,m+1L,len,sockfd,RECV_NORMALLY);
                        return m+1L;
                    }else{
                        memmove((char *)buf+1,after_uuid,data_len);
#ifdef DEBUG
                        sprintf(log_text,"%sreal recv data length:%ld\t%s\n",log_text,1+data_len,"read recv with multi uuid and without uuid!");
                        log_event(log_text);
#endif
                        int num=(m-data_len)/ID_LENGTH;
                        //                        char mul_id[LOG_LENGTH];
                        char *mul_id=(char *)malloc(ID_LENGTH*num+ID_LENGTH);//to be checked
                        memset(mul_id,0,ID_LENGTH*num+ID_LENGTH);
                        int t=0;
                        for(;t<num;t++){
                            strcat(mul_id,uuids[t]);
                        }
                        push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),mul_id,gettime(),F_RECV,m+1L,len,sockfd,RECV_NORMALLY_ID);
                        //                        log_message(mul_id,strlen(mul_id));
                        //                        pthread_setspecific(uuid_key,(void *)uuids[0]);
                        return 1L+data_len;
                    }
                    //for multi-write to one read
                }
                
            }
            
        }else{
#ifdef DEBUG
            sprintf(log_text,"%s%s\n",log_text,"recv done with getsocketname errors!");
            log_event(log_text);
#endif
        }
    }else if(socket_family==AF_UNIX){
#ifdef OTHER_SOCK
        sprintf(log_text,"%s%s\t",log_text,"AF_UNIX socket\t");
        
        struct sockaddr_un uin;
        struct sockaddr_un uon;
        socklen_t s_len = sizeof(uin);
        if ((getsockname(sockfd, (struct sockaddr *)&uin, &s_len) != -1) &&(getpeername(sockfd, (struct sockaddr *)&uon, &s_len) != -1)){
            sprintf(log_text,"%srecv from:%s\twith:%s\t recv done!\n",log_text,uon.sun_path,uin.sun_path);
            log_event(log_text);
        }else{
            sprintf(log_text,"%s%s\n",log_text,"recv done with getsocketname errors!");
            log_event(log_text);
        }
#endif
        push_to_database("",1,"",1,getpid(),pthread_self(),"",0,F_RECV,0,len,sockfd,DONE_UNIX);
    }else if(socket_family==UNKNOWN_FAMILY){
#ifdef OTHER_SOCK
        sprintf(log_text,"%s%s\n",log_text,"recv done but errors happend during getting socket family");
        log_event(log_text);
#endif
        push_to_database("",1,"",1,getpid(),pthread_self(),"",0,F_RECV,0,len,sockfd,DONE_OTHER);
    }else{
#ifdef OTHER_SOCK
        sprintf(log_text,"%s%s\t",log_text,"recv done but Unknown socket\n");
        log_event(log_text);
#endif
        push_to_database("",1,"",1,getpid(),pthread_self(),"",0,F_RECV,0,len,sockfd,DONE_OTHER);
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
                push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),"",gettime(),F_SEND,n,len,sockfd,SEND_ERROR);
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
                push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),"",gettime(),F_SEND,n,len,sockfd,SEND_FILTER);
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
                push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),"",gettime(),F_SEND,n,len,sockfd,SEND_NORMALLY);
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
            log_message3(target,new_len);
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
            push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),id,gettime(),F_SEND,n,len,sockfd,SEND_ID);
            if(n==new_len){
                return len;
            }else if(n<=len){
                return n;
            }else{
                return len;
            }
            
        }else{
#ifdef DEBUG
            sprintf(log_text,"%s%s\n",log_text,"send done with get socket error!");
            log_event(log_text);
#endif
        }
    }else if(socket_family==AF_UNIX){
#ifdef OTHER_SOCK
        sprintf(log_text,"%s%s\t",log_text,"AF_UNIX socket");
        struct sockaddr_un uin;
        struct sockaddr_un uon;
        socklen_t s_len = sizeof(uin);
        if ((getsockname(sockfd, (struct sockaddr *)&uin, &s_len) != -1) &&(getpeername(sockfd, (struct sockaddr *)&uon, &s_len) != -1)){
            sprintf(log_text,"%ssend to:%s\twith:%s\t send done!\n",log_text,uin.sun_path,uon.sun_path);
            log_event(log_text);
        }else{
            sprintf(log_text,"%s%s\n",log_text,"send done with getsocketname errors!");
            log_event(log_text);
        }
#endif
        push_to_database("",1,"",1,getpid(),pthread_self(),"",0,F_SEND,0,len,sockfd,DONE_UNIX);
    }else if(socket_family==UNKNOWN_FAMILY){
#ifdef OTHER_SOCK
        sprintf(log_text,"%s%s\n",log_text,"send done but errors happend during getting socket family");
        log_event(log_text);
#endif
        push_to_database("",1,"",1,getpid(),pthread_self(),"",0,F_SEND,0,len,sockfd,DONE_OTHER);
    }else{
#ifdef OTHER_SOCK
        sprintf(log_text,"%s%s\n",log_text,"send done but Unknown socket");
        log_event(log_text);
#endif
        push_to_database("",1,"",1,getpid(),pthread_self(),"",0,F_SEND,0,len,sockfd,DONE_OTHER);
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
                push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),"",gettime(),F_WRITE,n,count,fd,SEND_ERROR);
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
                push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),"",gettime(),F_WRITE,n,count,fd,SEND_FILTER);
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
                push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),"",gettime(),F_WRITE,n,count,fd,SEND_NORMALLY);
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
            log_message3(target,new_len);
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
            push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),id,gettime(),F_WRITE,n,count,fd,SEND_ID);
            if(n==new_len){
                return count;
            }else if(n<=count){
                return n;
            }else{
                return count;
            }
            
        }else{
#ifdef DEBUG
            sprintf(log_text,"%s%s\n",log_text,"write done with get socket error!");
            log_event(log_text);
#endif
        }
    }else if(socket_family==AF_UNIX){
#ifdef OTHER_SOCK
        sprintf(log_text,"%s%s\t",log_text,"AF_UNIX socket");
        struct sockaddr_un uin;
        struct sockaddr_un uon;
        socklen_t s_len = sizeof(uin);
        if ((getsockname(fd, (struct sockaddr *)&uin, &s_len) != -1) &&(getpeername(fd, (struct sockaddr *)&uon, &s_len) != -1)){
            sprintf(log_text,"%swrite to:%s\twith:%s\t write done!\n",log_text,uin.sun_path,uon.sun_path);
            log_event(log_text);
        }else{
            sprintf(log_text,"%s%s\n",log_text,"write done with getsocketname errors!");
            log_event(log_text);
        }
#endif
        push_to_database("",1,"",1,getpid(),pthread_self(),"",0,F_WRITE,0,count,fd,DONE_UNIX);
    }else if(socket_family==UNKNOWN_FAMILY){
#ifdef OTHER_SOCK
        sprintf(log_text,"%s%s\n",log_text,"write done but errors happend during getting socket family");
        log_event(log_text);
#endif
        push_to_database("",1,"",1,getpid(),pthread_self(),"",0,F_WRITE,0,count,fd,DONE_OTHER);
    }else{
#ifdef OTHER_SOCK
        sprintf(log_text,"%s%s\n",log_text,"write done but Unknown socket");
        log_event(log_text);
#endif
        push_to_database("",1,"",1,getpid(),pthread_self(),"",0,F_WRITE,0,count,fd,DONE_OTHER);
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
                push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),"",gettime(),F_READ,n,count,fd,RECV_FILTER);
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
                    push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),"",gettime(),F_READ,n,count,fd,RECV_ERROR);
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
                push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),"",gettime(),F_READ,n,count,fd,RECV_NORMALLY);
                return n;
            }else{
#ifdef DATA_INFO
                sprintf(log_text,"%sread buff length:%zu\t socket recv buff length:%d\t socket send buff length:%d\t",log_text,count,recv_size,send_size);
                sprintf(log_text,"%sread process:%u\t thread:%lu\t socketid:%d\t",log_text,getpid(),pthread_self(),fd);
#endif
                
                char result[count];
                //                char id[ID_LENGTH];
                char *id=malloc(ID_LENGTH);//memory collected by the pthread_key_create
                ssize_t n;
                long long ttime;
                ssize_t m=old_read(fd,id,1);
                
                if (m<=0){//no message left or errors happened!
#ifdef RW_ERR
                    sprintf(log_text,"%sreal read data length:%ld\terrorno:%d\t%s\t%s\n",log_text,m,errno,strerror(errno),"read done with read error!");
                    log_event(log_text);
#endif
                    int tmp_erro=errno;
                    push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),"",gettime(),F_READ,m,count,fd,RECV_ERROR);
                    errno=tmp_erro;
                    return m;
                }
                if(id[0]=='^'){
                    m=old_read(fd,&id[1],1);
                    if(m<=0){
                        memmove(buf,id,1);
                        sprintf(log_text,"%sreal read data length:%ld\t%s\t%s\n",log_text,1L,strerror(errno),"read done 2 with read error!");
                        log_event(log_text);
                        push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),"",gettime(),F_READ,1L,count,fd,RECV_ERROR);
                        return 1L;
                    }
                    if(id[1]=='^'){//get uuid start
                        m=old_read(fd,&id[2],ID_LENGTH-2);
                        if(m==(ID_LENGTH-2)){
                            if((id[ID_LENGTH-2]=='^')&&(id[ID_LENGTH-3]=='^')){
                                // get complete uuid
#ifdef DEBUG
                                sprintf(log_text,"%s%s\t",log_text,"id extracted");
#endif
                                //                                m=old_read(fd,buf,count);
                                //for multi-write to one read
                                m=old_read(fd,result,count);
                                //                                char uuids[m/ID_LENGTH][ID_LENGTH];
                                char **uuids=init_uuids(m);
                                char after_uuid[m];
                                size_t data_len=0;
                                data_len=get_uuid(uuids,result,m,after_uuid);
                                if(data_len==m){
                                    memmove(buf,result,m);
#ifdef DEBUG
                                    sprintf(log_text,"%sreal read data length:%ld\t%s\n",log_text,m,"read done!");
                                    log_event(log_text);
#endif
                                    push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),id,gettime(),F_READ,m+ID_LENGTH,count,fd,RECV_SINGLE);
                                    //                                    pthread_setspecific(uuid_key,(void *)id);
                                    return m;
                                }else{
                                    memmove(buf,after_uuid,data_len);
#ifdef DEBUG
                                    sprintf(log_text,"%sreal read data length:%zu\t%s\n",log_text,data_len,"multi uuid !read done!");
                                    log_event(log_text);
                                    
#endif
                                    int num=(m-data_len)/ID_LENGTH;
                                    //                                    char mul_id[LOG_LENGTH];
                                    char *mul_id=(char *)malloc(ID_LENGTH*num+ID_LENGTH);
                                    strcat(mul_id,id);
                                    int t=0;
                                    for(;t<num;t++){
                                        strcat(mul_id,uuids[t]);
                                    }
                                    push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),mul_id,gettime(),F_READ,data_len+ID_LENGTH,count,fd,RECV_MULTI_BRO);
                                    //                                    pthread_setspecific(uuid_key,(void *)uuids[0]);
                                    return data_len;
                                }
                                //for multi-write to one read
                                
                            }else{
                                
                                //for multi-write to one read
                                memmove(buf,id,ID_LENGTH);
                                m=old_read(fd,result,count-ID_LENGTH);
                                //                                char uuids[m/ID_LENGTH][ID_LENGTH];
                                char **uuids=init_uuids(m);
                                char after_uuid[m];
                                size_t data_len=0;
                                data_len=get_uuid(uuids,result,m,after_uuid);
                                if(data_len==m){
                                    memmove((char *)buf+ID_LENGTH,result,m);
#ifdef DEBUG
                                    sprintf(log_text,"%sreal read data length:%ld\t%s\n",log_text,m+ID_LENGTH,"read done with similar uuid!");
                                    log_event(log_text);
#endif
                                    push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),"",gettime(),F_READ,m+ID_LENGTH,count,fd,RECV_SIM1);
                                    return m+ID_LENGTH;
                                }else{
                                    memmove((char *)buf+ID_LENGTH,after_uuid,data_len);
#ifdef DEBUG
                                    sprintf(log_text,"%sreal read data length:%zu\t%s\n",log_text,data_len+ID_LENGTH,"read done with multi and similar uuid!");
                                    log_event(log_text);
#endif
                                    
                                    int num=(m-data_len)/ID_LENGTH;
                                    //                                    char mul_id[LOG_LENGTH];
                                    char *mul_id=(char *)malloc(ID_LENGTH*num+ID_LENGTH);
                                    int t=0;
                                    for(;t<num;t++){
                                        strcat(mul_id,uuids[t]);
                                    }
                                    push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),mul_id,gettime(),F_READ,data_len+ID_LENGTH,count,fd,RECV_SIM1_ID);
                                    //                                    pthread_setspecific(uuid_key,(void *)uuids[0]);
                                    return data_len+ID_LENGTH;
                                }
                                //for multi-write to one read
                                
                            }
                        }else{
#ifdef DEBUG
                            sprintf(log_text,"%sreal read data length:%ld\t%s\n",log_text,m+2,"read done with 2 starting symbols!");
                            log_event(log_text);
#endif
                            push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),"",gettime(),F_RECV,m+2L,count,fd,RECV_SIM2);
                            
                            //not a uuid
                            if(count==(m+2)){
                                memmove(buf,id,count);
                                
                                return count;
                            }else{
                                //not a uuid or network error and will result in application error
                                memmove(buf,id,m+2);
                                return m+2;
                            }
                        }
                    }else{//not a uuid
                        
                        memmove(buf,id,2);
                        
                        //for multi-write to one read
                        m=old_read(fd,result,count-2);
                        //                        char uuids[m/ID_LENGTH][ID_LENGTH];
                        char **uuids=init_uuids(m);
                        char after_uuid[m];
                        size_t data_len=0;
                        data_len=get_uuid(uuids,result,m,after_uuid);
                        if(data_len==m){
                            memmove((char *)buf+2,result,m);
#ifdef DEBUG
                            sprintf(log_text,"%sreal read data length:%ld\t%s\n",log_text,m+2,"read done with 1 starting symbols!");
                            log_event(log_text);
#endif
                            push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),"",gettime(),F_READ,m+2L,count,fd,RECV_SIM3);
                            return m+2L;
                        }else{
                            memmove((char *)buf+2,after_uuid,data_len);
#ifdef DEBUG
                            sprintf(log_text,"%sreal read data length:%ld\t%s\n",log_text,2+data_len,"read done with 1 starting symbols and multi uuid!");
                            log_event(log_text);
                            
#endif
                            
                            int num=(m-data_len)/ID_LENGTH;
                            //                            char mul_id[LOG_LENGTH];
                            char *mul_id=(char *)malloc(ID_LENGTH*num+ID_LENGTH);
                            int t=0;
                            for(;t<num;t++){
                                strcat(mul_id,uuids[t]);
                            }
                            push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),mul_id,gettime(),F_READ,2L+data_len,count,fd,RECV_SIM3_ID);
                            //                            pthread_setspecific(uuid_key,(void *)uuids[0]);
                            return 2L+data_len;
                        }
                        //for multi-write to one read
                        
                    }
                    
                }else{
                    
                    memmove(buf,id,1);
                    
                    //for multi-write to one read
                    m=old_read(fd,result,count-1);
                    //                    char uuids[m/ID_LENGTH][ID_LENGTH];
                    char **uuids=init_uuids(m);
                    char after_uuid[m];
                    size_t data_len=0;
                    data_len=get_uuid(uuids,result,m,after_uuid);
                    if(data_len==m){
                        memmove((char *)buf+1,result,m);
#ifdef DEBUG
                        sprintf(log_text,"%sreal read data length:%ld\t%s\n",log_text,m+1,"read done without uuid!");
                        log_event(log_text);
#endif
                        push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),"",gettime(),F_READ,m+1L,count,fd,RECV_NORMALLY);
                        return m+1L;
                    }else{
                        memmove((char *)buf+1,after_uuid,data_len);
#ifdef DEBUG
                        sprintf(log_text,"%sreal read data length:%ld\t%s\n",log_text,1+data_len,"read done with multi uuid and without uuid!");
                        log_event(log_text);
#endif
                        int num=(m-data_len)/ID_LENGTH;
                        //                        char mul_id[LOG_LENGTH];
                        char *mul_id=(char *)malloc(ID_LENGTH*num+ID_LENGTH);//to be checked
                        memset(mul_id,0,ID_LENGTH*num+ID_LENGTH);
                        int t=0;
                        for(;t<num;t++){
                            strcat(mul_id,uuids[t]);
                        }
                        push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),mul_id,gettime(),F_READ,m+1L,count,fd,RECV_NORMALLY_ID);
                        //                        pthread_setspecific(uuid_key,(void *)uuids[0]);
                        //                        log_message(mul_id,strlen(mul_id));
                        //                        memset(mul_id,0,LOG_LENGTH);
                        return 1L+data_len;
                    }
                    //for multi-write to one read
                    
                }
                
            }
            
        }else{
#ifdef DEBUG
            sprintf(log_text,"%s%s\n",log_text,"read donw with get socket error!");
            log_event(log_text);
#endif
        }
        
    }else if(socket_family==AF_UNIX){
#ifdef OTHER_SOCK
        sprintf(log_text,"%s%s\t",log_text,"AF_UNIX socket");
        struct sockaddr_un uin;
        struct sockaddr_un uon;
        socklen_t s_len = sizeof(uin);
        if ((getsockname(fd, (struct sockaddr *)&uin, &s_len) != -1) &&(getpeername(fd, (struct sockaddr *)&uon, &s_len) != -1)){
            sprintf(log_text,"%sread from:%s\twith:%s\t read done!\n",log_text,uon.sun_path,uin.sun_path);
            log_event(log_text);
        }else{
            sprintf(log_text,"%s%s\n",log_text,"read done with getsocketname errors!");
            log_event(log_text);
        }
#endif
        push_to_database("",1,"",1,getpid(),pthread_self(),"",gettime(),F_READ,0,count,fd,DONE_UNIX);
    }else if(socket_family==UNKNOWN_FAMILY){
#ifdef OTHER_SOCK
        sprintf(log_text,"%s%s\n",log_text,"read done but errors happend during getting socket family");
        log_event(log_text);
#endif
        push_to_database("",1,"",1,getpid(),pthread_self(),"",gettime(),F_READ,0,count,fd,DONE_OTHER);
    }else{
#ifdef OTHER_SOCK
        sprintf(log_text,"%s%s\n",log_text,"read done but Unknown socket");
        log_event(log_text);
#endif
        push_to_database("",1,"",1,getpid(),pthread_self(),"",gettime(),F_READ,0,count,fd,DONE_OTHER);
    }
    
    
    return old_read(fd, buf, count);
}


ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags){
    
    static void *handle = NULL;
    static RECVMSG old_recvmsg = NULL;
    
    if( !handle )
    {
        handle = dlopen("libc.so.6", RTLD_LAZY);
        old_recvmsg = (RECVMSG)dlsym(handle, "recvmsg");
    }
    if(!uuid_key){
        pthread_key_create(&uuid_key,NULL);
    }
    
#ifdef DEBUG
    char log_text[LOG_LENGTH]="init";
    sprintf(log_text,"%s\t","in recvmsg");
#endif
    
    //check socket type first
    int sockType = 0;
    int opt = 4;
    getsockopt(sockfd, SOL_SOCKET, SO_TYPE, (char*)&sockType, &opt);
    sa_family_t socket_family = -1;
    if(sockType == 1 ){
        //TCP
        socket_family = get_socket_family(sockfd);
    }else if(sockType == 2){
        //UDP
        socket_family = ((struct sockaddr_in *)msg->msg_name)->sin_family;
    }
    
    if (socket_family==AF_INET){
#ifdef DEBUG
        sprintf(log_text,"%s%s\t",log_text,"AF_INET socket");
#endif
        struct sockaddr_in sin; //local socket info
        struct sockaddr_in son; //remote socket into
        socklen_t s_len = sizeof(sin);
        unsigned short int in_port;
        unsigned short int on_port;
        char in_ip[256];
        char on_ip[256];
        char *tmp_in_ip;
        char *tmp_on_ip;
        if(sockType ==1){
            //TCP
            getsockname(sockfd, (struct sockaddr *)&sin, &s_len);
            getpeername(sockfd, (struct sockaddr *)&son, &s_len);
            in_port=ntohs(sin.sin_port);
            on_port=ntohs(son.sin_port);
            tmp_in_ip=inet_ntoa(sin.sin_addr);
            memmove(in_ip,tmp_in_ip,strlen(tmp_in_ip)+1);
            tmp_on_ip=inet_ntoa(son.sin_addr);
            memmove(on_ip,tmp_on_ip,strlen(tmp_on_ip)+1);
        }else if(sockType ==2){
            //UDP
            getsockname(sockfd, (struct sockaddr *)&sin, &s_len);
            in_port=ntohs(sin.sin_port);
            on_port=ntohs(((struct sockaddr_in *)msg->msg_name)->sin_port);
            tmp_in_ip=inet_ntoa(sin.sin_addr);
            memmove(in_ip,tmp_in_ip,strlen(tmp_in_ip)+1);
            tmp_on_ip=inet_ntoa(((struct sockaddr_in *)msg->msg_name)->sin_addr);
            memmove(on_ip,tmp_on_ip,strlen(tmp_on_ip)+1);
        }
        
        
#ifdef IP_PORT
        sprintf(log_text,"%srecvmsg from %s:%d with %s:%d\t",log_text,on_ip,on_port,in_ip,in_port);
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
        
        ssize_t len=0;
        if(check_filter(on_ip,in_ip,on_port,in_port)==0){
#ifdef FILTER
            sprintf(log_text,"%s%s\n",log_text,"recvmsg done with filter!");
            log_event(log_text);
#endif
            len = old_recvmsg(sockfd, msg, flags);
            if((on_port==80)||(in_port==80)){
                return len;
            }
            int tmp_erro=errno;
            push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),"",gettime(),F_RECVFROM,len,msg->msg_iov->iov_len,sockfd,RECV_FILTER);
            if(len<=0){
                errno=tmp_erro;
            }
            
            return len;
        }
        
        if(with_uuid==0){//no uuid, write this message
            len = old_recvmsg(sockfd, msg, flags);
#ifdef DATA_INFO
            sprintf(log_text,"%srecvmsg data length:%ld\t supposed read data length:%ld\t socket recv buff length:%d\t socket send buff length:%d\t",log_text,len,len-39,recv_size,send_size);
            sprintf(log_text,"%srecvmsg process:%u\t thread:%lu\t socketid:%d\t",log_text,getpid(),pthread_self(),sockfd);
#endif
            if(len<=0){
#ifdef DEBUG
                sprintf(log_text,"%s%s\t%s\n",log_text,"recvmsg error happened!","recvmsg done with read error!");
                log_event(log_text);
#endif
                int tmp_erro=errno;
                push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),"",gettime(),F_RECVMSG,len,msg->msg_iov->iov_len,sockfd,RECV_ERROR);
                errno=tmp_erro;
                return len;
            }
#ifdef MESSAGE
            log_message(msg->msg_iov->iov_base,len);
#endif
#ifdef DEBUG
            sprintf(log_text,"%s%s\n",log_text,"recvmsg done without uuid!");
            log_event(log_text);
#endif
            push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),"",gettime(),F_RECVMSG,len,msg->msg_iov->iov_len,sockfd,RECV_NORMALLY);
            return len;
        }else{
#ifdef DATA_INFO
            sprintf(log_text,"%srecvmsg data length:%ld\t supposed read data length:%ld\t socket recv buff length:%d\t socket send buff length:%d\t",log_text,len,len-39,recv_size,send_size);
            sprintf(log_text,"%srecvmsg process:%u\t thread:%lu\t socketid:%d\t",log_text,getpid(),pthread_self(),sockfd);
#endif
            //            char id[ID_LENGTH];
            char *id=malloc(ID_LENGTH);//memory collected by the pthread_key_create
            int if_id;
            long long ttime;
            ssize_t len=old_recvmsg(sockfd, msg, flags);
            char *result=malloc(len);
            memcpy(result,(char *)msg->msg_iov->iov_base,len);
            
            if (len<=0){
#ifdef RW_ERR
                sprintf(log_text,"%sreal recv data length:%ld\terrorno:%d\t%s\t%s\n",log_text,len,errno,strerror(errno),"recv done with read error!");
                log_event(log_text);
#endif
                int tmp_erro=errno;
                push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),"",gettime(),F_RECVFROM,len,msg->msg_iov->iov_len,sockfd,RECV_ERROR);
                errno=tmp_erro;
                return len;
            }
            
            if((result[ID_LENGTH-1]=='^')&&(result[ID_LENGTH-2]=='^')){
#ifdef DEBUG
                sprintf(log_text,"%s%s\t",log_text,"id extracted");
#endif
                //                char uuids[len/ID_LENGTH][ID_LENGTH];
                char **uuids=init_uuids(len);
                char after_uuid[len];
                int data_len=0;
                data_len=get_uuid(uuids,result,len,after_uuid);
                if(data_len!=len){
                    
                    ttime=gettime();
                    
                    //syscall(SYS_gettid) also can be use to get thread id
                    push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),"",gettime(),F_RECVMSG,len,msg->msg_iov->iov_len,sockfd,RECV_FILTER);
                    
                    memcpy((char *)msg->msg_iov->iov_base,after_uuid,data_len);
                    msg->msg_iov->iov_len = data_len;
#ifdef DEBUG
                    printf("logtxt:%s\n",log_text);
                    sprintf(log_text,"%s%s\n",log_text,"recvmsg done");
                    log_event(log_text);
#endif
                    free(result);
                    return data_len;
                }else{
#ifdef DEBUG
                    sprintf(log_text,"%s%s\t%s\n",log_text,"id extracting failed\t","recvmsg done!");
                    log_event(log_text);
#endif
                    free(result);
                    push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),id,gettime(),F_RECVMSG,len,msg->msg_iov->iov_len,sockfd,RECV_NORMALLY);
                    return len;
                }
                
            }
            
        }
        
    }else if(socket_family==AF_UNIX){
        
#ifdef OTHER_SOCK
        sprintf(log_text,"%s%s\t",log_text,"AF_UNIX socket");
        struct sockaddr_un uin;
        struct sockaddr_un uon;
        socklen_t s_len = sizeof(uin);
        
        if(sockType == 1)
        {
            // TCP
            getsockname(socket, (struct sockaddr *)&uin, &s_len);
            getpeername(socket, (struct sockaddr *)&uon, &s_len);
            sprintf(log_text,"%srecvmsg from:%s\twith:%s\t recvmsg done!\n",log_text,uon.sun_path,uin.sun_path);
            log_event(log_text);
        }
        else if(sockType == 2)
        {
            // UDP
            getsockname(socket, (struct sockaddr *)&sin, &s_len);
            on_port=ntohs(((struct sockaddr_in *)addr)->sin_port);
            sprintf(log_text,"%srecvmsg from:%s\twith:%s\t recvmsg done!\n",log_text,uon.sun_path,uin.sun_path);
            log_event(log_text);
        }
        else
        {
            sprintf(log_text,"%srecvmsg from:%s\twith:%s\t recvmsg done!\n",log_text,uon.sun_path,uin.sun_path);
            log_event(log_text);
        }
#endif
        push_to_database("",0,"",0,getpid(),pthread_self(),"",gettime(),F_RECVMSG,0,msg->msg_iov->iov_len,sockfd,DONE_UNIX);
        
    }else if(socket_family==UNKNOWN_FAMILY){
        
#ifdef OTHER_SOCK
        sprintf(log_text,"%s%s\n",log_text,"recvmsg done but errors happend during getting socket family");
        log_event(log_text);
#endif
        push_to_database("",0,"",0,getpid(),pthread_self(),"",gettime(),F_RECVMSG,0,msg->msg_iov->iov_len,sockfd,DONE_OTHER);
        
    }else{
        
#ifdef OTHER_SOCK
        sprintf(log_text,"%s%s\n",log_text,"recvmsg done but Unknown socket");
        log_event(log_text);
#endif
        push_to_database("",0,"",0,getpid(),pthread_self(),"",gettime(),F_RECVMSG,0,msg->msg_iov->iov_len,sockfd,DONE_OTHER);
    }
    
    return old_recvmsg(sockfd, msg, flags);
}

ssize_t sendmsg(int sockfd, const struct msghdr *msg, int flags){
    
    static void *handle = NULL;
    static SENDMSG old_sendmsg = NULL;
    
    if( !handle )
    {
        handle = dlopen("libc.so.6", RTLD_LAZY);
        old_sendmsg = (SENDMSG)dlsym(handle, "sendmsg");
    }
    if(!uuid_key){
        pthread_key_create(&uuid_key,NULL);
    }
    
#ifdef DEBUG
    char log_text[LOG_LENGTH]="init";
    sprintf(log_text,"%s\t","in sendmsg");
#endif
    
    int sockType = 0;
    int opt = 4;
    getsockopt(sockfd, SOL_SOCKET, SO_TYPE, (char*)&sockType, &opt);
    sa_family_t socket_family = -1;
    
    if(sockType == 1){
        // TCP
        socket_family = get_socket_family(sockfd);
    }else if(sockType == 2){
        // UDP
        socket_family = ((struct sockaddr_in *)msg->msg_name)->sin_family;
    }
    
    if (socket_family==AF_INET){
#ifdef DEBUG
        sprintf(log_text,"%s%s\t",log_text,"AF_INET socket");
#endif
        struct sockaddr_in sin;
        struct sockaddr_in son;
        socklen_t s_len = sizeof(son);
        unsigned short int in_port;
        unsigned short int on_port;
        char in_ip[256];
        char on_ip[256];
        char *tmp_in_ip;
        char *tmp_on_ip;
        
        if(sockType == 1){
            //TCP
            if  ((getsockname(sockfd, (struct sockaddr *)&sin, &s_len) != -1) &&(getpeername(sockfd, (struct sockaddr *)&son, &s_len) != -1)){
                in_port=ntohs(sin.sin_port);
                on_port=ntohs(son.sin_port);
                tmp_in_ip=inet_ntoa(sin.sin_addr);
                memmove(in_ip,tmp_in_ip,strlen(tmp_in_ip)+1);
                tmp_on_ip=inet_ntoa(son.sin_addr);
            }
            memmove(on_ip,tmp_on_ip,strlen(tmp_on_ip)+1);
            
        }else if(sockType == 2){
            //UDP
            getsockname(sockfd, (struct sockaddr *)&sin, &s_len);
            in_port=ntohs(sin.sin_port);
            on_port=ntohs(((struct sockaddr_in *)msg->msg_name)->sin_port);
            tmp_in_ip=inet_ntoa(sin.sin_addr);
            memmove(in_ip,tmp_in_ip,strlen(tmp_in_ip)+1);
            tmp_on_ip=inet_ntoa(((struct sockaddr_in *)msg->msg_name)->sin_addr);
            memmove(on_ip,tmp_on_ip,strlen(tmp_on_ip)+1);
        }
        
#ifdef IP_PORT
        sprintf(log_text,"%ssendmsg to %s:%d with %s:%d\t",log_text,on_ip,on_port,in_ip,in_port);
#endif
#ifdef DATA_INFO
        int recv_size = 0;
        int send_size = 0;
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
        
        size_t len = msg->msg_iov->iov_len;
        size_t new_len = ID_LENGTH + len;
        ssize_t n=0;
        if((SIZE_MAX - len) < MAX_SPACE){
#ifdef DEBUG
            sprintf(log_text,"%s%s\t%s\n",log_text,"too large buf size, cannot add uuid to it","write done!");
            log_event(log_text);
#endif
            n=old_sendmsg(sockfd, msg, flags);
            int tmp_erro=errno;
            push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),"",gettime(),F_SENDTO,n,msg->msg_iov->iov_len,sockfd,SEND_ERROR);
            if(n<=0){
                errno=tmp_erro;
            }
            return n;
        }
        
        if(with_uuid==0){//no uuid, write this message
#ifdef DATA_INFO
            sprintf(log_text,"%ssend data length:%zu\t supposed send data length:%zu\t socket recv buff length:%d\t socket send buff length:%d\t",log_text,new_len,len,recv_size,send_size);
            sprintf(log_text,"%ssendmsg process:%u\t thread:%lu\t socketid:%d\t",log_text,getpid(),pthread_self(),sockfd);
#endif
            
#ifdef MESSAGE
            log_message((char *)msg->msg_iov->iov_base,msg->msg_iov->iov_len);
#endif
            
#ifdef DEBUG
            sprintf(log_text,"%s%s\n",log_text,"sendmsg done without uuid!");
            log_event(log_text);
#endif
            n=old_sendmsg(sockfd, msg, flags);
            push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),"",gettime(),F_SENDMSG,n,msg->msg_iov->iov_len,sockfd,SEND_NORMALLY);
            return n;
        }
        
        
        if(check_filter(on_ip,in_ip,on_port,in_port)==0){
#ifdef FILTER
            sprintf(log_text,"%s%s\n",log_text,"sendmsg done with filter!");
            log_event(log_text);
#endif
            n = old_sendmsg(sockfd, msg, flags);
            if((on_port==80)||(in_port==80)){
                return n;
            }
            int tmp_erro=errno;
            push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),"",gettime(),F_SENDTO,n,msg->msg_iov->iov_len,sockfd,SEND_FILTER);
            if(n<=0){
                errno=tmp_erro;
            }
            return n;
        }
        
        //add uuid  at the first of the msg
        
        char *uuid_msg=malloc(new_len);
        //        char id[ID_LENGTH];
        char *id=malloc(ID_LENGTH);//memory collected by the pthread_key_create
        random_uuid(id);
        int i;
        for (i=0;i<ID_LENGTH;i++){
            uuid_msg[i]=id[i];
        }
        memcpy(&uuid_msg[ID_LENGTH],(char *)msg->msg_iov->iov_base,len);
        
        msg->msg_iov->iov_base = uuid_msg;
        msg->msg_iov->iov_len = new_len;
        
        long long ttime;
        ttime=gettime();
        n=old_sendmsg(sockfd,msg,flags);
        
#ifdef DATA_INFO
        sprintf(log_text,"%sreal send length:%ld\t send data length:%zu\t supposed send data length:%zu\t socket recv buff length:%d\t socket send buff length:%d\t",log_text,n,new_len,len,recv_size,send_size);
        sprintf(log_text,"%ssendmsg process:%u\t thread:%lu\t socketid:%d\t",log_text,getpid(),pthread_self(),sockfd);
#endif
#ifdef MESSAGE
        log_message((char *)msg->msg_iov->iov_base,msg->msg_iov->iov_len);
#endif
        
#ifdef DEBUG
        sprintf(log_text,"%s%s\n",log_text,"sendmsg done!");
        log_event(log_text);
#endif
        push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),id,gettime(),F_SENDMSG,n,msg->msg_iov->iov_len,sockfd,SEND_ID);
        
        if(n == new_len){
            return len;
        }else if(n <= len){
            return n;
        }else{
            return len;
        }
        
    }else if(socket_family==AF_UNIX){
#ifdef OTHER_SOCK
        sprintf(log_text,"%s%s\t",log_text,"AF_UNIX socket");
        struct sockaddr_un uin;
        struct sockaddr_un uon;
        socklen_t s_len = sizeof(uin);
        if(sockType == 1){
            //TCP
            getsockname(socket, (struct sockaddr *)&uin, &s_len);
            getpeername(socket, (struct sockaddr *)&uon, &s_len);
            sprintf(log_text,"%ssendmsg to:%s\twith:%s\t sendmsg done!\n",log_text,uin.sun_path,uon.sun_path);
            log_event(log_text);
        }else if(sockType == 2){
            // UDP
            getsockname(socket, (struct sockaddr *)&sin, &s_len);
            on_port=ntohs(((struct sockaddr_in *)addr)->sin_port);
            sprintf(log_text,"%ssendmsg to:%s\twith:%s\t sendmsg done!\n",log_text,uin.sun_path,uon.sun_path);
            log_event(log_text);
        }else{
            sprintf(log_text,"%s%s\n",log_text,"sendmsg done with getsocketname errors!");
            log_event(log_text);
        }
#endif
        push_to_database("",0,"",0,getpid(),pthread_self(),"",gettime(),F_SENDMSG,0,msg->msg_iov->iov_len,sockfd,DONE_UNIX);
    }else if(socket_family==UNKNOWN_FAMILY){
#ifdef OTHER_SOCK
        sprintf(log_text,"%s%s\n",log_text,"sendmsg done but errors happend during getting socket family");
        log_event(log_text);
#endif
        push_to_database("",0,"",0,getpid(),pthread_self(),"",gettime(),F_SENDMSG,0,msg->msg_iov->iov_len,sockfd,DONE_OTHER);
    }else{
#ifdef OTHER_SOCK
        sprintf(log_text,"%s%s\n",log_text,"sendmsg done but Unknown socket");
        log_event(log_text);
#endif
        push_to_database("",0,"",0,getpid(),pthread_self(),"",gettime(),F_SENDMSG,0,msg->msg_iov->iov_len,sockfd,DONE_OTHER);
    }
    
    return old_sendmsg(sockfd, msg, flags);
}

ssize_t sendto(int socket, const void* buf, size_t len, int flags, const struct sockaddr* addr, socklen_t addrlen)
{
    static void* handle = NULL;
    static SENDTO old_sendto = NULL;
    if(!handle)
    {
        handle = dlopen("libc.so.6", RTLD_LAZY);
        old_sendto = (SENDTO)dlsym(handle, "sendto");
    }
    if(!uuid_key){
        pthread_key_create(&uuid_key,NULL);
    }
#ifdef DEBUG
    char log_text[LOG_LENGTH]="init";
    sprintf(log_text,"%s%s\t",log_text,"in sendto");
#endif
    
    int sockType = 0;
    int opt = 4;
    getsockopt(socket, SOL_SOCKET, SO_TYPE, (char*)&sockType, &opt);
    int socket_family = -1;
    if(sockType == 1)
    {
        // TCP
        socket_family = get_socket_family(socket);
    }
    else if(sockType == 2)
    {
        // UDP
        socket_family = ((struct sockaddr_in *)addr)->sin_family;
    }
    
    
    if (socket_family == AF_INET)
    {
#ifdef DEBUG
        sprintf(log_text,"%s%s\t",log_text,"AF_INET socket");
#endif
        struct sockaddr_in sin;
        struct sockaddr_in son;
        socklen_t s_len = sizeof(son);
        unsigned short int in_port;
        unsigned short int on_port;
        char in_ip[256];
        char on_ip[256];
        char *tmp_in_ip;
        char *tmp_on_ip;
        
        if(sockType == 1)
        {
            // TCP
            getsockname(socket, (struct sockaddr *)&sin, &s_len);
            getpeername(socket, (struct sockaddr *)&son, &s_len);
            in_port=ntohs(sin.sin_port);
            on_port=ntohs(son.sin_port);
            tmp_in_ip=inet_ntoa(sin.sin_addr);
            memmove(in_ip,tmp_in_ip,strlen(tmp_in_ip)+1);
            tmp_on_ip=inet_ntoa(son.sin_addr);
            memmove(on_ip,tmp_on_ip,strlen(tmp_on_ip)+1);
        }
        else if(sockType == 2)
        {
            // UDP
            getsockname(socket, (struct sockaddr *)&sin, &s_len);
            in_port=ntohs(sin.sin_port);
            on_port=ntohs(((struct sockaddr_in *)addr)->sin_port);
            tmp_in_ip=inet_ntoa(sin.sin_addr);
            memmove(in_ip,tmp_in_ip,strlen(tmp_in_ip)+1);
            tmp_on_ip=inet_ntoa(((struct sockaddr_in *)addr)->sin_addr);
            memmove(on_ip,tmp_on_ip,strlen(tmp_on_ip)+1);
        }
        
#ifdef IP_PORT
        sprintf(log_text,"%ssend to %s:%d with %s:%d\t",log_text,on_ip,on_port,in_ip,in_port);
#endif
        
#ifdef DATA_INFO
        //get socket buff size
        int recv_size = 0;    /* socket接收缓冲区大小 */
        int send_size=0;
        socklen_t optlen;
        optlen = sizeof(send_size);
        if(getsockopt(socket, SOL_SOCKET, SO_SNDBUF,&send_size, &optlen)!=0){
            sprintf(log_text,"%s%s\t",log_text,"get socket send buff failed!");
        }
        optlen = sizeof(recv_size);
        if(getsockopt(socket, SOL_SOCKET, SO_RCVBUF, &recv_size, &optlen)!=0){
            sprintf(log_text,"%s%s\t",log_text,"get socket recv buff failed!");
        }
#endif
        
        size_t new_len=len+ID_LENGTH;
        ssize_t n=0;
        
        if((SIZE_MAX - len) < MAX_SPACE){
#ifdef DEBUG
            sprintf(log_text,"%s%s\t%s\n",log_text,"too large buf size, cannot add uuid to it","write done!");
            log_event(log_text);
#endif
            n=old_sendto(socket, buf, len, flags, addr, addrlen);
            int tmp_erro=errno;
            push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),"",gettime(),F_SENDTO,n,len,socket,SEND_ERROR);
            if(n<=0){
                errno=tmp_erro;
            }
            return n;
        }
        
        if(with_uuid==0)
        {
#ifdef DATA_INFO
            sprintf(log_text,"%ssend data length:%zu\t supposed send data length:%zu\t socket recv buff length:%d\t socket send buff length:%d\t",log_text,new_len,len,recv_size,send_size);
            sprintf(log_text,"%ssend process:%u\t thread:%lu\t socketid:%d\t",log_text,getpid(),pthread_self(),socket);
#endif
        }
        if(check_filter(on_ip,in_ip,on_port,in_port)==0)
        {
#ifdef FILTER
            sprintf(log_text,"%s%s\n",log_text,"send done with filter");
            log_event(log_text);
#endif
            n=old_sendto(socket, buf, len, flags, addr, addrlen);
            if((on_port==80)||(in_port==80)){
                return n;
            }
            int tmp_erro=errno;
            push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),"",gettime(),F_SENDTO,n,len,socket,SEND_FILTER);
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
            n=old_sendto(socket, buf, len, flags, addr, addrlen);
            int tmp_erro=errno;
            push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),"",gettime(),F_SENDTO,n,len,socket,SEND_NORMALLY);
            if(n<=0){
                errno=tmp_erro;
            }
            return n;
        }
        
        // 正式开始
        char target[new_len];
        //        char id[ID_LENGTH];
        char *id=malloc(ID_LENGTH);//memory collected by the pthread_key_create
        random_uuid(id);
        int i;
        for (i=0;i<ID_LENGTH;i++){
            target[i]=id[i];
        }
        memmove(&target[ID_LENGTH], buf, len);
        
        
        long long ttime;
        //        ssize_t n;
        ttime=gettime();
        
        n = old_sendto(socket, target, new_len, flags, addr, addrlen);
        
        
#ifdef DATA_INFO
        sprintf(log_text,"%sreal send length:%ld\t send data length:%zu\t supposed send data length:%zu\t socket recv buff length:%d\t socket send buff length:%d\t",log_text,n,new_len,len,recv_size,send_size);
        sprintf(log_text,"%ssend process:%u\t thread:%lu\t socketid:%d\t",log_text,getpid(),pthread_self(), socket);
#endif
#ifdef MESSAGE
        log_message(target,new_len);
#endif
#ifdef DEBUG
        sprintf(log_text,"%s%s\n",log_text,"send done!");
        log_event(log_text);
#endif
        push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),id,gettime(),F_SENDTO,n,len,socket,SEND_ID);
        if(n == new_len)
        {
            return len;
        }
        else if(n<=len)
        {
            return n;
        }
        else
        {
            return len;
        }
        
    }
    else if(socket_family == AF_UNIX)
    {
#ifdef OTHER_SOCK
        sprintf(log_text,"%s%s\t",log_text,"AF_UNIX socket");
        struct sockaddr_un uin;
        struct sockaddr_un uon;
        socklen_t s_len = sizeof(uin);
        
        if(sockType == 1)
        {
            // TCP
            getsockname(socket, (struct sockaddr *)&uin, &s_len);
            getpeername(socket, (struct sockaddr *)&uon, &s_len);
            sprintf(log_text,"%ssend to:%s\twith:%s\t send done!\n",log_text,uin.sun_path,uon.sun_path);
            log_event(log_text);
        }
        else if(sockType == 2)
        {
            // UDP
            getsockname(socket, (struct sockaddr *)&sin, &s_len);
            on_port=ntohs(((struct sockaddr_in *)addr)->sin_port);
            sprintf(log_text,"%ssend to:%s\twith:%s\t send done!\n",log_text,uin.sun_path,((struct sockaddr_in *)addr)->sun_path);
            log_event(log_text);
        }
        else
        {
            sprintf(log_text,"%s%s\n",log_text,"sendto done with getsocketname errors!");
            log_event(log_text);
        }
#endif
        push_to_database("",1,"",1,getpid(),pthread_self(),"",gettime(),F_SENDTO,0,len,socket,DONE_UNIX);
    }
    else if(socket_family == UNKNOWN_FAMILY)
    {
#ifdef OTHER_SOCK
        sprintf(log_text,"%s%s\n",log_text,"sendto done but errors happend during getting socket family");
        log_event(log_text);
#endif
        push_to_database("",1,"",1,getpid(),pthread_self(),"",gettime(),F_SENDTO,0,len,socket,DONE_OTHER);
    }
    else
    {
#ifdef OTHER_SOCK
        sprintf(log_text,"%s%s\n",log_text,"sendto done but Unknown socket");
        log_event(log_text);
#endif
        push_to_database("",1,"",1,getpid(),pthread_self(),"",gettime(),F_SENDTO,0,len,socket,DONE_OTHER);
    }
    return old_sendto(socket, buf, len, flags, addr, addrlen);
    
}

ssize_t recvfrom(int socket, void* buf, size_t len, int flags, struct sockaddr* addr, socklen_t* addrlen)
{
    static void* handle = NULL;
    static RECVFROM old_recvfrom = NULL;
    
    if(!handle)
    {
        handle = dlopen("libc.so.6", RTLD_LAZY);
        old_recvfrom = (RECVFROM)dlsym(handle, "recvfrom");
    }
    if(!uuid_key){
        pthread_key_create(&uuid_key,NULL);
    }
    
#ifdef DEBUG
    char log_text[LOG_LENGTH]="init";
    sprintf(log_text,"%s\t","in recvfrom");
#endif
    
    int sockType = 0;
    int opt = 4;
    getsockopt(socket, SOL_SOCKET, SO_TYPE, (char*)&sockType, &opt);
    int socket_family = -1;
    if(sockType == 1)
    {
        // TCP
        socket_family = get_socket_family(socket);
    }
    else if(sockType == 2)
    {
        // UDP
        socket_family = ((struct sockaddr_in *)addr)->sin_family;
    }
    
    if(socket_family == AF_INET)
    {
#ifdef DEBUG
        sprintf(log_text,"%s%s\t",log_text,"AF_INET socket");
#endif
        struct sockaddr_in sin;
        struct sockaddr_in son;
        socklen_t s_len = sizeof(son);
        unsigned short int in_port;
        unsigned short int on_port;
        char in_ip[256];
        char on_ip[256];
        char *tmp_in_ip;
        char *tmp_on_ip;
        
        if(sockType == 1)
        {
            // TCP
            getsockname(socket, (struct sockaddr *)&sin, &s_len);
            getpeername(socket, (struct sockaddr *)&son, &s_len);
            in_port=ntohs(sin.sin_port);
            on_port=ntohs(son.sin_port);
            tmp_in_ip=inet_ntoa(sin.sin_addr);
            memmove(in_ip,tmp_in_ip,strlen(tmp_in_ip)+1);
            tmp_on_ip=inet_ntoa(son.sin_addr);
            memmove(on_ip,tmp_on_ip,strlen(tmp_on_ip)+1);
        }
        else if(sockType == 2)
        {
            // UDP
            getsockname(socket, (struct sockaddr *)&sin, &s_len);
            in_port=ntohs(sin.sin_port);
            on_port=ntohs(((struct sockaddr_in *)addr)->sin_port);
            tmp_in_ip=inet_ntoa(sin.sin_addr);
            memmove(in_ip,tmp_in_ip,strlen(tmp_in_ip)+1);
            tmp_on_ip=inet_ntoa(((struct sockaddr_in *)addr)->sin_addr);
            memmove(on_ip,tmp_on_ip,strlen(tmp_on_ip)+1);
        }
        
#ifdef IP_PORT
        sprintf(log_text,"%srecv from %s:%d with %s:%d\t",log_text,on_ip,on_port,in_ip,in_port);
#endif
        
#ifdef DATA_INFO
        int recv_size = 0;
        int send_size = 0;
        socklen_t optlen;
        optlen = sizeof(send_size);
        if(getsockopt(socket, SOL_SOCKET, SO_SNDBUF,&send_size, &optlen)!=0){
            sprintf(log_text,"%s%s\t",log_text,"get socket send buff failed!");
        }
        optlen = sizeof(recv_size);
        if(getsockopt(socket, SOL_SOCKET, SO_RCVBUF, &recv_size, &optlen)!=0){
            sprintf(log_text,"%s%s\t",log_text,"get socket recv buff failed!");
        }
#endif
        
        ssize_t n=0;
        if(check_filter(on_ip,in_ip,on_port,in_port)==0){
#ifdef FILTER
            sprintf(log_text,"%s%s\n",log_text,"recv done with filter!");
            log_event(log_text);
#endif
            n=old_recvfrom(socket, buf, len, flags, addr, addrlen);
            if((on_port==80)||(in_port==80)){
                return n;
            }
            int tmp_erro=errno;
            push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),"",gettime(),F_RECVFROM,n,len,socket,RECV_FILTER);
            if(n<=0){
                errno=tmp_erro;
            }
            return n;
        }
        
        if(with_uuid == 0)
        {
            n = old_recvfrom(socket, buf, len, flags, addr, addrlen);
#ifdef DATA_INFO
            sprintf(log_text,"%srecv buff length:%zu\t recv data length:%ld\t supposed recv data length:%ld\t socket recv buff length:%d\t socket send buff length:%d\t",log_text,len,n,n-ID_LENGTH,recv_size,send_size);
            sprintf(log_text,"%srecv process:%u\t thread:%lu\t socketid:%d\t",log_text,getpid(),pthread_self(), socket);
#endif
            //if errors happened during recv, then return the error result directly
            if(n <= 0)
            {
#ifdef DEBUG
                sprintf(log_text,"%s%s\t%s\n",log_text,strerror(errno),"recvfrom done with recv error!");
                log_event(log_text);
#endif
                int tmp_erro=errno;
                push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),"",gettime(),F_RECVFROM,n,len,socket,RECV_ERROR);
                errno=tmp_erro;
                return n;
            }
#ifdef MESSAGE
            log_message(result,n);
#endif
#ifdef DEBUG
            sprintf(log_text,"%s%s\n",log_text,"recvfrom done without uuid!");
            log_event(log_text);
#endif
            push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),"",gettime(),F_RECVFROM,n,len,socket,RECV_NORMALLY);
            return n;
        }
        else
        {
#ifdef DATA_INFO
            sprintf(log_text,"%srecv buff length:%zu\t socket recv buff length:%d\t socket send buff length:%d\t",log_text,len,recv_size,send_size);
            sprintf(log_text,"%srecv process:%u\t thread:%lu\t socketid:%d\t",log_text,getpid(),pthread_self(),socket);
#endif
            char result[len];
            //            char id[ID_LENGTH];
            char *id=malloc(ID_LENGTH);//memory collected by the pthread_key_create
            long long ttime;
            ssize_t m = old_recvfrom(socket, id, 1, flags, addr, addrlen);
            if (m<=0){//no message left or errors happened!
#ifdef RW_ERR
                sprintf(log_text,"%sreal recv data length:%ld\terrorno:%d\t%s\t%s\n",log_text,m,errno,strerror(errno),"recv done with read error!");
                log_event(log_text);
#endif
                int tmp_erro=errno;
                push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),"",gettime(),F_RECVFROM,m,len,socket,RECV_ERROR);
                errno=tmp_erro;
                return m;
            }
            if(id[0]=='^')
            {
                m=old_recvfrom(socket,&id[1],1,flags, addr, addrlen);
                if(m<=0)
                {
                    memmove(buf,id,1);
                    sprintf(log_text,"%sreal recv data length:%ld\t%s\t%s\n",log_text,1L,strerror(errno),"recv done 2 with read error!");
                    log_event(log_text);
                    push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),"",gettime(),F_RECVFROM,1L,len,socket,RECV_ERROR);
                    return 1L;
                }
                if(id[1]=='^')
                {
                    m=old_recvfrom(socket,&id[2], ID_LENGTH-2, flags, addr, addrlen);
                    if(m==(ID_LENGTH-2))
                    {
                        if((id[ID_LENGTH-2]=='^')&&(id[ID_LENGTH-3]=='^'))
                        {
#ifdef DEBUG
                            sprintf(log_text,"%s%s\t",log_text,"id extracted");
#endif
                            
                            m=old_recvfrom(socket, result, len, flags, addr, addrlen);
                            //                            char uuids[m/ID_LENGTH][ID_LENGTH];
                            char **uuids=init_uuids(m);//memory collected by the pthread_key_create
                            char *id=malloc(ID_LENGTH);//memory collected by the pthread_key_create
                            char after_uuid[m];
                            size_t data_len=0;
                            data_len = get_uuid(uuids,result,m,after_uuid);
                            
                            if(data_len==m)
                            {
                                memmove(buf,result,m);
#ifdef DEBUG
                                sprintf(log_text,"%sreal recv data length:%ld\t%s\n",log_text,m,"recvfrom done!");
                                log_event(log_text);
#endif
                                push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),id,gettime(),F_RECVFROM,m+ID_LENGTH,len,socket,RECV_SINGLE);
                                return m;
                            }
                            else
                            {
                                memmove(buf,after_uuid,data_len);
#ifdef DEBUG
                                sprintf(log_text,"%sreal recv data length:%zu\t%s\n",log_text,data_len,"multi uuid !recvfrom done!");
                                log_event(log_text);
#endif
                                int num=(m-data_len)/ID_LENGTH;
                                //                                char mul_id[LOG_LENGTH];
                                char *mul_id=(char *)malloc(ID_LENGTH*num+ID_LENGTH);
                                strcat(mul_id,id);
                                int t=0;
                                for(;t<num;t++){
                                    strcat(mul_id,uuids[t]);
                                }
                                push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),mul_id,gettime(),F_RECVFROM,data_len+ID_LENGTH,len,socket,RECV_MULTI_BRO);
                                return data_len;
                            }
                            
                        }
                        else
                        {
                            
                            memmove(buf,id,ID_LENGTH);
                            m=old_recvfrom(socket, result,len-ID_LENGTH, flags, addr, addrlen);
                            //                            char uuids[m/ID_LENGTH][ID_LENGTH];
                            char **uuids=init_uuids(m);
                            char after_uuid[m];
                            size_t data_len=0;
                            data_len=get_uuid(uuids,result,m,after_uuid);
                            if(data_len==m)
                            {
                                memmove((char *)buf+ID_LENGTH,result,m);
#ifdef DEBUG
                                sprintf(log_text,"%sreal recv data length:%ld\t%s\n",log_text,m+ID_LENGTH,"recv done with similar uuid!");
                                log_event(log_text);
#endif
                                push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),"",gettime(),F_RECVFROM,m+ID_LENGTH,len,socket,RECV_SIM1);
                                return m+ID_LENGTH;
                            }
                            else
                            {
                                memmove((char *)buf+ID_LENGTH,after_uuid,data_len);
#ifdef DEBUG
                                sprintf(log_text,"%sreal recv data length:%zu\t%s\n",log_text,data_len+ID_LENGTH,"recv done with multi and similar uuid!");
                                log_event(log_text);
#endif
                                int num=(m-data_len)/ID_LENGTH;
                                //                                char mul_id[LOG_LENGTH];
                                char *mul_id=(char *)malloc(ID_LENGTH*num+ID_LENGTH);
                                int t=0;
                                for(;t<num;t++){
                                    strcat(mul_id,uuids[t]);
                                }
                                push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),mul_id,gettime(),F_RECVFROM,data_len+ID_LENGTH,len,socket,RECV_SIM1_ID);
                                return data_len+ID_LENGTH;
                            }
                        }
                    }
                    else
                    {
#ifdef DEBUG
                        sprintf(log_text,"%sreal recv data length:%ld\t%s\n",log_text,m+2,"recvfrom done with 2 starting symbols!");
                        log_event(log_text);
#endif
                        push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),"",gettime(),F_RECVFROM,m+2L,len,socket,RECV_SIM2);
                        if(len==(m+2))
                        {
                            memmove(buf,id,len);
                            
                            return len;
                        }
                        else
                        {
                            memmove(buf,id,m+2);
                            return m+2;
                        }
                    }
                }
                else
                {
                    
                    memmove(buf,id,2);
                    
                    m=old_recvfrom(socket, result, len-2, flags, addr, addrlen);
                    //                    char uuids[m/ID_LENGTH][ID_LENGTH];
                    char **uuids=init_uuids(m);
                    char after_uuid[m];
                    size_t data_len=0;
                    data_len=get_uuid(uuids,result,m,after_uuid);
                    if(data_len==m)
                    {
                        memmove((char *)buf+2,result,m);
#ifdef DEBUG
                        sprintf(log_text,"%sreal recv data length:%ld\t%s\n",log_text,m+2,"recvfrom done with 1 starting symbols!");
                        log_event(log_text);
#endif
                        push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),"",gettime(),F_RECVFROM,m+2L,len,socket,RECV_SIM3);
                        return m+2L;
                    }
                    else
                    {
                        memmove((char *)buf+2,after_uuid,data_len);
#ifdef DEBUG
                        sprintf(log_text,"%sreal recv data length:%ld\t%s\n",log_text,2+data_len,"recvfrom done with 1 starting symbols and multi uuid!");
                        log_event(log_text);
#endif
                        int num=(m-data_len)/ID_LENGTH;
                        //                        char mul_id[LOG_LENGTH];
                        char *mul_id=(char *)malloc(ID_LENGTH*num+ID_LENGTH);
                        int t=0;
                        for(;t<num;t++){
                            strcat(mul_id,uuids[t]);
                        }
                        push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),mul_id,gettime(),F_RECVFROM,2L+data_len,len,socket,RECV_SIM3_ID);
                        return 2L+data_len;
                    }
                }
                
            }
            else
            {
                
                memmove(buf,id,1);
                m=old_recvfrom(socket,result,len-1,flags,addr, addrlen);
                //                char uuids[m/ID_LENGTH][ID_LENGTH];
                char **uuids=init_uuids(m);
                char after_uuid[m];
                size_t data_len=0;
                data_len=get_uuid(uuids,result,m,after_uuid);
                if(data_len==m)
                {
                    memmove((char *)buf+1,result,m);
#ifdef DEBUG
                    sprintf(log_text,"%sreal recv data length:%ld\t%s\n",log_text,m+1,"recvfrom done without uuid!");
                    log_event(log_text);
#endif
                    push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),"",gettime(),F_RECVFROM,m+1L,len,socket,RECV_NORMALLY);
                    return m+1L;
                }
                else
                {
                    memmove((char *)buf+1,after_uuid,data_len);
#ifdef DEBUG
                    sprintf(log_text,"%sreal recv data length:%ld\t%s\n",log_text,1+data_len,"read recv with multi uuid and without uuid!");
                    log_event(log_text);
#endif
                    int num=(m-data_len)/ID_LENGTH;
                    //                    char mul_id[LOG_LENGTH];
                    char *mul_id=(char *)malloc(ID_LENGTH*2);
                    int t=0;
                    for(;t<num;t++){
                        strcat(mul_id,uuids[t]);
                    }
                    push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),mul_id,gettime(),F_RECVFROM,m+1L,len,socket,RECV_NORMALLY_ID);
                    return 1L+data_len;
                }
            }
            
        }
        
    }
    else if(socket_family == AF_UNIX)
    {
#ifdef OTHER_SOCK
        sprintf(log_text,"%s%s\t",log_text,"AF_UNIX socket");
        struct sockaddr_un uin;
        struct sockaddr_un uon;
        socklen_t s_len = sizeof(uin);
        
        if(sockType == 1)
        {
            // TCP
            getsockname(socket, (struct sockaddr *)&uin, &s_len);
            getpeername(socket, (struct sockaddr *)&uon, &s_len);
            sprintf(log_text,"%srecv from:%s\twith:%s\t recvfrom done!\n",log_text,uin.sun_path,uon.sun_path);
            log_event(log_text);
        }
        else if(sockType == 2)
        {
            // UDP
            getsockname(socket, (struct sockaddr *)&sin, &s_len);
            on_port=ntohs(((struct sockaddr_in *)addr)->sin_port);
            sprintf(log_text,"%srecv from:%s\twith:%s\t recvfrom done!\n",log_text,uin.sun_path,((struct sockaddr_in *)addr)->sun_path);
            log_event(log_text);
        }
        else
        {
            sprintf(log_text,"%s%s\n",log_text,"recvfrom done with getsocketname errors!");
            log_event(log_text);
        }
#endif
        push_to_database("",1,"",1,getpid(),pthread_self(),"",gettime(),F_RECVFROM,0,len,socket,DONE_UNIX);
    }
    else if(socket_family == UNKNOWN_FAMILY)
    {
#ifdef OTHER_SOCK
        sprintf(log_text,"%s%s\n",log_text,"recvfrom done but errors happend during getting socket family");
        log_event(log_text);
#endif
        push_to_database("",1,"",1,getpid(),pthread_self(),"",gettime(),F_RECVFROM,0,len,socket,DONE_OTHER);
    }
    else
    {
#ifdef OTHER_SOCK
        sprintf(log_text,"%s%s\t",log_text,"recvfrom done but Unknown socket\n");
        log_event(log_text);
#endif
        push_to_database("",1,"",1,getpid(),pthread_self(),"",gettime(),F_RECVFROM,0,len,socket,DONE_OTHER);
    }
    return old_recvfrom(socket, buf, len, flags, addr, addrlen);
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

extern void *_dl_sym(void *, const char *, void *);
extern void *dlsym(void *handle, const char *name)
{
    static void * (*real_dlsym)(void *, const char *)=NULL;
    if (real_dlsym == NULL)
        real_dlsym=_dl_sym(RTLD_NEXT, "dlsym", dlsym);
    
    /* my target binary is even asking for dlsym() via dlsym()... */
    if (!strcmp(name,"dlsym"))
        return (void*)dlsym;
    //    log_message(name,strlen(name));
    return real_dlsym(handle,name);
}

void *intermedia(void * arg){
    
    struct thread_param *temp;
    
    void *(*start_routine) (void *);
    temp=(struct thread_param *)arg;
    
    pthread_setspecific(uuid_key,(void *)temp->uuid);
    //for log
    //    char test[1024]="";
    //    sprintf(test,"1new ptid:%ld\ttid:%lu\tkey:%u\tvalue:%s\n",syscall(SYS_gettid),pthread_self(),uuid_key,(char*)pthread_getspecific(uuid_key));
    //    log_message(test,strlen(test));
    
    push_thread_db(getpid(),syscall(SYS_gettid),pthread_self(),temp->ppid,temp->pktid,temp->ptid,temp->ttime);
    
    return temp->start_routine(temp->args);
    //    return (*start_routine)(args);
    
    
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
    temp->ttime=gettime();
    
    
    int result=old_create(thread,attr,intermedia,(void *)temp);
    
    //    char * parameter_tmp="pid=%d\ttid=%lu\tptid=%lu\tppid=%d\tttime=%lld\trtype=%d\tresult=%d";
    //    char parameter[1024];
    //    sprintf(parameter,parameter_tmp,getpid(),*thread,tmp,0,gettime(),R_THREAD,result);
    //    log_message(parameter,strlen(parameter));
    
    //    push_thread_db(getpid(),*thread,tmp,0,gettime());
    //    sprintf(test,"%s\tnew ptid:%ld\ttid:%lu\tkey:%u\tvalue:%s\n",test,syscall(SYS_gettid),*thread,uuid_key,(char*)pthread_getspecific(uuid_key));
    //    log_message(test,strlen(test));
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
//int close(int fd)
//{
//    static void *handle = NULL;
//    static CLOSE old_close = NULL;
//    struct sockaddr_in sin;
//    struct sockaddr_in son;
//    socklen_t s_len = sizeof(sin);
//    if( !handle )
//    {
//        handle = dlopen("libc.so.6", RTLD_LAZY);
//        old_close = (CLOSE)dlsym(handle, "close");
//    }
//    if  ((getsockname(fd, (struct sockaddr *)&sin, &s_len) != -1) &&(getpeername(fd, (struct sockaddr *)&son, &s_len) != -1)){
//        unsigned short int in_port;
//        unsigned short int on_port;
//        char *in_ip;
//        char *on_ip;
//        in_port=ntohs(sin.sin_port);
//        in_ip=inet_ntoa(sin.sin_addr);
//        on_port=ntohs(son.sin_port);
//        on_ip=inet_ntoa(son.sin_addr);
//
//        // filter our own service, db and controller
//        if ((in_port==80) || (on_port==80)||(strcmp(in_ip,controllerip)==0)||(strcmp(on_ip,controllerip)==0)){
//            return old_close(fd);
//        }else{
//            mark_socket_close(fd);
//            return old_close(fd);
//        }
//
//        return old_close(fd);
//    }
//
//    return old_close(fd);
//
//}

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
    char **a=(char **)malloc(m/ID_LENGTH * sizeof(char *));
    int i=0;
    for (i=0;i<m/ID_LENGTH;i++){
        a[i]=malloc(ID_LENGTH);
    }
    return a;
}

//int get_uuid(char result[][ID_LENGTH],const char* buf,int len,char* after_uuid){
int get_uuid(char **result,const char* buf,int len,char* after_uuid){
    int id_count=0;
    int i;
    int j=0;
    short start=0;
    //    char uuid[ID_LENGTH+4];
    for(i=0;i<len;i++){
        if ((buf[i]=='^')&&((i+1)<len)&&(buf[i+1]=='^')){
            if((i+ID_LENGTH)>len){
                //possiblely broken uuids, currently processed as normal char
                after_uuid[j++]=buf[i];
                log_event("possibly broken uuid\n");
            }else if((buf[i+ID_LENGTH-2]=='^')&&(buf[i+ID_LENGTH-3]=='^')){
                memmove(result[id_count++],&buf[i],ID_LENGTH);
                //                printf("hehe:%s\n",result[id_count-1]);
                i=i+ID_LENGTH-1;
            }else if(i<=(ID_LENGTH-3)){
                //possiblely broken uuids, currently processed as normal char
                after_uuid[j++]=buf[i];
                log_event("possibly broken uuid\n");
            }else{
                //normal ^^
                log_event("danger\n");
                after_uuid[j++]=buf[i];
            }
        }else{
            after_uuid[j++]=buf[i];
        }
    }
    return j;
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
    
    buf[ID_LENGTH-3]='^';
    buf[ID_LENGTH-2]='^';
    buf[ID_LENGTH-1]='\0';
    buf[0]='^';
    buf[1]='^';
    return buf;
}
long long gettime(){
    struct timeb t;
    ftime(&t);
    return 1000 * t.time + t.millitm;
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
        fprintf(logFile,"%s", info);
        fclose(logFile);
        
    }else{
        printf("Cannot open file!");
        
    }
    
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
    if(length<1){
        return;
    }
    if(length>1000){
        return;
    }
    FILE *logFile;
    char result[LOG_LENGTH]="";
    
    if((logFile=fopen(filePath2,"a+"))!=NULL){
        sprintf(result,"message:\t");
        
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
    
}
//return 0, if this message is recv/send from outer host/port and it should be recv/send directly
//return 1, if this message is internal message
int check_filter(char* on_ip,char* in_ip,int on_port,int in_port){
    int ip_list_length=4;
    int port_list_length=3;
    char *legal_ip_list[]={"127.0.0.1","10.211.55.36","10.211.55.37","10.211.55.38"};
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


//type=24
int push_to_database(char* on_ip,int on_port,char* in_ip, int in_port,pid_t pid,pthread_t tid,char* uuid,long long time,char ftype,long length,long supposed_length,int socketid,char dtype){
    if(uuid_key==12345){
        log_message("hahaha",6);
        pthread_key_create(&uuid_key,NULL);
        char * tmp_env=(char *)malloc(1024); //never free
        int env_null=get_own_env(tmp_env);
        log_message(tmp_env,strlen(tmp_env));
        if(!env_null){
            pthread_setspecific(uuid_key,tmp_env);
        }
        
    }
    
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
    // To do
    char * parameter_tmp="on_ip=%s&on_port=%d&in_ip=%s&in_port=%ld&pid=%u&ktid=%ld&tid=%lu&uuid=%s&unit_uuid=%s&ttime=%lld&ftype=%d&length=%ld&supposed_length=%ld&rtype=%d&socket=%d&dtype=%d";
    char parameter[1024];
    sprintf(parameter,parameter_tmp,on_ip,on_port,in_ip,in_port,pid,syscall(SYS_gettid),tid,uuid,unit_uuid,time,ftype,length,supposed_length,R_DATABASE,socketid,dtype);
    return getresponse(parameter);
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
