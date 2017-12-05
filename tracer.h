//
//  tracer.h
//
//
//  Created by 杨勇 on 21/06/2017.
//
//
#define _GNU_SOURCE
#ifndef tracer_h
#define tracer_h


#endif /* tracer_h */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <dlfcn.h>
#include <arpa/inet.h>
#include <sys/timeb.h>
#include <pthread.h>
#include <uuid/uuid.h>
#include <curl/curl.h>
#include <string.h>
#include <sys/un.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <spawn.h>
#include <sys/syscall.h>

#define MAX_SPACE 1000000
#define LOG_LENGTH 2048
#define ID_LENGTH 39
#define UNKNOWN_FAMILY 99

#define DEBUG 1
#ifdef DEBUG
//#define MESSAGE 1
//#define OTHER_SOCK 1
#define DATA_INFO 1
//#define FILTER 1
#define IP_PORT 1
#define RW_ERR 1
//#define CONNECT 1
//#define SOCK 1
//#define HEARTBEAT 1
#endif

//define request type
#define R_DATABASE 24
#define R_CHECK_SEND 20
#define R_MARK_SEND 21
#define R_CHECK_RECV 22
#define R_MARK_RECV 23
#define R_OPEN_SOCK 25
#define R_MARK_SOCK 26
#define R_THREAD 27
#define R_THREAD_DEP 28




//define function type
#define F_SEND 1
#define F_RECV 2
#define F_WRITE 3
#define F_READ 4
#define F_SENDMSG 5
#define F_RECVMSG 6
#define F_SENDTO 7
#define F_RECVFROM 8
#define F_WRITEV 9
#define F_READV 10
#define F_SENDMMSG 11
#define F_RECVMMSG 12
#define F_CONNECT 13
#define F_SOCKET 14
#define F_CLOSE 15



//define finish type
#define SEND_NORMALLY 1
#define SEND_ERROR 2
#define SEND_ID 3
#define SEND_FILTER 4
#define RECV_NORMALLY 5
#define RECV_FILTER 6
#define RECV_ERROR 7
#define RECV_SINGLE 8
#define RECV_MULTI 9
#define RECV_SINGLE_BRO 10
#define RECV_MULTI_BRO 11
#define DONE_IP6 12
#define DONE_UNIX 13
#define DONE_OTHER 14
#define RECV_SIM1 15
#define RECV_SIM1_ID 16
#define RECV_SIM2 17
#define RECV_SIM3 18
#define RECV_SIM3_ID 19
#define RECV_NORMALLY_ID 20


typedef void*(*START)(void *);

//struct mmsghdr {
//    struct msghdr msg_hdr;  /* Message header */
//    unsigned int  msg_len;  /* Number of received bytes for header */
//};

struct string {
    char *ptr;
    size_t len;
};
struct thread_param{
    char * uuid;
    void * args;
    void *(*start_routine)(void *);
    long int ppid;
    long int pktid;
    unsigned long  int ptid;
    long long int ttime;
};


typedef ssize_t(*RECV)(int sockfd, void *buf, size_t len, int flags);
typedef ssize_t(*SEND)(int sockfd, const void *buf, size_t len, int flags);
typedef ssize_t(*WRITE)(int fd, const void *buf, size_t count);
typedef ssize_t(*READ)(int fd, void *buf, size_t count);
typedef ssize_t(*SENDMSG)(int sockfd, const struct msghdr *msg, int flags);
typedef ssize_t(*RECVMSG)(int sockfd, struct msghdr * msg, int flags);
typedef ssize_t(*SENDTO)(int socket, const void* buf, size_t buflen, int flags, const struct sockaddr* addr, socklen_t addrlen);
typedef ssize_t(*RECVFROM)(int socket, void* buf, size_t buflen, int flags, struct sockaddr* addr, socklen_t* addrlen);
typedef int(*SENDMMSG)(int sockfd, struct mmsghdr *msgvec, unsigned int vlen,unsigned int flags);
typedef int(*RECVMMSG)(int sockfd, struct mmsghdr *msgvec, unsigned int vlen,unsigned int flags, struct timespec *timeout);
typedef ssize_t(*READV)(int fd, const struct iovec *iov, int iovcnt);
typedef ssize_t(*WRITEV)(int fd, const struct iovec *iov, int iovcnt);

typedef void *(*DLSYM)(void *handle, const char *symbol);

typedef int(*SOCKET)(int domain, int type, int protocol);
typedef int(*CONN)(int socket, const struct sockaddr *addr, socklen_t length);
typedef int(*CLOSE)(int fd);

typedef int(*P_CREATE)(pthread_t *thread, const pthread_attr_t *attr,void *(*start_routine) (void *), void *arg);
typedef int(*K_DELETE)(pthread_key_t key);
typedef int(*P_JOIN)(pthread_t thread, void **retval);



typedef pid_t(*FORK)(void);
typedef pid_t(*VFORK)(void);
typedef int(*CLONE)(int (*fn)(void *), void *child_stack,int flags, void *arg, ... /* pid_t *ptid, struct user_desc *tls, pid_t *ctid */ );
typedef long(*SYSCALL)(long number, ...);
typedef int(*SPAWN)(pid_t *pid, const char *path,const posix_spawn_file_actions_t *file_actions,const posix_spawnattr_t *attrp,
char *const argv[], char *const envp[]);
typedef int(*SPAWNP)(pid_t *pid, const char *file,const posix_spawn_file_actions_t *file_actions,const posix_spawnattr_t *attrp,
char *const argv[], char *const envp[]);

typedef int(*EXECL)(const char *path, const char *arg, ...);

typedef int(*EXECLP)(const char *file, const char *arg, ...);

//typedef int(*EXECLE)(const char *path, const char *arg, ..., char *const envp[]);

typedef int(*EXECV)(const char *path, char *const argv[]);

typedef int(*EXECVE)(const char *path, char *const argv[], char *const envp[]);

typedef int(*EXECVP)(const char *file, char *const argv[]);

typedef int(*EXECVPE)(const char *file, char *const argv[], char *const envp[]);
//
//int execl(const char *path, const char *arg, ...);
//
//int execlp(const char *file, const char *arg, ...);
//
//int execle(const char *path, const char *arg, ..., char *const envp[]);
//
//int execv(const char *path, char *const argv[]);
//
//int execvp(const char *file, char *const argv[]);
//
//int execve(const char *path, char *const argv[], char *const envp[]);

//generate 37bytes uuid
char *random_uuid( char*);

//extract 37bytes uuid from message
//int get_uuid(char result[][ID_LENGTH],const char* buf,int len,char* after_uuid);
int get_uuid(char **result,const char* buf,int len,char* after_uuid);

//unused, message buffer in local db
int push_to_local_database(char*,int,char*,int,pid_t,pthread_t,char*,long long,char,int);

//send the message to the central db directly
int push_to_database(char*,int,char*,int,pid_t,pthread_t,char*,long long,char,long,long,int,char);

//send the message to the central db directly
int push_thread_db(pid_t ,pid_t,pthread_t ,pid_t,pid_t,pthread_t,long long );

//get the thread dependency information
int push_thread_dep(pid_t,pid_t,pthread_t,pthread_t,long long );



//check wether a file descriptor is a socket descriptor
int is_socket(int);

//get local timestamp
long long gettime();

//log infomation to local file
void log_event(char*);

//log message to local file
void log_message(char*,int);

//malloc memory for multi-uuid
char ** init_uuids(ssize_t m);

//check a socket is a unix socket or inet socket
sa_family_t get_socket_family(int sockfd);

//for experiment
void *getresponse2(void *);


//to get the http response
int get_response(char* post_parameter);
void init_string(struct string *s);
extern int errno;
size_t writefunc(void *ptr, size_t size, size_t nmemb, struct string *s);
// char *ptr, size_t size, size_t nmemb, void *userdata


