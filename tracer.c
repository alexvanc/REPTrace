#include "tracer_experiment.h"





//0 controller off
//1 controller on
const short with_uuid=1;

const char* controllerip="10.211.55.38";
const char* controller_service="10.211.55.38/controller.php";
const char* filePath="/tmp/tracelog.txt";
const char* debugFilePath="/tmp/tracelog2.txt";
const char* dataFilePath="/tmp/traceData.dat";
const char* errFilePath="/tmp/traceErr.txt";

const char* logRule="logs/userlogs";

const char* idenv="UUID_PASS";
static pthread_key_t uuid_key=12345;
extern char **environ;


//with previous implementation and current read (without push)


ssize_t recv(int sockfd, void *buf, size_t len, int flags)
{
    init_context();
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
    //currently only support for IPv4
    if (socket_family==AF_INET){
#ifdef DEBUG
        sprintf(log_text,"%s%s\t",log_text,"AF_INET socket");
#endif
        int tmp_erro=0;
        struct sockaddr_in sin; //local socket info
        struct sockaddr_in son; //remote socket into
        socklen_t s_len = sizeof(sin);
        unsigned short int in_port=0;
        unsigned short int on_port=0;
        char in_ip[256]="0.0.0.0";
        char on_ip[256]="0.0.0.0";
        if(getsockname(sockfd, (struct sockaddr *)&sin, &s_len) != -1){
            char *tmp_in_ip;
            in_port=ntohs(sin.sin_port);
            tmp_in_ip=inet_ntoa(sin.sin_addr);
            memmove(in_ip,tmp_in_ip,strlen(tmp_in_ip)+1);
        }else{
            errno=0;
            log_important("fatal_getsock_recv");
#ifdef DEBUG
            sprintf(log_text,"%s%s\t",log_text,"fatal_getsock_recv");
#endif
        }
        
        //no connection, actually for recv, no connection now already means error
        if(getpeername(sockfd, (struct sockaddr *)&son, &s_len) != -1){
            char *tmp_on_ip;
            on_port=ntohs(son.sin_port);
            tmp_on_ip=inet_ntoa(son.sin_addr);
            memmove(on_ip,tmp_on_ip,strlen(tmp_on_ip)+1);
        }else{
            errno=0;
            log_important("fatal_getpeer_recv");
#ifdef DEBUG
            sprintf(log_text,"%s%s\t",log_text,"fatal_getpeer_recv");
#endif
        }
        
#ifdef IP_PORT
        sprintf(log_text,"%srecv from %s:%d with %s:%d\t",log_text,on_ip,on_port,in_ip,in_port);
#endif
        
        //actually this information is currently unused
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
#ifdef DEBUG
        sprintf(log_text,"%srecv process:%u\t thread:%lu\t socketid:%d\trequired:%lu\t",log_text,getpid(),pthread_self(),sockfd,len);
#endif
        //for filtered service
        ssize_t n=0;
        if(check_filter(on_ip,in_ip,on_port,in_port)==0){
            
            n=old_recv(sockfd,buf,len,flags);
            tmp_erro=errno;
#ifdef FILTER
            sprintf(log_text,"%sresult:%ld\t%s\n",log_text,n,"recv done with filter!");
            log_event(log_text);
#endif
            //currently saved to local file, unused
            if((on_port==80)||(in_port==80)){
                return n;
            }
            push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),"",gettime(),F_RECV,n,len,n,sockfd,RECV_FILTER,buf);
            errno=tmp_erro;
            return n;
        }
        
        
        //not modify messages, just record
        if (with_uuid==0){
            n=old_recv(sockfd,buf,len,flags);
            tmp_erro=errno;
#ifdef DEBUG
            sprintf(log_text,"%sreal recv:%ld\tresult:%ld\t",log_text,n,n);
#endif
            //if errors happened during recv, then return the error result directly
            //            if(n<=0){
            //#ifdef DEBUG
            //                sprintf(log_text,"%s%s\t%s\n",log_text,strerror(errno),"recv done with recv error!");
            //                log_event(log_text);
            //#endif
            //                push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),"",gettime(),F_RECV,n,len,n,sockfd,RECV_ERROR,buf);
            //                errno=tmp_erro;
            //                return n;
            //            }
#ifdef MESSAGE
            log_message(buf,n,"r_recv");
#endif
#ifdef DEBUG
            sprintf(log_text,"%s%s\n",log_text,"recv done without uuid!");
            log_event(log_text);
#endif
            push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),"",gettime(),F_RECV,n,len,n,sockfd,RECV_NORMALLY,buf);
            errno=tmp_erro;
            return n;
        }else{
            
            char uuids[LOG_LENGTH];
            ssize_t n=0;
            size_t original_length=0;
            
            //            char logs[LOG_LENGTH]="in recv";
            
            size_t left=op_storage(S_GET,sockfd,original_length);
#ifdef DEBUG
            sprintf(log_text,"%sleft:%lu\t",log_text,left);
#endif
            if (left!=0){
                n=check_read_rest(buf,sockfd,left,len,flags);
                tmp_erro=errno;
                errno=0;// not sure necessary or not
#ifdef DEBUG
                sprintf(log_text,"%sresult:%ld\n",log_text,n);
                log_event(log_text);
#endif
                push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),"",gettime(),F_RECV,n,len,n,sockfd,RECV_LEFT,buf);
                errno=tmp_erro;
                return n;
            }else{
                ssize_t header_length;
                header_length=check_read_header(uuids,sockfd,&original_length,flags);
                tmp_erro=errno;
                errno=0;
                if(header_length==ID_LENGTH){
                    if(original_length==0){// we haven't added ID to the other event
                        log_important("note_noid_recv");
                        if(header_length>len){//temporary solution, may error, should have a real storage like before to put the left butys
                            log_important("fatal_bigger_recv");
                            memmove(buf,uuids,len);
#ifdef DEBUG
                            sprintf(log_text,"%sheader:%lu\tnormal\tresult:%ld\n",log_text,original_length,len);
                            log_event(log_text);
#endif
                            push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),"",gettime(),F_RECV,ID_LENGTH,len,len,sockfd,RECV_FAIL,buf);
                            errno=tmp_erro;
                            return len;
                        }else{
                            memmove(buf,uuids,header_length);
                            ssize_t second=old_recv(sockfd,&buf[header_length],len-header_length,flags);
                            tmp_erro=errno;
                            if (second<=0){
                                log_important("note_noid_error_recv");
                            }else{
                                n=header_length+second;
                            }
#ifdef DEBUG
                            sprintf(log_text,"%sheader:%lu\tnormal2\tresult:%ld\n",log_text,original_length,n);
                            log_event(log_text);
#endif
                            push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),"",gettime(),F_RECV,n,len,n,sockfd,RECV_BYD,buf);
                            errno=tmp_erro;
                            return n;
                        }
                        
                    }else{
                        n=check_read_rest(buf,sockfd,original_length,len,flags);
                        tmp_erro=errno;
                        errno=0;
                        char tmp_id[ID_LENGTH];
                        format_uuid(uuids,tmp_id);
#ifdef DEBUG
                        
                        sprintf(log_text,"%suuid:%s\theader:%ld\tresult:%ld\n",log_text,tmp_id,original_length,n);
                        log_event(log_text);
#endif
                        push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),tmp_id,gettime(),F_RECV,ID_LENGTH,len,len,sockfd,RECV_ID,buf);
                        errno=tmp_erro;
                        return n;
                    }
                }else{
                    char f_type=-1;
                    if(header_length!=-1){
                        f_type=RECV_HEADFAIL;
                        //                        log_important("note_nohead_recv");
                    }else{
                        f_type=RECV_HEADERR;
                    }
#ifdef DEBUG
                    sprintf(log_text,"%sresult:%ld\tempty\n",log_text,header_length);
                    log_event(log_text);
#endif
                    push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),"",gettime(),F_RECV,header_length,len,header_length,sockfd,f_type,NULL);
                    errno=tmp_erro;
                    return header_length;
                }
            }
            
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
        push_to_database("",1,"",1,getpid(),pthread_self(),"",0,F_RECV,0,len,0,sockfd,DONE_UNIX,"");
    }else if(socket_family==UNKNOWN_FAMILY){
#ifdef OTHER_SOCK
        sprintf(log_text,"%s%s\n",log_text,"recv done but errors happend during getting socket family");
        log_event(log_text);
#endif
        push_to_database("",1,"",1,getpid(),pthread_self(),"",0,F_RECV,0,len,0,sockfd,DONE_OTHER,"");
    }else{
#ifdef OTHER_SOCK
        sprintf(log_text,"%s%s\t",log_text,"recv done but Unknown socket\n");
        log_event(log_text);
#endif
        push_to_database("",1,"",1,getpid(),pthread_self(),"",0,F_RECV,0,len,0,sockfd,DONE_OTHER,"");
    }
    
    return old_recv(sockfd, buf, len, flags);
    
}

ssize_t send(int sockfd, const void *buf, size_t len, int flags)
{
    init_context();
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
        int tmp_erro=0;
        struct sockaddr_in sin; //local socket info
        struct sockaddr_in son; //remote socket into
        socklen_t s_len = sizeof(sin);
        unsigned short int in_port=0;
        unsigned short int on_port=0;
        char in_ip[256]="0.0.0.0";
        char on_ip[256]="0.0.0.0";
        if(getsockname(sockfd, (struct sockaddr *)&sin, &s_len) != -1){
            char *tmp_in_ip;
            in_port=ntohs(sin.sin_port);
            tmp_in_ip=inet_ntoa(sin.sin_addr);
            memmove(in_ip,tmp_in_ip,strlen(tmp_in_ip)+1);
        }else{
            errno=0;
            log_important("fatal_getsock_send");
#ifdef DEBUG
            sprintf(log_text,"%s%s\t",log_text,"fatal_getsock_send");
#endif
        }
        
        //no connection, actually for recv, no connection now already means error
        if(getpeername(sockfd, (struct sockaddr *)&son, &s_len) != -1){
            char *tmp_on_ip;
            on_port=ntohs(son.sin_port);
            tmp_on_ip=inet_ntoa(son.sin_addr);
            memmove(on_ip,tmp_on_ip,strlen(tmp_on_ip)+1);
        }else{
            errno=0;
            log_important("fatal_getpeer_send");
#ifdef DEBUG
            sprintf(log_text,"%s%s\t",log_text,"fatal_getpeer_send");
#endif
        }
        
#ifdef IP_PORT
        sprintf(log_text,"%ssend to %s:%d with %s:%d\t",log_text,on_ip,on_port,in_ip,in_port);
#endif
        
        //actually this information is currently unused
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
        ssize_t n=0;
        //        if((SIZE_MAX-len)<MAX_SPACE){
        //#ifdef DEBUG
        //            sprintf(log_text,"%s%s\t%s\n",log_text,"too large buf size, cannot add uuid to it","write done!");
        //            log_event(log_text);
        //#endif
        //            n=old_send(sockfd, buf, len, flags);
        //            int tmp_erro=errno;
        //            push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),"",gettime(),F_SEND,n,len,n,sockfd,SEND_ERROR,buf);
        //            if(n<=0){
        //                errno=tmp_erro;
        //            }
        //            return n;
        //        }
        
#ifdef DEBUG
        sprintf(log_text,"%ssupposed send data:%lu\tnew data length:%zu\tsend process:%u\t thread:%lu\t socketid:%d\t",log_text,len,new_len,getpid(),pthread_self(),sockfd);
#endif
        
        if(check_filter(on_ip,in_ip,on_port,in_port)==0){
            n=old_send(sockfd, buf, len, flags);
            tmp_erro=errno;
#ifdef FILTER
            sprintf(log_text,"%sresult:%ld\t%s\n",log_text,n,"send done with filter");
            log_event(log_text);
#endif
            if((on_port==80)||(in_port==80)){
                return n;
            }
            int tmp_erro=errno;
            push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),"",gettime(),F_SEND,n,len,n,sockfd,SEND_FILTER,buf);
            errno=tmp_erro;
            return n;
        }
        
        if(with_uuid==0){//no uuid, recv this message
            
#ifdef MESSAGE
            log_message((const char*)buf,len,"r_send");
#endif
            n=old_send(sockfd, buf, len, flags);
            tmp_erro=errno;
#ifdef DEBUG
            sprintf(log_text,"%sresult:%ld\t%s\n",log_text,n,"send done without uuid!");
            log_event(log_text);
#endif
            push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),"",gettime(),F_SEND,n,len,n,sockfd,SEND_NORMALLY,buf);
            errno=tmp_erro;
            return n;
        }else{
            char f_type=-1;
            char target[new_len];
            char id[ID_LENGTH];
            random_uuid(id,len);
            char tmp_id[ID_LENGTH];
            format_uuid(id,tmp_id);
            
            memmove(&target,id,ID_LENGTH);
            memmove(&target[ID_LENGTH],buf,len);
#ifdef MESSAGE
            log_message(target,new_len,"r_send");
#endif
            ssize_t rvalue=0;
            errno=0;
            long long ttime;
            ttime=gettime();
            n=old_send(sockfd, target, new_len, flags);
            tmp_erro=errno;
            errno=0;
#ifdef DEBUG
            sprintf(log_text,"%sfirst err:%d\tfirst send:%ld\t",log_text,tmp_erro,n);
#endif
            if(n==new_len){
                rvalue=len;
                f_type=SEND_ID;
#ifdef DEBUG
                sprintf(log_text,"%suuid:%s\tnormal1\tresult:%ld\n",log_text,tmp_id,rvalue);
                log_event(log_text);
#endif
            }else if(n==0){
#ifdef DEBUG
                sprintf(log_text,"%suuid:%s\tdisconnect\tresult:%ld\n",log_text,id,rvalue);
                log_event(log_text);
#endif
                f_type=SEND_DISCONN;
                rvalue=0;
            }else{
                tmp_erro=errno;
                while((n!=new_len)&&((tmp_erro==0)||(tmp_erro==EAGAIN)||(tmp_erro==EWOULDBLOCK))){
                    ssize_t tmpValue=0;
                    if(n!=-1){
                        tmpValue=old_send(sockfd, &target[n], new_len-n, flags);
                    }else{
                        tmpValue=old_send(sockfd, &target[0], new_len, flags);
                    }
                    tmp_erro=errno;
                    if(tmpValue>=0){
                        n+=tmpValue;
                    }
                    //                    log_message("iteration",10,"send");
                    //#ifdef DEBUG
                    //                    sprintf(log_text,"%ssend iteration\terrno:%d\t",log_text,tmp_erro);
                    //#endif
                }
                if(n==new_len){
#ifdef DEBUG
                    sprintf(log_text,"%suuid:%s\tnormal2\tresult:%ld\n",log_text,tmp_id,len);
#endif
                    f_type=SEND_ID;
                    rvalue=len;
                }else if(n==-1){
#ifdef DEBUG
                    sprintf(log_text,"%sfinal err:%d\tsend short error\tresult:%ld\n",log_text,tmp_erro,-1L);
#endif
                    f_type=SEND_ERROR;
                    errno=tmp_erro;
                    rvalue=-1;
                }else if((n<=new_len)&&(n>=ID_LENGTH)){
#ifdef DEBUG
                    sprintf(log_text,"%sfinal err:%d\tsend short1 error\tresult:%ld\n",log_text,tmp_erro,n-ID_LENGTH);
#endif
                    log_important("fatal_short1_send");
                    f_type=SEND_BROKEN;
                    rvalue=n-ID_LENGTH;
                    //                return n-39;
                }else if((n<ID_LENGTH)&&(n>0)){
#ifdef DEBUG
                    sprintf(log_text,"%sfinal err:%d\tsend short2 error\tresult:%ld\n",log_text,tmp_erro,-1L);
#endif
                    log_important("fatal_short2_send");
                    f_type=SEND_FAIL;
                    rvalue=-1;
                }else{
#ifdef DEBUG
                    sprintf(log_text,"%sfinal err:%d\tsend short3 error\tresult:%ld\n",log_text,tmp_erro,0L);
#endif
                    log_important("note_short3_send");
                    f_type=SEND_OTHER;
                    rvalue=0;
                }
                
            }
#ifdef DEBUG
            log_event(log_text);
#endif
            errno=0;
            push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),tmp_id,gettime(),F_SEND,n,len,rvalue,sockfd,f_type,buf);
            errno=tmp_erro;
            return rvalue;
        }
        
    }
    //    else if(socket_family==AF_UNIX){
    //#ifdef OTHER_SOCK
    //        sprintf(log_text,"%s%s\t",log_text,"AF_UNIX socket");
    //        struct sockaddr_un uin;
    //        struct sockaddr_un uon;
    //        socklen_t s_len = sizeof(uin);
    //        if ((getsockname(sockfd, (struct sockaddr *)&uin, &s_len) != -1) &&(getpeername(sockfd, (struct sockaddr *)&uon, &s_len) != -1)){
    //            sprintf(log_text,"%ssend to:%s\twith:%s\t send done!\n",log_text,uin.sun_path,uon.sun_path);
    //            log_event(log_text);
    //        }else{
    //            sprintf(log_text,"%s%s\n",log_text,"send done with getsocketname errors!");
    //            log_event(log_text);
    //        }
    //#endif
    //        push_to_database("",1,"",1,getpid(),pthread_self(),"",0,F_SEND,0,len,0,sockfd,DONE_UNIX,"");
    //    }else if(socket_family==UNKNOWN_FAMILY){
    //#ifdef OTHER_SOCK
    //        sprintf(log_text,"%s%s\n",log_text,"send done but errors happend during getting socket family");
    //        log_event(log_text);
    //#endif
    //        push_to_database("",1,"",1,getpid(),pthread_self(),"",0,F_SEND,0,len,0,sockfd,DONE_OTHER,"");
    //    }else{
    //#ifdef OTHER_SOCK
    //        sprintf(log_text,"%s%s\n",log_text,"send done but Unknown socket");
    //        log_event(log_text);
    //#endif
    //        push_to_database("",1,"",1,getpid(),pthread_self(),"",0,F_SEND,0,len,0,sockfd,DONE_OTHER,"");
    //    }
    return old_send(sockfd,buf,len,flags);
}

ssize_t write(int fd, const void *buf, size_t count)
{
    init_context();
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
        int tmp_erro=0;
        struct sockaddr_in sin; //local socket info
        struct sockaddr_in son; //remote socket into
        socklen_t s_len = sizeof(sin);
        unsigned short int in_port=0;
        unsigned short int on_port=0;
        char in_ip[256]="0.0.0.0";
        char on_ip[256]="0.0.0.0";
        if(getsockname(fd, (struct sockaddr *)&sin, &s_len) != -1){
            char *tmp_in_ip;
            in_port=ntohs(sin.sin_port);
            tmp_in_ip=inet_ntoa(sin.sin_addr);
            memmove(in_ip,tmp_in_ip,strlen(tmp_in_ip)+1);
        }else{
            errno=0;
            log_important("fatal_getsock_write");
#ifdef DEBUG
            sprintf(log_text,"%s%s\t",log_text,"fatal_getsock_write");
#endif
        }
        
        //no connection, actually for recv, no connection now already means error
        if(getpeername(fd, (struct sockaddr *)&son, &s_len) != -1){
            char *tmp_on_ip;
            on_port=ntohs(son.sin_port);
            tmp_on_ip=inet_ntoa(son.sin_addr);
            memmove(on_ip,tmp_on_ip,strlen(tmp_on_ip)+1);
        }else{
            errno=0;
            log_important("fatal_getpeer_write");
#ifdef DEBUG
            sprintf(log_text,"%s%s\t",log_text,"fatal_getpeer_write");
#endif
        }
        
#ifdef IP_PORT
        sprintf(log_text,"%swrite to %s:%d with %s:%d\t",log_text,on_ip,on_port,in_ip,in_port);
#endif
        
        //actually this information is currently unused
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
        //        if((SIZE_MAX-count)<MAX_SPACE){
        //#ifdef DEBUG
        //            sprintf(log_text,"%s%s\t%s\n",log_text,"get socket recv buff failed!","write done!");
        //            log_event(log_text);
        //#endif
        //            n=old_write(fd, buf, count);
        //            int tmp_erro=errno;
        //            push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),"",gettime(),F_WRITE,n,count,n,fd,SEND_ERROR,buf);
        //            if(n<=0){
        //                errno=tmp_erro;
        //            }
        //            return n;
        //        }
#ifdef DEBUG
        sprintf(log_text,"%ssupposed write data length:%zu\tnew data length:%lu\twrite process:%d\t thread:%lu\t socketid:%d\t",log_text,count,new_len,getpid(),pthread_self(),fd);
#endif
        
        if(check_filter(on_ip,in_ip,on_port,in_port)==0){
            
            n=old_write(fd, buf, count);
            tmp_erro=errno;
#ifdef FILTER
            sprintf(log_text,"%sreturn:%ld\t%s\n",log_text,n,"write done with filter!");
            log_event(log_text);
#endif
            if((on_port==80)||(in_port==80)){
                errno=tmp_erro;
                return n;
            }
            push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),"",gettime(),F_WRITE,n,count,n,fd,SEND_FILTER,buf);
            errno=tmp_erro;
            return n;
        }
        
        
        if(with_uuid==0){//no uuid, write this message
            
#ifdef MESSAGE
            log_message( (char*)buf,count,"r_write");
#endif
            n=old_write(fd, buf, count);
            tmp_erro=errno;
#ifdef DEBUG
            sprintf(log_text,"%sresult:%ld\t%s\n",log_text,n,"write done without uuid!");
            log_event(log_text);
#endif
            push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),"",gettime(),F_WRITE,n,count,n,fd,SEND_NORMALLY,buf);
            errno=tmp_erro;
            return n;
        }else{
            char f_type=-1;
            char target[new_len];
            char id[ID_LENGTH];
            //            char *id=malloc(ID_LENGTH);//memory collected by the pthread_key_create
            random_uuid(id,count);
            char tmp_id[ID_LENGTH];
            format_uuid(id,tmp_id);
            memmove(&target,id,ID_LENGTH);
            memmove(&target[ID_LENGTH],buf,count);
#ifdef MESSAGE
            log_message(target,new_len,"r_write");
#endif
            ssize_t rvalue=0;
            long long ttime;
            ttime=gettime();
            errno=0;//unsure
            n=old_write(fd, target, new_len);
            tmp_erro=errno;
            errno=0;
#ifdef DEBUG
            sprintf(log_text,"%sfirst err:%d\tfirst write:%ld\t",log_text,tmp_erro,n);
#endif
            if(n==new_len){
                f_type=SEND_ID;
                rvalue=count;
#ifdef DEBUG
                sprintf(log_text,"%suuid:%s\tnormal1\tresult:%ld\n",log_text,tmp_id,rvalue);
                log_event(log_text);
#endif
            }else if(n==0){
#ifdef DEBUG
                sprintf(log_text,"%suuid:%s\tdisconnect\tresult:%ld\n",log_text,tmp_id,rvalue);
                log_event(log_text);
#endif
                f_type=SEND_DISCONN;
                rvalue=0;
            }else{
                while((n!=new_len)&&((tmp_erro==0)||(tmp_erro==EAGAIN)||(tmp_erro==EWOULDBLOCK))){
                    ssize_t tmpValue=0;
                    if(n!=-1){
                        tmpValue=old_write(fd, &target[n], new_len-n);
                    }else{
                        tmpValue=old_write(fd, &target[0], new_len);
                    }
                    tmp_erro=errno;
                    if(tmpValue>=0){
                        n+=tmpValue;
                    }
                    //                    log_message("iteration",10,"write");
                    //#ifdef DEBUG
                    //this will result erro
                    //                    sprintf(log_text,"%swrite iteration\terrno:%d\t",log_text,tmp_erro);
                    //#endif
                }
                if(n==new_len){
#ifdef DEBUG
                    sprintf(log_text,"%suuid:%s\tnormal2\tresult:%ld\n",log_text,tmp_id,count);
#endif
                    f_type=SEND_ID;
                    rvalue=count;
                }else if(n==-1){
                    errno=tmp_erro;
#ifdef DEBUG
                    sprintf(log_text,"%sfinal err:%d\twrite short1 error\tresult:%ld\n",log_text,tmp_erro,-1L);
#endif
                    f_type=SEND_ERROR;
                    rvalue=-1;
                }else if((n<new_len)&&(n>=ID_LENGTH)){
#ifdef DEBUG
                    sprintf(log_text,"%sfinal err:%d\twrite short2 error\tresult:%ld\n",log_text,tmp_erro,n-ID_LENGTH);
#endif
                    log_important("fatal_short1_write");
                    f_type=SEND_BROKEN;
                    rvalue=n-ID_LENGTH;
                    //                return n-39;
                }else if((n<ID_LENGTH)&&(n>0)){
#ifdef DEBUG
                    sprintf(log_text,"%sfinal err:%d\twrite short2 error\tresult:%ld\n",log_text,tmp_erro,-1L);
#endif
                    log_important("fatal_short2_write");
                    f_type=SEND_FAIL;
                    rvalue=-1;
                }else{
#ifdef DEBUG
                    sprintf(log_text,"%sfinal err:%d\twrite short2 error\tresult:%ld\n",log_text,tmp_erro,0L);
#endif
                    log_important("note_short3_write");
                    f_type=SEND_OTHER;
                    rvalue=0;
                }
                
            }
            errno=0;
            push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),tmp_id,gettime(),F_WRITE,n,count,rvalue,fd,f_type,buf);
#ifdef DEBUG
            log_event(log_text);
#endif
            errno=tmp_erro;
            return rvalue;
        }
        
    }
    //    else if(socket_family==AF_UNIX){
    //#ifdef OTHER_SOCK
    //        sprintf(log_text,"%s%s\t",log_text,"AF_UNIX socket");
    //        struct sockaddr_un uin;
    //        struct sockaddr_un uon;
    //        socklen_t s_len = sizeof(uin);
    //        if ((getsockname(fd, (struct sockaddr *)&uin, &s_len) != -1) &&(getpeername(fd, (struct sockaddr *)&uon, &s_len) != -1)){
    //            sprintf(log_text,"%swrite to:%s\twith:%s\t write done!\n",log_text,uin.sun_path,uon.sun_path);
    //            log_event(log_text);
    //        }else{
    //            sprintf(log_text,"%s%s\n",log_text,"write done with getsocketname errors!");
    //            log_event(log_text);
    //        }
    //#endif
    //        push_to_database("",1,"",1,getpid(),pthread_self(),"",0,F_WRITE,0,count,0,fd,DONE_UNIX,"");
    //    }else if(socket_family==UNKNOWN_FAMILY){
    //#ifdef OTHER_SOCK
    //        sprintf(log_text,"%s%s\n",log_text,"write done but errors happend during getting socket family");
    //        log_event(log_text);
    //#endif
    //        push_to_database("",1,"",1,getpid(),pthread_self(),"",0,F_WRITE,0,count,fd,0,DONE_OTHER,"");
    //    }else{
    //#ifdef OTHER_SOCK
    //        sprintf(log_text,"%s%s\n",log_text,"write done but Unknown socket");
    //        log_event(log_text);
    //#endif
    //        push_to_database("",1,"",1,getpid(),pthread_self(),"",0,F_WRITE,0,count,fd,0,DONE_OTHER,"");
    //    }
    
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
        int tmp_erro=0;
        struct sockaddr_in sin; //local socket info
        struct sockaddr_in son; //remote socket into
        socklen_t s_len = sizeof(sin);
        unsigned short int in_port=0;
        unsigned short int on_port=0;
        char in_ip[256]="0.0.0.0";
        char on_ip[256]="0.0.0.0";
        if(getsockname(fd, (struct sockaddr *)&sin, &s_len) != -1){
            char *tmp_in_ip;
            in_port=ntohs(sin.sin_port);
            //https://linux.die.net/man/3/inet_ntoa reference
            tmp_in_ip=inet_ntoa(sin.sin_addr);
            memmove(in_ip,tmp_in_ip,strlen(tmp_in_ip)+1);
        }else{
            errno=0;
            log_important("fatal_getsock_read");
#ifdef DEBUG
            sprintf(log_text,"%s%s\t",log_text,"fatal_getsock_read");
#endif
        }
        
        //no connection, actually for recv, no connection now already means error
        if(getpeername(fd, (struct sockaddr *)&son, &s_len) != -1){
            char *tmp_on_ip;
            on_port=ntohs(son.sin_port);
            tmp_on_ip=inet_ntoa(son.sin_addr);
            memmove(on_ip,tmp_on_ip,strlen(tmp_on_ip)+1);
        }else{
            errno=0;
            log_important("fatal_getpeer_read");
#ifdef DEBUG
            sprintf(log_text,"%s%s\t",log_text,"fatal_getpeer_read");
#endif
        }
        
#ifdef IP_PORT
        sprintf(log_text,"%sread from %s:%d with %s:%d\t",log_text,on_ip,on_port,in_ip,in_port);
#endif
        
        //actually this information is currently unused
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
#ifdef DEBUG
        sprintf(log_text,"%sread process:%u\t thread:%lu\t socketid:%d\trequired:%lu\t",log_text,getpid(),pthread_self(),fd,count);
#endif
        ssize_t n=0;
        if(check_filter(on_ip,in_ip,on_port,in_port)==0){
            n=old_read(fd,buf,count);
            tmp_erro=errno;
#ifdef FILTER
            sprintf(log_text,"%sresult:%ld\t%s\n",log_text,n,"read done with filter!");
            log_event(log_text);
#endif
            if((on_port==80)||(in_port==80)){
                return n;
            }
            push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),"",gettime(),F_READ,n,count,0,fd,RECV_FILTER,buf);
            errno=tmp_erro;
            return n;
        }
        
        if(with_uuid==0){//no uuid, read this message
            
            n=old_read(fd, buf, count);
            tmp_erro=errno;
#ifdef DEBUG
            sprintf(log_text,"%sreal read:%ld\tresult:%ld\t",log_text,n,n);
#endif
            //            if(n<=0){
            //#ifdef DEBUG
            //                sprintf(log_text,"%serrorno:%d\t%s\t%s\n",log_text,errno,strerror(errno),"read done with read error!");
            //                log_event(log_text);
            //#endif
            //                push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),"",gettime(),F_READ,n,count,n,fd,RECV_ERROR,"");
            //                errno=tmp_erro;
            //                return n;
            //            }
#ifdef MESSAGE
            log_message(buf,n,"r_read");
#endif
#ifdef DEBUG
            sprintf(log_text,"%s%s\n",log_text,"read done without uuid!");
            log_event(log_text);
#endif
            push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),"",gettime(),F_READ,n,count,n,fd,RECV_NORMALLY,buf);
            errno=tmp_erro;
            return n;
        }else{
            char uuids[LOG_LENGTH];
            size_t original_length=0;
            ssize_t n=0;
            size_t left=op_storage(S_GET,fd,original_length);
#ifdef DEBUG
            sprintf(log_text,"%sleft:%lu\t",log_text,left);
#endif
            if (left!=0){
                n=check_read_rest(buf,fd,left,count,0);
                tmp_erro=errno;
                errno=0;
#ifdef DEBUG
                sprintf(log_text,"%sresult:%ld\n",log_text,n);
                log_event(log_text);
#endif
                push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),"",gettime(),F_READ,n,count,n,fd,RECV_LEFT,buf);
                errno=tmp_erro;
                return n;
            }else{
                ssize_t header_length;
                
                header_length=check_read_header(uuids,fd,&original_length,0);
                tmp_erro=errno;
                errno=0;
                if(header_length==ID_LENGTH){
                    if(original_length==0){// we haven't added ID to the writev/other event
                        log_important("note_noid_read");
                        if(header_length>count){//temporary solution, may error
                            log_important("fatal_bigger_read");
                            memmove(buf,uuids,count);
#ifdef DEBUG
                            sprintf(log_text,"%sheader:%lu\tnormal\tresult:%ld\n",log_text,original_length,count);
                            log_event(log_text);
#endif
                            push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),"",gettime(),F_READ,ID_LENGTH,count,count,fd,RECV_FAIL,buf);
                            errno=tmp_erro;
                            return count;
                        }else{
                            memmove(buf,uuids,header_length);
                            ssize_t second=old_read(fd,&buf[header_length],count-header_length);
                            if (second<=0){
                                log_important("note_noid_error_read");
                            }else{
                                n=header_length+second;
                            }
#ifdef DEBUG
                            sprintf(log_text,"%sheader:%lu\tnormal2\tresult:%ld\n",log_text,original_length,n);
                            log_event(log_text);
#endif
                            push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),"",gettime(),F_READ,n,count,n,fd,RECV_BYD,buf);
                            errno=tmp_erro;
                            return n;
                        }
                        
                    }else{
                        n=check_read_rest(buf,fd,original_length,count,0);
                        tmp_erro=errno;
                        errno=0;
                        char tmp_id[ID_LENGTH];
                        format_uuid(uuids,tmp_id);
#ifdef DEBUG
                        char tmp_id[ID_LENGTH];
                        format_uuid(uuids,tmp_id);
                        sprintf(log_text,"%s\tuuid:%s\theader:%ld\tresult:%ld\n",log_text,tmp_id,original_length,n);
                        log_message3(log_text,strlen(log_text));
#endif
                        push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),tmp_id,gettime(),F_READ,n+ID_LENGTH,count,n,fd,RECV_ID,buf);
                        errno=tmp_erro;
                        return n;
                    }
                }else{
                    char f_type=-1;
                    if(header_length!=-1){
                        f_type=RECV_HEADFAIL;
                        //                        log_important("note_nohead_read");
                    }else{
                        f_type=RECV_HEADERR;
                    }
#ifdef DEBUG
                    
                    sprintf(log_text,"%sresult:%ld\tempty\n",log_text,header_length);
                    log_event(log_text);
#endif
                    push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),"",gettime(),F_READ,header_length,count,header_length,fd,f_type,NULL);
                    errno=tmp_erro;
                    return header_length;
                }
            }
        }
        
    }
    //    else if(socket_family==AF_UNIX){
    //#ifdef OTHER_SOCK
    //        sprintf(log_text,"%s%s\t",log_text,"AF_UNIX socket");
    //        struct sockaddr_un uin;
    //        struct sockaddr_un uon;
    //        socklen_t s_len = sizeof(uin);
    //        if ((getsockname(fd, (struct sockaddr *)&uin, &s_len) != -1) &&(getpeername(fd, (struct sockaddr *)&uon, &s_len) != -1)){
    //            sprintf(log_text,"%sread from:%s\twith:%s\t read done!\n",log_text,uon.sun_path,uin.sun_path);
    //            log_event(log_text);
    //        }else{
    //            sprintf(log_text,"%s%s\n",log_text,"read done with getsocketname errors!");
    //            log_event(log_text);
    //        }
    //#endif
    //        push_to_database("",1,"",1,getpid(),pthread_self(),"",gettime(),F_READ,0,count,0,fd,DONE_UNIX,"");
    //    }else if(socket_family==UNKNOWN_FAMILY){
    //#ifdef OTHER_SOCK
    //        sprintf(log_text,"%s%s\n",log_text,"read done but errors happend during getting socket family");
    //        log_event(log_text);
    //#endif
    //        push_to_database("",1,"",1,getpid(),pthread_self(),"",gettime(),F_READ,0,count,0,fd,DONE_OTHER,"");
    //    }else{
    //#ifdef OTHER_SOCK
    //        sprintf(log_text,"%s%s\n",log_text,"read done but Unknown socket");
    //        log_event(log_text);
    //#endif
    //        push_to_database("",1,"",1,getpid(),pthread_self(),"",gettime(),F_READ,0,count,0,fd,DONE_OTHER,"");
    //    }
    
    
    return old_read(fd, buf, count);
}

ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,const struct sockaddr *dest_addr, socklen_t addrlen){
    init_context();
    static void *handle = NULL;
    static SENDTO old_sendto = NULL;
    if( !handle )
    {
        handle = dlopen("libc.so.6", RTLD_LAZY);
        old_sendto = (SENDTO)dlsym(handle, "sendto");
    }
    if (!is_socket(sockfd)){
        return old_sendto(sockfd, buf, len,flags,dest_addr,addrlen);
    }
    log_important("in sendto");
    
    return old_sendto(sockfd, buf, len,flags,dest_addr,addrlen);
}

ssize_t sendmsg(int sockfd, const struct msghdr *msg, int flags){
    init_context();
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
    log_event("in sendmsg");
    //    struct msghdr {
    //        void         *msg_name;       /* optional address */
    //        socklen_t     msg_namelen;    /* size of address */
    //        struct iovec *msg_iov;        /* scatter/gather array */
    //        size_t        msg_iovlen;     /* # elements in msg_iov */
    //        void         *msg_control;    /* ancillary data, see below */
    //        size_t        msg_controllen; /* ancillary data buffer len */
    //        int           msg_flags;      /* flags on received message */
    //    };
    return old_sendmsg(sockfd,msg,flags);
    
}

ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,struct sockaddr *src_addr, socklen_t *addrlen){
    init_context();
    static void *handle = NULL;
    static RECVFROM old_recvfrom = NULL;
    if( !handle )
    {
        handle = dlopen("libc.so.6", RTLD_LAZY);
        old_recvfrom = (RECVFROM)dlsym(handle, "recvfrom");
    }
    if(!uuid_key){
        pthread_key_create(&uuid_key,NULL);
    }
    //    log_important("in recvfrom");
    return old_recvfrom (sockfd,buf,len,flags,src_addr,addrlen);
}

ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags){
    init_context();
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
    log_important("in recvmsg");
    
    return old_recvmsg(sockfd,msg,flags);
}

int sendmmsg(int sockfd, struct mmsghdr *msgvec, unsigned int vlen, int flags){
    init_context();
    static void *handle = NULL;
    static SENDMMSG old_sendmmsg = NULL;
    if( !handle )
    {
        handle = dlopen("libc.so.6", RTLD_LAZY);
        old_sendmmsg = (SENDMMSG)dlsym(handle, "sendmmsg");
    }
    if(!uuid_key){
        pthread_key_create(&uuid_key,NULL);
    }
    //    if  ((getsockname(sockfd, (struct sockaddr *)&sin, &s_len) != -1) &&(getpeername(sockfd, (struct sockaddr *)&son, &s_len) != -1)){
    log_important("in sendmmsg\n");
    //        return old_sendmmsg(sockfd,msgvec,vlen,flags);
    //    }
    return old_sendmmsg(sockfd,msgvec,vlen,flags);
    
}
//int recvmmsg(int sockfd, struct mmsghdr *msgvec, unsigned int vlen, int flags, const struct timespec *timeout){
//    init_context();
//    static void *handle = NULL;
//    static RECVMMSG old_recvmmsg = NULL;
//    if( !handle )
//    {
//        handle = dlopen("libc.so.6", RTLD_LAZY);
//        old_recvmmsg = (RECVMMSG)dlsym(handle, "recvmmsg");
//    }
//    if(!uuid_key){
//        pthread_key_create(&uuid_key,NULL);
//    }
//    log_important("in recvmmsg\n");
//    return old_recvmmsg(sockfd,msgvec,vlen,flags,timeout);
//}

ssize_t sendfile(int out_fd, int in_fd, off_t *offset, size_t count){
    init_context();
    static void *handle = NULL;
    static SENDFILE old_sendfile = NULL;
    if( !handle )
    {
        handle = dlopen("libc.so.6", RTLD_LAZY);
        old_sendfile = (SENDFILE)dlsym(handle, "sendfile");
    }
    if(!uuid_key){
        pthread_key_create(&uuid_key,NULL);
    }
    if(is_socket(out_fd)){
        log_important("in sendfile");
    }
    return old_sendfile(out_fd,in_fd,offset,count);
}

ssize_t sendfile64(int out_fd, int in_fd, off64_t *offset, size_t count){
    init_context();
    static void *handle = NULL;
    static SENDFILE64 old_sendfile64 = NULL;
    static WRITE old_write=NULL;
    if( !handle )
    {
        handle = dlopen("libc.so.6", RTLD_LAZY);
        old_sendfile64 = (SENDFILE64)dlsym(handle, "sendfile64");
        old_write=(WRITE)dlsym(handle,"write");
    }
    if(!uuid_key){
        pthread_key_create(&uuid_key,NULL);
    }
    if(is_socket(in_fd)){
        log_important("note_insock_sendfile64");
    }
    if(!is_socket(out_fd)){
        return old_sendfile64(out_fd,in_fd,offset,count);
    }
#ifdef DEBUG
    char log_text[LOG_LENGTH]="init";
    sprintf(log_text,"%s\t","in sendfile64");
#endif
    sa_family_t socket_family=get_socket_family(out_fd);
    if (socket_family==AF_INET){
#ifdef DEBUG
        sprintf(log_text,"%s%s\t",log_text,"AF_INET socket");
#endif
        int tmp_erro=0;
        struct sockaddr_in sin; //local socket info
        struct sockaddr_in son; //remote socket into
        socklen_t s_len = sizeof(sin);
        unsigned short int in_port=0;
        unsigned short int on_port=0;
        char in_ip[256]="0.0.0.0";
        char on_ip[256]="0.0.0.0";
        if(getsockname(out_fd, (struct sockaddr *)&sin, &s_len) != -1){
            char *tmp_in_ip;
            in_port=ntohs(sin.sin_port);
            tmp_in_ip=inet_ntoa(sin.sin_addr);
            memmove(in_ip,tmp_in_ip,strlen(tmp_in_ip)+1);
        }else{
            errno=0;
            log_important("fatal_getsock_write");
#ifdef DEBUG
            sprintf(log_text,"%s%s\t",log_text,"fatal_getsock_write");
#endif
        }
        
        //no connection, actually for recv, no connection now already means error
        if(getpeername(out_fd, (struct sockaddr *)&son, &s_len) != -1){
            char *tmp_on_ip;
            on_port=ntohs(son.sin_port);
            tmp_on_ip=inet_ntoa(son.sin_addr);
            memmove(on_ip,tmp_on_ip,strlen(tmp_on_ip)+1);
        }else{
            errno=0;
            log_important("fatal_getpeer_write");
#ifdef DEBUG
            sprintf(log_text,"%s%s\t",log_text,"fatal_getpeer_write");
#endif
        }
        
#ifdef IP_PORT
        sprintf(log_text,"%swrite to %s:%d with %s:%d\t",log_text,on_ip,on_port,in_ip,in_port);
#endif
        
        //actually this information is currently unused
#ifdef DATA_INFO
        //get socket buff size
        int recv_size = 0;    /* socket接收缓冲区大小 */
        int send_size=0;
        socklen_t optlen;
        optlen = sizeof(send_size);
        if(getsockopt(out_fd, SOL_SOCKET, SO_SNDBUF,&send_size, &optlen)!=0){
            sprintf(log_text,"%s%s\t",log_text,"get socket send buff failed!");
        }
        optlen = sizeof(recv_size);
        if(getsockopt(out_fd, SOL_SOCKET, SO_RCVBUF, &recv_size, &optlen)!=0){
            sprintf(log_text,"%s%s\t",log_text,"get socket recv buff failed!");
        }
#endif
#ifdef DEBUG
        sprintf(log_text,"%ssupposed sendfile64 data length:%zu\tnew data length:%lu\tsendfile64 process:%u\t thread:%lu\t socketid:%d\tfilefd:%d\t",log_text,count,count+ID_LENGTH,getpid(),pthread_self(),out_fd,in_fd);
#endif
        ssize_t n=0;
        if(check_filter(on_ip,in_ip,on_port,in_port)==0){
            n=old_sendfile64(out_fd,in_fd,offset,count);
            tmp_erro=errno;
#ifdef FILTER
            sprintf(log_text,"%sreturn:%ld\t%s\n",log_text,n,"sendfile64 done with filter!");
            log_event(log_text);
#endif
            if((on_port==80)||(in_port==80)){
                errno=tmp_erro;
                return n;
            }
            push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),"",gettime(),F_SEND64,n,count,n,out_fd,SEND_FILTER,NULL);
            errno=tmp_erro;
            return n;
        }
        
        if(with_uuid==0){//no uuid, write this message
            
            n=old_sendfile64(out_fd,in_fd,offset,count);
            tmp_erro=errno;
#ifdef DEBUG
            sprintf(log_text,"%sresult:%ld\t%s\n",log_text,n,"sendfile done without uuid!");
            log_event(log_text);
#endif
            push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),"",gettime(),F_SEND64,n,count,n,out_fd,SEND_NORMALLY,NULL);
            errno=tmp_erro;
            return n;
        }
        char id[ID_LENGTH];
        random_uuid(id,count);
        
        int flag=0;
        long long ttime;
        ttime=gettime();
        n=old_write(out_fd, id, ID_LENGTH);
        tmp_erro=errno;
#ifdef DEBUG
        sprintf(log_text,"%sfirst err:%d\tfirst write:%ld\t",log_text,tmp_erro,n);
#endif
        if(n==ID_LENGTH){
            flag=1;
        }else if(n==0){
            flag=0;
        }else{
            //                    log_event(" sendfile6");
            tmp_erro=errno;
            while((n!=ID_LENGTH)&&((tmp_erro==0)||(tmp_erro==EAGAIN)||(tmp_erro==EWOULDBLOCK))){
                ssize_t tmpValue=0;
                if(n!=-1){
                    tmpValue=old_write(out_fd, &id[n], ID_LENGTH-n);
                }else{
                    tmpValue=old_write(out_fd, &id[0], ID_LENGTH);
                }
                tmp_erro=errno;
                if(tmpValue>=0){
                    n+=tmpValue;
                }
                //                log_message("iteration",10,"sendfile64_write");
                //#ifdef DEBUG
                //                sprintf(log_text,"%siteration\terrno:%d\t",log_text,tmp_erro);
                //#endif
            }
            if(n==ID_LENGTH){
                flag=1;
            }else if(n==-1){
                flag=0;
            }else if((n<=ID_LENGTH)&&(n>=1)){
                flag=0;
                log_important("fatal_write_short_sendfile64");
            }else{
                flag=0;
                log_important("fatal_write_noconnection_sendfile64");
            }
            
        }
        
        if(flag==0){
#ifdef DEBUG
            sprintf(log_text,"%sid_flag:%d\terrno:%d\tlength:%ld\tsendfile failed!",log_text,flag,tmp_erro,n);
            log_event(log_text);
#endif
            push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),"",gettime(),F_SEND64,n,count,n,out_fd,SEND_ERROR,NULL);
            errno=tmp_erro;
            return n;
        }
        
        //now start to send real data
        char tmp_id[ID_LENGTH];
        format_uuid(id,tmp_id);
        char f_type=-1;
        errno=0;
        n=old_sendfile64(out_fd,in_fd,offset,count);
        tmp_erro=errno;
#ifdef DEBUG
        sprintf(log_text,"%sfirst err:%d\tfirst sendfile64:%ld\t",log_text,tmp_erro,n);
#endif
        ssize_t rvalue=0;
        if(n==count){
            f_type=SEND_ID;
            rvalue=count;
        }else if(n==0){
            f_type=SEND_DISCONN;
            rvalue=0;
        }else{
            while((n!=count)&&((tmp_erro==0)||(tmp_erro==EAGAIN)||(tmp_erro==EWOULDBLOCK))){
                ssize_t tmpValue=0;
                if(n!=-1){
                    tmpValue=old_sendfile64(out_fd,in_fd,offset,count-n);//suppose senfile will set offset automatically. autually we should check whether it's null first
                }else{
                    tmpValue=old_sendfile64(out_fd,in_fd,offset,count);
                }
                log_important("note_offset_sendfile64");
                ssize_t
                tmp_erro=errno;
                if(tmpValue>=0){
                    n+=tmpValue;
                }
                //#ifdef DEBUG
                //                sprintf(log_text,"%siteration\terrno:%d\t",log_text,errno);
                //#endif
                //                log_message("iteration",10,"sendfile64");
            }
            if(n==count){
                f_type=SEND_ID;
                rvalue=count;
            }else if(n==-1){
                f_type=SEND_ERROR;
                rvalue=-1;
                log_important("fatal_short1_sendfile64");
            }else if((n<=count)&&(n>=1)){
                f_type=SEND_BROKEN;
                rvalue=-1;
                log_important("fatal_short2_sendfile64");// should deal with this like how we deal with unfinished recv
                //                return n-39;
            }else{
                f_type=SEND_OTHER;
                rvalue=-1;
                log_important("fatal_short2_sendfile64");// should deal with this like how we deal with unfinished recv
            }
        }
#ifdef DEBUG
        sprintf(log_text,"%suuid:%s\terrno:%d\tn:%ld\treturn:%ld\n",log_text,tmp_id,tmp_erro,n,rvalue);
        log_event(log_text);
#endif
        push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),tmp_id,gettime(),F_SEND64,n,count,rvalue,out_fd,f_type,NULL);
        if(n!=count){
            errno=tmp_erro;
        }
        return rvalue;
        
    }
    
    return old_sendfile64(out_fd,in_fd,offset,count);
}

//ssize_t copy_file_range(int fd_in, loff_t *off_in,int fd_out, loff_t *off_out,size_t len, unsigned int flags){
//    if (is_socket(fd_in) || is_socket(fd_out)){
//        log_event("in copy");
//    }
//}

ssize_t readv(int fd, const struct iovec *iov, int iovcnt){
    init_context();
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
        
    }
    log_important("in readv\n");
    return old_readv(fd,iov,iovcnt);
    
}
ssize_t writev(int fd, const struct iovec *iov, int iovcnt){
    init_context();
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
    if (!is_socket(fd)){
        return old_writev(fd,iov,iovcnt);
    }
#ifdef DEBUG
    char log_text[LOG_LENGTH]="init";
    sprintf(log_text,"%s\t","in writev");
#endif
    sa_family_t socket_family=get_socket_family(fd);
    if(socket_family==AF_INET){
#ifdef DEBUG
        sprintf(log_text,"%s%s\t",log_text,"AF_INET socket");
#endif
        int tmp_erro=0;
        struct sockaddr_in sin; //local socket info
        struct sockaddr_in son; //remote socket into
        socklen_t s_len = sizeof(sin);
        unsigned short int in_port=0;
        unsigned short int on_port=0;
        char in_ip[256]="0.0.0.0";
        char on_ip[256]="0.0.0.0";
        if(getsockname(fd, (struct sockaddr *)&sin, &s_len) != -1){
            char *tmp_in_ip;
            in_port=ntohs(sin.sin_port);
            tmp_in_ip=inet_ntoa(sin.sin_addr);
            memmove(in_ip,tmp_in_ip,strlen(tmp_in_ip)+1);
        }else{
            errno=0;
            log_important("fatal_getsock_write");
#ifdef DEBUG
            sprintf(log_text,"%s%s\t",log_text,"fatal_getsock_write");
#endif
        }
        
        //no connection, actually for recv, no connection now already means error
        if(getpeername(fd, (struct sockaddr *)&son, &s_len) != -1){
            char *tmp_on_ip;
            on_port=ntohs(son.sin_port);
            tmp_on_ip=inet_ntoa(son.sin_addr);
            memmove(on_ip,tmp_on_ip,strlen(tmp_on_ip)+1);
        }else{
            errno=0;
            log_important("fatal_getpeer_write");
#ifdef DEBUG
            sprintf(log_text,"%s%s\t",log_text,"fatal_getpeer_write");
#endif
        }
        
#ifdef IP_PORT
        sprintf(log_text,"%swrite to %s:%d with %s:%d\t",log_text,on_ip,on_port,in_ip,in_port);
#endif
        
        //actually this information is currently unused
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
        size_t total=0;
        struct iovec myiov[iovcnt];
        int i=0;
        for(;i<iovcnt;i++){
            total+=iov[i].iov_len;
            myiov[i].iov_len=iov[i].iov_len;
            myiov[i].iov_base=iov[i].iov_base;
        }
        size_t new_total=total+ID_LENGTH;
        ssize_t n=0;
#ifdef DEBUG
        sprintf(log_text,"%ssupposed writev data length:%zu\tnew data length:%lu\twrite process:%u\t thread:%lu\t socketid:%d\t",log_text,total,new_total,getpid(),pthread_self(),fd);
#endif
        if(check_filter(on_ip,in_ip,on_port,in_port)==0){
            
            n=old_writev(fd,iov,iovcnt);
            tmp_erro=errno;
#ifdef FILTER
            sprintf(log_text,"%sreturn:%ld\t%s\n",log_text,n,"writev done with filter!");
            log_event(log_text);
#endif
            if((on_port==80)||(in_port==80)){
                errno=tmp_erro;
                return n;
            }
            push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),"",gettime(),F_WRITEV,n,total,n,fd,SEND_FILTER,NULL);
            errno=tmp_erro;
            return n;
        }
        
        if(with_uuid==0){//no uuid, write this message
            
#ifdef MESSAGE
            log_message( (char*)buf,count,"r_writev");
#endif
            n=old_writev(fd,iov,iovcnt);
            tmp_erro=errno;
#ifdef DEBUG
            sprintf(log_text,"%sresult:%ld\t%s\n",log_text,n,"writev done without uuid!");
            log_event(log_text);
#endif
            push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),"",gettime(),F_WRITEV,n,total,n,fd,SEND_NORMALLY,NULL);
            errno=tmp_erro;
            return n;
        }else{
            size_t new_len=myiov[0].iov_len+ID_LENGTH;
            char target[new_len];
            char id[ID_LENGTH];
            random_uuid(id,total);
            char tmp_id[ID_LENGTH];
            format_uuid(id,tmp_id);
            memmove(&target,id,ID_LENGTH);
            memmove(&target[ID_LENGTH],myiov[0].iov_base,iov[0].iov_len);
            myiov[0].iov_len=new_len;
            myiov[0].iov_base=&target;
            errno=0;
            ssize_t n=old_writev(fd,myiov,iovcnt);
            tmp_erro=errno;
            errno=0;
#ifdef DEBUG
            
            sprintf(log_text,"%s\tuuid:%s\tarray:%d\ttotal:%lu\treal writev:%ld\treturn:%ld\t",log_text,tmp_id,iovcnt,total,n,n-ID_LENGTH);
#endif
            if(n!=new_total){
                //should deal with this situation
#ifdef DEBUG
                sprintf(log_text,"%s\tfailed!\n",log_text);
                log_event(log_text);
#endif
                log_important("fatal_short_writev");
                push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),tmp_id,gettime(),F_WRITEV,n,total,n-ID_LENGTH,fd,SEND_FAIL,NULL);
                errno=tmp_erro;
                return n-ID_LENGTH;
            }else{
#ifdef DEBUG
                sprintf(log_text,"%s\tsuccess!\n",log_text);
                log_event(log_text);
#endif
                push_to_database(on_ip,on_port,in_ip,in_port,getpid(),pthread_self(),tmp_id,gettime(),F_WRITEV,n,total,total,fd,SEND_ID,NULL);
                errno=tmp_erro;
                return total;
            }
        }
        
        
    }
    
    return old_writev(fd,iov,iovcnt);
    
}

//long syscall(long number, ...){
//    static void *handle = NULL;
//    static SYSCALL old_syscall = NULL;
//    if( !handle )
//    {
//        handle = dlopen("libc.so.6", RTLD_LAZY);
//        old_syscall = (SYSCALL)dlsym(handle, "syscall");
//    }
//    if(!uuid_key){
//        pthread_key_create(&uuid_key,NULL);
//    }
//    char tmp[1024]="";
//    sprintf(tmp,"%s\tin syscall\tcall number:%ld",tmp,number);
//    log_event(tmp);
//    va_list va;
//    va_start(va, number);
//    long result=old_syscall(number,va);
//    va_end(va);
//    sprintf(tmp,"%s\tresult:%ld\n",tmp,result);
//    return result;
//}

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
    init_context();
    static void *handle = NULL;
    static P_CREATE old_create=NULL;
    if( !handle )
    {
        handle = dlopen("libpthread.so.0", RTLD_LAZY);
        old_create = (P_CREATE)dlsym(handle, "pthread_create");
    }
    
    pthread_t tmp=pthread_self();
    
    //for log
    //    int i=0;
    //    for(;environ[i]!=NULL;i++){}
    //    char test[1024]="";
    //    sprintf(test,"create pid:%d\tptid:%ld\ttid:%lu,key:%u\t,value:%s\tenv:%s\tenv str:%s\t",getpid(),syscall(SYS_gettid),tmp,uuid_key,(char *)pthread_getspecific(uuid_key),getenv(idenv),environ[i-1]);
    
    //    if(uuid_key==12345){
    //        pthread_key_create(&uuid_key,NULL);
    //        char * tmp_env=(char *)malloc(1024); //never free
    //        int env_null=get_own_env(tmp_env);
    //        if(!env_null){
    //            pthread_setspecific(uuid_key,tmp_env);
    //        }
    //    }
    
    
    
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
    
    //        push_thread_db(getpid(),*thread,tmp,0,gettime());
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
    init_context();
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
    
    char *uuid=(char *)pthread_getspecific(uuid_key);
    char parameter[1024]="";
    sprintf(parameter,"ppid=%d&pktid=%ld&ptid=%lu&ttime=%lld&rtype=%d&",getpid(),syscall(SYS_gettid),pthread_self(),gettime(),R_THREAD);
    
    pid_t result=old_fork();
    if(result==0){
        sprintf(parameter,"%spid=%d&ktid=%ld&tid=%lu",parameter,getpid(),syscall(SYS_gettid),pthread_self());
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
    init_context();
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
    
    char *uuid=(char *)pthread_getspecific(uuid_key);
    char parameter[1024]="";
    sprintf(parameter,"ppid=%d&pktid=%ld&ptid=%lu&ttime=%lld&rtype=%d&",getpid(),syscall(SYS_gettid),pthread_self(),gettime(),R_THREAD);
    
    
    pid_t result=old_fork();
    if(result==0){
        sprintf(parameter,"%spid=%d&ktid=%ld&tid=%lu",parameter,getpid(),syscall(SYS_gettid),pthread_self());
        pthread_setspecific(uuid_key, (void *)uuid);
        //        log_message(parameter,strlen(parameter));
        push_thread_db2(parameter);
        //        push_thread_db(getpid(),syscall(SYS_gettid),pthread_self(),ppid,pktid,ptid,gettime());
        //        sprintf(test,"%s\tnew pid:%d\tptid:%ld\ttid:%lu, key:%uvalue:%s\n",test,getpid(),syscall(SYS_gettid),pthread_self(),uuid_key,(char*)pthread_getspecific(uuid_key));
        //        log_message(test,strlen(test));
    }
    return result;
}

int execl(const char *path, const char *arg, ...){
    //    char test[1014]="";
    //    sprintf(test,"in execl pid:%d\tptid:%ld\ttid:%lu\n",getpid(),syscall(SYS_gettid),pthread_self());
    //    log_message(test,strlen(test));
    log_important("in execl");
    return 1;
}

int execlp(const char *file, const char *arg, ...){
    //    char test[1014]="";
    //    sprintf(test,"in execlp pid:%d\tptid:%ld\ttid:%lu\n",getpid(),syscall(SYS_gettid),pthread_self());
    //    log_message(test,strlen(test));
    log_important("in execlp");
    return 1;
}
// int execlpe(const char *file, const char *arg0, ..., const char *argn, (char *)0, char *const envp[]);

// int execle(const char *path, const char *arg, ..., char *const envp[]){
//     char test[1014]="";
//     sprintf(test,"in execle pid:%d\tptid:%ld\ttid:%lu\n",getpid(),syscall(SYS_gettid),pthread_self());
//     log_message(test,strlen(test));
//     log_important("in execle");
//     return 1;
// }

int execv(const char *path, char *const argv[]){
    //    char test[1014]="";
    //    sprintf(test,"in execv pid:%d\tptid:%ld\ttid:%lu\n",getpid(),syscall(SYS_gettid),pthread_self());
    //    log_message(test,strlen(test));
    log_important("in execv");
    return 1;
}
//
int execve(const char *path, char *const argv[], char *const envp[]){
    init_context();
    char *unit_uuid=(char *)pthread_getspecific(uuid_key);
    setenv(idenv,unit_uuid,1);
    static void *handle = NULL;
    static EXECVE old_execve=NULL;
    if( !handle )
    {
        handle = dlopen("libc.so.6", RTLD_LAZY);
        old_execve = (EXECVE)dlsym(handle, "execve");
        
    }
    return old_execve(path,argv,envp);

    
}

int execvp(const char *file, char *const argv[]){
    init_context();
    char *unit_uuid=(char *)pthread_getspecific(uuid_key);
    setenv(idenv,unit_uuid,1);
    static void *handle = NULL;
    static EXECVP old_execvp=NULL;
    if( !handle )
    {
        handle = dlopen("libc.so.6", RTLD_LAZY);
        old_execvp = (EXECVP)dlsym(handle, "execvp");
        
    }
    return old_execvp(file,argv);
    

    
}

int execvpe(const char *file, char *const argv[], char *const envp[]){
    //    char test[4096]="";
    //    sprintf(test,"in execvpe file:%s\tpid:%d\tptid:%ld\ttid:%lu\tenvtest:%s\tTLD:%s\n",file,getpid(),syscall(SYS_gettid),pthread_self(),getenv(idenv),(char*)pthread_getspecific(uuid_key));
    //    log_message(test,strlen(test));
    
    static void *handle = NULL;
    static EXECVPE old_execvpe=NULL;
    if( !handle )
    {
        handle = dlopen("libc.so.6", RTLD_LAZY);
        old_execvpe = (EXECVPE)dlsym(handle, "execvpe");
    }
    log_important("in execvpe");
    return old_execvpe(file,argv,envp);
}

//int fputs(const char *str, FILE *stream){
//    static void *handle = NULL;
//    static FPUTS old_fputs=NULL;
//    if( !handle )
//    {
//        handle = dlopen("libc.so.6", RTLD_LAZY);
//        old_fputs = (FPUTS)dlsym(handle, "fputs");
//    }
//    char filename[1024]="";
//    get_filename(stream,filename);
//    int result=old_fputs(str,stream);
//    int tmp_erro=errno;
//
////    char tmp[2046]="";
////    sprintf(tmp,"in fputs:\tfilename:%s\tresult:%d\n",filename,result);
////    log_message(tmp,strlen(tmp),"fputs");
//
//    errno=tmp_erro;
//    return result;
//}

//int fprintf(FILE *stream, const char *format, ...){
//    static void *handle = NULL;
//    static FPRINTF old_fprintf=NULL;
//    if( !handle )
//    {
//        handle = dlopen("libc.so.6", RTLD_LAZY);
//        old_fprintf = (FPRINTF)dlsym(handle, "fprintf");
//    }
//    log_message("hehe",5,"fprintf");
//
//    char filename[1024]="";
//    get_filename(stream,filename);
//
//    va_list va;
//    va_start(va, format);
//    int result=old_fprintf(stream,format,va);
//    int tmp_erro=errno;
//    va_end(va);
//
//    char tmp[2046]="";
//    sprintf(tmp,"in fprintf:\tfilename:%s\tresult:%d\n",filename,result);
//    log_message(tmp,strlen(tmp),"fprintf");
//
//    errno=tmp_erro;
//    return result;
//}

//size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream){
//    static void *handle = NULL;
//    static FWRITE old_fwrite=NULL;
//    if( !handle )
//    {
//        handle = dlopen("libc.so.6", RTLD_LAZY);
//        old_fwrite = (FWRITE)dlsym(handle, "fwrite");
//    }
//
//    char filename[1024]="";
//    get_filename(stream,filename);
//
//    size_t result=old_fwrite(ptr,size,nmemb,stream);
//    int tmp_erro=errno;
//
//    char tmp[2046]="";
//    sprintf(tmp,"in fwrite:\tfilename:%s\tresult:%lu\n",filename,result);
//    log_message(tmp,strlen(tmp),"fwrite");
//
//    errno=tmp_erro;
//    return result;
//}

int get_filename(FILE * f,char* filename) {
    int fd;
    char fd_path[255];
    //    char * filename = malloc(255);
    ssize_t n;
    
    fd = fileno(f);
    sprintf(fd_path, "/proc/self/fd/%d", fd);
    n = readlink(fd_path, filename, 255);
    if (n < 0)
        return 0;
    filename[n] = '\0';
    return 1;
}

int get_filename2(int fd,char* filename) {
    char fd_path[255];
    //    char * filename = malloc(255);
    ssize_t n;
    
    sprintf(fd_path, "/proc/self/fd/%d", fd);
    n = readlink(fd_path, filename, 255);
    if (n < 0)
        return 0;
    filename[n] = '\0';
    return 1;
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
    if (!tmp){

        return 0;
    }else{

        memmove(env,tmp,ID_LENGTH);
        return 1;
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
    //    struct sockaddr_storage tt_on;
    struct sockaddr *t_in=(struct sockaddr *)&tt_in;
    //    struct sockaddr *t_on=(struct sockaddr *)&tt_on;
    socklen_t s_len = sizeof(tt_in);
    //    if ((getsockname(sockfd, t_in, &s_len) != -1) &&(getpeername(sockfd, t_on, &s_len) != -1))
    //    {
    //        return t_in->sa_family;
    //    }else{
    //        return UNKNOWN_FAMILY;
    //    }
    if (getsockname(sockfd, t_in, &s_len) != -1)
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

size_t op_storage(int type,int sockfd,size_t left){
    
    static struct storage* buffer_storage[S_SIZE];
    static short initialized=0;
    static long counter=0;
    
    if(type==S_PUT){//put
        if(initialized==0){
            int i;
            for(i=0;i<S_SIZE;i++){
                buffer_storage[i]=malloc(sizeof(struct storage));
                buffer_storage[i]->sockfd=0;
                buffer_storage[i]->used=0;
                buffer_storage[i]->left=0;
            }
            initialized=1;
            counter+=20*13;
            //            char tmp[256]="";
            //            sprintf(tmp,"counter%ld\n",counter);
            //            log_event("haha");
            //            log_event(tmp);
        }
        
        int  i;
        for(i=0;i<S_SIZE;i++){
            if (buffer_storage[i]->used==0){
                buffer_storage[i]->used=1;
                buffer_storage[i]->sockfd=sockfd;
                buffer_storage[i]->left=left;
                //                log_message("save",5,"operate");
                //                char tmp[1024]="";
                //                sprintf(tmp,"socket:%d\tput:%ld!\n",sockfd,left);
                //                log_message3(tmp,strlen(tmp));
                return 0;
            }
        }
        log_important("fatal_nospace_storage");
        return 1;
        
    }else if(type==S_GET){//get
        if(initialized==0){
            return 0;
        }
        int i;
        //        char tmp[1024]="";
        //        sprintf(tmp,"socket:%d\tget\t",sockfd);
        for(i=0;i<S_SIZE;i++){
            if(buffer_storage[i]->sockfd==sockfd){
                //                sprintf(tmp,"%s\tfound:%d!",tmp,i);
                size_t result=buffer_storage[i]->left;
                buffer_storage[i]->sockfd=0;
                buffer_storage[i]->used=0;
                buffer_storage[i]->left=0;
                //                sprintf(tmp,"%s\treturn:%lu\n!",tmp,result);
                //                log_message("load",5,"operate");
                //                log_message3(tmp,strlen(tmp));
                return result;
            }
        }
        //        sprintf(tmp,"%s\t not found!\n",tmp);
        //        log_message3(tmp,strlen(tmp));
        return 0;
        
    }else if(type==S_RELEASE){//release
        int i;
        for(i=0;i<S_SIZE;i++){
            if(buffer_storage[i]->sockfd==sockfd){
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

int check_storage(char * buf){
    int len=(int)buf[0];
    int i=0;
    int flag=0;
    if(len==(ID_LENGTH-1)){
        for(i=3;i<(len-1);i++){
            if(((buf[i]>=97)&&(buf[i]<=122))||((buf[i]>=48)&&(buf[i]<=57))||(buf[i]=='-')){
                flag=1;
            }else{
                flag=0;
                break;
            }
        }
        if ((flag==1)&&(buf[1]=='@')&&(buf[2]=='@')&&(buf[len-1]=='@')&&(buf[len]=='@')){
            return 1;
        }else{
            return 0;
        }
    }else{
        if (len>=2){
            for(i=3;i<=len;i++){
                if(((buf[i]>=97)&&(buf[i]<=122))||((buf[i]>=48)&&(buf[i]<=57))||(buf[i]=='-')||(buf[i]=='@')){
                    flag=1;
                }else{
                    flag=0;
                    break;
                }
                
            }
            if((flag==1)&&(buf[1]=='@')&&(buf[2]=='@')){
                return 1;
            }else{
                return 0;
            }
        }else{
            if(buf[1]=='@'){
                return 1;
            }else{
                return 0;
            }
        }
        
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

//// to start post and return the response
//int getresponse(char* post_parameter){
//    int tmp=errno;
//    CURL *curl;
//    CURLcode res;
//
//    curl = curl_easy_init();
//    if(curl) {
//        struct string s;
//        init_string(&s);
//
//        curl_easy_setopt(curl, CURLOPT_URL, controller_service);
//        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_parameter);
//        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
//        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
//        res = curl_easy_perform(curl);
//
//        int result_flag;
//        //printf("%s\n", s.ptr);
//        if(strcmp(s.ptr, "0") == 0){
//            result_flag=0;
//        }else{
//            result_flag=1;
//
//        }
//
//        free(s.ptr);
//
//        /* always cleanup */
//        curl_easy_cleanup(curl);
//        errno=tmp;
//        return result_flag;
//    }
//}

// write the message to the logfile
int getresponse(char* post_parameter){
    static void *handle = NULL;
    static FPRINTF old_fprintf=NULL;
    if( !handle )
    {
        handle = dlopen("libc.so.6", RTLD_LAZY);
        old_fprintf = (FPRINTF)dlsym(handle, "fprintf");
    }
    FILE *logFile;
    if((logFile=fopen(dataFilePath,"a+"))!=NULL){
        int result =old_fprintf(logFile,"%s\n", post_parameter);
        if (result!=(strlen(post_parameter)+1)){
            log_important("fatal_short_record");
        }
        fclose(logFile);
        return 1;
    }else{
        return -1;
        
    }
}

/**
 * Create random UUID
 *
 * @param buf - buffer to be filled with the uuid string
 */
char *random_uuid( char buf[ID_LENGTH],size_t len )
{
    uuid_t uuid;
    uuid_generate(uuid);
    uuid_unparse(uuid, buf);
    
    //    log_message(buf,ID_LENGTH,"id1");
    buf[ID_LENGTH-1]='\0';
    
    //    char tmp[ID_LENGTH];
    //    memmove(tmp,buf,ID_LENGTH);
    //    int i=1;
    //    for(;i<(1+sizeof(len));i++){
    //        tmp[i]='-';
    //    }
    
    //    char tmplog[1024]="";
    //    sprintf(tmplog,"uuid:%s\tmarshal:%lu\n",tmp,len);
    //    log_message(tmplog,strlen(tmplog),"random");
    //    }
    
    int i=1;
    for(;i<(1+sizeof(len));i++){
        buf[i]='-';
    }
    //    log_message(buf,ID_LENGTH,"id2");
    
    buf[0]='@';
    //    buf[1]=ID_LENGTH;//note. ID_LENGTH should be below 127
    if(len!=0){//len=0 indicates it's for unit_id
        memmove(&buf[1],&len,sizeof(len));
    }
    buf[ID_LENGTH-2]='@';
    //    log_message(buf,ID_LENGTH,"id3");
    return buf;
}

int format_uuid(char uuid[ID_LENGTH],char format_uuid[ID_LENGTH]){
    memmove(format_uuid,uuid,ID_LENGTH);
    size_t len=0;
    int i=1;
    for(;i<(1+sizeof(len));i++){
        format_uuid[i]='-';
    }
    return 1;
    
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

//wether a file descriptor is a regular file
int is_file(int fd){
    struct stat statbuf;
    fstat(fd, &statbuf);
    if (S_ISREG(statbuf.st_mode)){
        return 1;
    }else{
        return 0;
    }
}

//whether it's writing to a
int check_log(int fd,size_t count){
    struct stat statbuf;
    fstat(fd, &statbuf);
    if (S_ISREG(statbuf.st_mode)){
        char filename[1024]="";
        get_filename2(fd,filename);
        if(strstr(filename,logRule)&&(count>1)){//found
            return 1;
        }else{
            return 0;
        }
        
    }else{
        return 0;
    }
}
//only for importance information which usually indicates an error we didn't deal with
void log_important(char* info){
    static void *handle = NULL;
    static FPRINTF old_fprintf=NULL;
    if( !handle )
    {
        handle = dlopen("libc.so.6", RTLD_LAZY);
        old_fprintf = (FPRINTF)dlsym(handle, "fprintf");
    }
    
    int tmp_erro=errno;
    FILE *errLogFile;
    if((errLogFile=fopen(errFilePath,"a+"))!=NULL){
        old_fprintf(errLogFile,"%s\n", info);
        fclose(errLogFile);
        
    }else{
        printf("Cannot open file!");
        
    }
    errno=tmp_erro;
}

void log_event(char* info){
    static void *handle = NULL;
    static FPRINTF old_fprintf=NULL;
    if( !handle )
    {
        handle = dlopen("libc.so.6", RTLD_LAZY);
        old_fprintf = (FPRINTF)dlsym(handle, "fprintf");
    }
    
    int tmp=errno;
    FILE *logFile;
    if((logFile=fopen(filePath,"a+"))!=NULL){
        old_fprintf(logFile,"%s\n", info);
        fclose(logFile);
        
    }else{
        printf("Cannot open file!");
        
    }
    errno=tmp;
    
}

int log_message3(char message[],size_t length){
    //    int tmp=errno;
    if(length<1){
        return 0;
    }
    if(length>1000){
        return 0;
    }
    static void *handle = NULL;
    static FPRINTF old_fprintf=NULL;
    if( !handle )
    {
        handle = dlopen("libc.so.6", RTLD_LAZY);
        old_fprintf = (FPRINTF)dlsym(handle, "fprintf");
    }
    
    FILE *logFile;
    char result[LOG_LENGTH]="";
    
    if((logFile=fopen("/tmp/tracelog3.txt","a+"))!=NULL){
        sprintf(result,"message:\tpid:%d\tktid:%ld\ttid:%lu\t",getpid(),syscall(SYS_gettid),pthread_self());
        
        int i=0;
        //        if(length>ID_LENGTH){
        //            i=length-10; //only show the last ten char
        //        }
        for(;i<length;i++){
            sprintf(result,"%s%c",result,message[i]);
            //            sprintf(result,"%s%d-",result,message[i]);
        }
        sprintf(result,"%s%c",result,'\0');
        old_fprintf(logFile,"%s\n", result);
        fclose(logFile);
    }else{
        printf("Cannot open file!");
    }
    //    errno=tmp;
    return 0;
}

void log_message(char message[],int length, const char * flag){
    //    int tmp=errno;
    //#ifndef MESSAGE
    //    return;
    //#endif
    if(length<1){
        return;
    }
    //    if(length>1000){
    //        length=600;
    ////        return;
    //    }
    static void *handle = NULL;
    static FPRINTF old_fprintf=NULL;
    if( !handle )
    {
        handle = dlopen("libc.so.6", RTLD_LAZY);
        old_fprintf = (FPRINTF)dlsym(handle, "fprintf");
    }
    
    
    FILE *logFile;
    char result[LOG_LENGTH]="";
    
    if((logFile=fopen(debugFilePath,"a+"))!=NULL){
        sprintf(result,"message:\t%s\t",flag);
        
        int i=0;
        //        if(length>ID_LENGTH){
        //            i=length-10; //only show the last ten char
        //        }
        for(;i<length;i++){
            sprintf(result,"%s%c",result,message[i]);
            //            sprintf(result,"%s%d-",result,message[i]);
        }
        sprintf(result,"%s%c",result,'\0');
        old_fprintf(logFile,"%s\n", result);
        fclose(logFile);
    }else{
        printf("Cannot open file!");
    }
    //    errno=tmp;
    
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



//return 1, this message contains a job_id
//return 0, this message doesn't contain a job_id
int find_job(char* content, const char *message,int length){
    
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

void translate(char * content,const char *message,int length){
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

//call this at the staring poing of every function
//to init the unit_id
void init_context(){
    if(uuid_key==12345){//new or after exec
        pthread_key_create(&uuid_key,NULL);//actually we should release the pointer when the thread exits
        char * tmp_env=malloc(ID_LENGTH);
        if(!get_own_env(tmp_env)){// check from the environment virable list
            random_uuid(tmp_env,0);
        }
        pthread_setspecific(uuid_key,tmp_env);
        
    }else{//the context already existed
        void *context=pthread_getspecific(uuid_key);
        if(!context){
            char* tmp_env=malloc(ID_LENGTH);
            random_uuid(tmp_env,0);
            pthread_setspecific(uuid_key,(void *)tmp_env);
        }
    }
}


//type=24
int push_to_database(char* on_ip,int on_port,char* in_ip, int in_port,pid_t pid,pthread_t tid,char* uuid,long long time,char ftype,long length,long supposed_length,long rlength,int socketid,char dtype,const char * message){
    
    char * original_unit=(char *)pthread_getspecific(uuid_key);
    char * unit_uuid=malloc(ID_LENGTH);
    if ((ftype%2)==0){//recv
        if (uuid!=""){
            random_uuid(unit_uuid,0);
            //free last context in the future
            //            free(original_unit);
            pthread_setspecific(uuid_key,(void *)unit_uuid);
            //            unit_uuid=uuid;
        }else{
            memmove(unit_uuid,original_unit,ID_LENGTH);

        }
    }else{//send
        if (uuid!=""){
            memmove(unit_uuid,uuid,ID_LENGTH);
            //            free(original_unit);
            pthread_setspecific(uuid_key,(void *)unit_uuid);

        }else{
            //            unit_uuid=(char *)pthread_getspecific(uuid_key);
            memmove(unit_uuid,original_unit,ID_LENGTH);
        }
    }

    
    char * parameter_tmp="on_ip=%s&on_port=%d&in_ip=%s&in_port=%ld&pid=%u&ktid=%ld&tid=%lu&uuid=%s&unit_uuid=%s&ttime=%lld&ftype=%d&length=%ld&supposed_length=%ld&rlength=%ld&rtype=%d&socket=%d&dtype=%d&message=%s&message2=%s";
    char parameter[4096];
    sprintf(parameter,parameter_tmp,on_ip,on_port,in_ip,in_port,pid,syscall(SYS_gettid),tid,uuid,unit_uuid,time,ftype,length,supposed_length,rlength,R_DATABASE,socketid,dtype,"","");
    //    free(tmpMsg);
    //    free(tmpMsg2);
    
    return getresponse(parameter);
    //    return 0;
}

//type=27
int push_thread_db(pid_t pid,pid_t ktid,pthread_t tid,pid_t ppid,pid_t pktid, pthread_t ptid,long long time){
    // To do
    char * parameter_tmp="pid=%ld&ktid=%ld&tid=%lu&ppid=%ld&pktid=%ld&ptid=%lu&ttime=%lld&rtype=%d";
    char parameter[1024];
    sprintf(parameter,parameter_tmp,pid,ktid,tid,ppid,pktid,ptid,time,R_THREAD);
    //    log_message(parameter,strlen(parameter));
    return getresponse(parameter);
    //    return 0;
}
//type=28
int push_thread_dep(pid_t pid,pid_t ktid,pthread_t dtid,pthread_t jtid,long long time){
    char * parameter_tmp="pid=%ld&ktid=%ld&dtid=%lu&jtid=%lu&ttime=%lld&rtype=%d";
    char parameter[1024];
    sprintf(parameter,parameter_tmp,pid,ktid,dtid,jtid,time,R_THREAD_DEP);
    //    log_message(parameter,strlen(parameter));
    return getresponse(parameter);
    //    return 0;
    
}

//type=27
int push_thread_db2(char *parameter){
    // To do
    //        char * parameter_tmp="pid=%ld&ktid=%ld&tid=%lu&ppid=%ld&pktid=%ld&ptid=%lu&ttime=%lld&rtype=%d";
    //        char parameter[1024];
    //        sprintf(parameter,parameter_tmp,pid,ktid,tid,ppid,pktid,ptid,time,R_THREAD);
    //        log_message(parameter,strlen(parameter));
    return getresponse(parameter);
    //    return 0;
}



////type=25
//int mark_socket_connect(int socketid){
//    char * parameter_tmp="uuid=%s&rtype=%d&socket=%d";
//    char parameter[1024];
//    sprintf(parameter,parameter_tmp,"socket",25,socketid);
//    return getresponse(parameter);
//}
////
////type=26
//int mark_socket_close(int socketid){
//    char * parameter_tmp="uuid=%s&rtype=%d&socket=%d";
//    char parameter[1024];
//    sprintf(parameter,parameter_tmp,"socket",26,socketid);
//    return getresponse(parameter);
//}



ssize_t check_read_header(char *uuids,int sockfd,size_t* length,int flags){
    static void *handle = NULL;
    static RECV old_recv = NULL;
    if( !handle )
    {
        handle = dlopen("libc.so.6", RTLD_LAZY);
        old_recv = (RECV)dlsym(handle, "recv");
    }
    //    ssize_t rvalue=0;
    ssize_t n=0;
    int tmp_erro=0;
    n=old_recv(sockfd,uuids,ID_LENGTH,flags);
    tmp_erro=errno;
    if((n!=-1)&&(n!=ID_LENGTH)&&(n!=0)){//n<ID_LENGTH
        while((n!=ID_LENGTH)&&((tmp_erro==0)||(tmp_erro==EAGAIN)||(tmp_erro==EWOULDBLOCK))){
            char tmp[1024]="";
            ssize_t tmpValue=0;
            if(n==-1){
                tmpValue=old_recv(sockfd,uuids,ID_LENGTH,flags);
            }else{
                tmpValue=old_recv(sockfd,&uuids[n],ID_LENGTH-n,flags);
            }
            tmp_erro=errno;

            
            if (tmpValue>=0){
                if(n>0){
                    n+=tmpValue;
                }else{
                    n=tmpValue;
                }
            }
        }
    }
    
    
    if(n==ID_LENGTH){
        
        if((uuids[0]=='@')&&(uuids[ID_LENGTH-2]=='@')&&(uuids[ID_LENGTH-1]=='\0')){

            *length=*((size_t *)&uuids[1]);
            int i=0;
            for(;i<sizeof(length);i++){
                uuids[i+1]='-';//just for print
            }

            errno=0;
            return ID_LENGTH;
        }else{//if it's not ID
            *length=0;
            log_message(uuids,ID_LENGTH,"check1");
            errno=0;
            return ID_LENGTH;
        }
        
        
    }else if(n==-1){
        errno=tmp_erro;
        return -1;
    }else if((n<ID_LENGTH)&&(n>0)){
        log_important("fatal_shorthead_readhead");
        log_message(uuids,n,"check2");
        errno=tmp_erro;
        return -1;//actually we should deal wth the rest of DATA_ID, and even if it's not a ID (from writev)
    }else{
        log_important("fatal_shorthead2_readhead");
        errno=tmp_erro;
        return 0;
    }
    
}

ssize_t check_read_rest(char * buf,int sockfd,size_t length, size_t count, int flags){
    static void *handle = NULL;
    static RECV old_recv = NULL;
    if( !handle )
    {
        handle = dlopen("libc.so.6", RTLD_LAZY);
        old_recv = (RECV)dlsym(handle, "recv");
    }
    int tmp_erro=0;
    ssize_t result=0;
    if(count<length){//should mark as some bytes left
        result=old_recv(sockfd,buf,count,flags);
        tmp_erro=errno;
        while((result!=count)&&((tmp_erro==0)||(tmp_erro==EAGAIN)||(tmp_erro==EWOULDBLOCK))){
            ssize_t tmpValue=0;
            if (result!=-1){
                tmpValue=old_recv(sockfd,&buf[result],count-result,flags);
            }else{
                tmpValue=old_recv(sockfd,&buf[0],count,flags);
            }
            tmp_erro=errno;
            if(tmpValue>0){
                if(result>0){
                    result+=tmpValue;
                }else{
                    result=tmpValue;
                }
            }
        }
        if(result>0){
            op_storage(S_PUT,sockfd,length-result);//here it should length-result
        }
        
        if(result!=count){
            char tmp[1024]="";
            sprintf(tmp,"here:result:%ld\tcount:%lu\n",result,count);
            log_message(tmp,strlen(tmp),"read_rest");
            log_important("fatal_short_readrest");
        }
        return result;
    }else{
        result=old_recv(sockfd,buf,length,flags);
        tmp_erro=errno;
        while((result!=length)&&((tmp_erro==0)||(tmp_erro==EAGAIN)||(tmp_erro==EWOULDBLOCK))){
            ssize_t tmpValue=0;
            if (result!=-1){
                tmpValue=old_recv(sockfd,&buf[result],length-result,flags);
            }else{
                tmpValue=old_recv(sockfd,&buf[0],length,flags);
            }
            tmp_erro=errno;
            if(tmpValue>0){
                if(result>0){
                    result+=tmpValue;
                }else{
                    result=tmpValue;
                }
            }

        }
        if(result==length){
            return result;
        }else if(result==-1){
            errno=tmp_erro;
            return -1;
        }else if(result==0){
            return 0;
        }else{
            op_storage(S_PUT,sockfd,length-result);
            errno=tmp_erro;
            return result;
        }
        
    }
}

