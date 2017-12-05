# tracer
tracer-beta

##1、tracer.c
* Use the following command to compile the tracer, and use **LD_PROLOAD** to intercept network requests.

 `gcc -fPIC -shared -o hook.so tracer.c -ldl -luuid -lcurl -lpthread`

* if you want to test the tracer on your cluster, please replace the ip address in tracer.c(which is 10.211.55.38) with your own controller service(the port of controller service should still be 80).

###To Do
	1、finish sendmsg/recvmsg, sendto/recvfrom, writev/readv
	2、Case 2: message queue based requests tracing(memory tracing or other solutions)
	3、Automatically generate the trace of a request from the trace database
	4、generate hadoop tracing data and analyze

​	

##2、controller.php
	Redis service should be set up as the backend of controller

##3、tracedb.sql
Mysql5.5 is used as the database of tracing data

##4、read/write
###-Read
`	read 逻辑
	getOldMethod()
	if(！issocket()){
		old_read()
	}
	LOG
	type=getScoketType()
	if(type==INET4){// 仅仅拦截基于IPv4的网络数据，hadoop默认会在网络支持的情况下开启IPv6，所以需要关闭
		LOG
		if (get socket && get peer){
			getIP&Port();
			LOG
			getSocketRead&WriteBufferSize();
			check_filter();
			if(with_uuid==0){
				n=old_read(fd,buf,count);
				LOG
				if(n<=0){
					LOG
					return n;
					}
				LOG
				return n;
			else{
				LOG
				char id[ID_LENGTH]
				processMessage&ExtractUUID()
			}			
		}else{
			LOG
			return old_read(fd,buf,count);
		}
	}else if(type=UNIX){
		old_write()
	}else if(type=UNKNOWN){
		old_write()
	}else{
		old_write()
	}`
###-write
`	write 逻辑
	getOldMethod()
	if(！issocket()){
		old_write()
	}
	LOG
	type=getScoketType()
	if(type==INET4){// 仅仅拦截基于IPv4的网络数据，hadoop默认会在网络支持的情况下开启IPv6，所以需要关闭
		LOG
		if(get socket && getpeer){
			getIP&Port();
			LOG
			getSocketRead&WriteBufferSize();
			new_size=old_write_size+ID_LENGTH
			if (new_size almost MAX_SIZE){
				LOG
				old_write()
			}
			if(with_uuid==0){
				LOG
			}
			if(check_filter()){
				LOG
				old_write()
			}
			if(without_uuid){
				LOG
				old_write()
			}
			generate_uuid();
			new_message=add_uuid_to_old_message();// 注意把uuid放到原生消息的前面
			n=old_write(new_message);
			LOG
			if(n==new_size){
	           			 return old_size;
	       		 }else if(n<=old_size){
	            		 	 return n;
	        		}else{
	            			return old_size;
	        		}
		}else{
			LOG
			return old_write(fd,buf,count);
		}
	}else if(type=UNIX){
		old_write()
	}else if(type=UNKNOWN){
		old_write()
	}else{
		old_write()
	}
`