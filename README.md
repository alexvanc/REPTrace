# REPTrace
REPTrace (Request Execution Path Trace)[1] is a generic methodology that captures complete execution paths of service/job request processing of distributed platforms in a transparent way. REPTrace intercepts runtime events at the system level (either within a physical machine, VM, or container), such as common library calls or system calls (intercepting library calls does not require root privilege, though intercepting OS system calls requires), and constructs the request execution path by identifying the causal rela‐ tionships for four categories of events: thread events, process events, network commu‐ nications events and control flow execution events.

## compile
* Use the following command to compile the tracer

	`gcc -fPIC -shared -o hook.so tracer.c -ldl -luuid -lcurl -lpthread`

* if you want to test the tracer on your cluster, please replace the ip address in tracer.c(which is 10.211.55.38) with your own controller service(the port of controller service should still be 80).

## run
* Set **LD_PROLOAD** for the service/program to be traced when starting the service/program. For example:
  
   `LD_PRELOAD=hook.so serice.exe`

### To Do
1、Case 2: message queue based requests tracing(memory tracing or other solutions)


## 2 About IPV6
Currently we are not supporting ipv6, please disable ipv6 on your linux server by adding following to the end of `/etc/sysctl.conf` :

	net.ipv6.conf.all.disable_ipv6 = 1
	net.ipv6.conf.default.disable_ipv6 = 1
	net.ipv6.conf.lo.disable_ipv6 = 1
and then run `sudo sysctl -p`

# Reference
1. Yang, Yong, et al. "Transparently capturing execution path of service/job request processing." Service-Oriented Computing: 16th International Conference, ICSOC 2018, Hangzhou, China, November 12-15, 2018, Proceedings 16. Springer International Publishing, 2018.




