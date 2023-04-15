#coding=utf-8
import socket
import sys
def client():
    HOST = "10.0.0.41"
    PORT = 8888
    ADDR = (HOST,PORT)
    BUFSIZE = 1024

    sock = socket.socket()
    try:
        a = sock.connect(ADDR)
    except Exception,e:
        print 'error',e
        sock.close()
        sys.exit()

    data = "Client send 1 !"
    sock.sendall(data)
    recv_data = sock.recv(BUFSIZE)
    print 'Client1 receive: ',recv_data
    sock.close()

if __name__ =="__main__":
    client()