//
//  specific.c
//  
//
//  Created by 杨勇 on 24/09/2017.
//
//

#include "specific.h"

static pthread_key_t key=5;

void func_a(){
    char * result;
    printf("test1\tkey1:%u\tvalue%s\n",key,pthread_getspecific(key));
    result=(char *)pthread_getspecific(key);
    if(!result){
        pthread_setspecific(key,"test a");
    }else{
        printf("in func_a: %s",result);
    }
    
}

void func_b(){
    char * result;
    printf("testb\tkey1:%u\tvalue%s\n",key,pthread_getspecific(key));
    result=(char *)pthread_getspecific(key);
    if(!result){
        pthread_setspecific(key,"test b");
    }else{
        printf("in func_b: %s",result);
    }

}

int main(){
    pthread_t ta, tb;
    printf("test1\tkey1:%u\tvalue%s\n",key,pthread_getspecific(key));
    pthread_key_create(&key,NULL);
    printf("test1\tkey2:%u\tvalue%s\n",key,pthread_getspecific(key));
    pthread_setspecific(key,"test main");
    pthread_create(&ta,NULL,func_a,NULL);
    pthread_create(&tb,NULL,func_b,NULL);
    
}
