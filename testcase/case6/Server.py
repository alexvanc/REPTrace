#coding=utf-8
#reference :http://www.cnblogs.com/ajianbeyourself/p/3771198.html
from SocketServer import  BaseRequestHandler,ThreadingTCPServer
import threading
BUFSIZE = 1024

class Handler(BaseRequestHandler): # to process the connect
    def handle(self):
        data = self.request.recv(BUFSIZE)
        print 'Server receive: ',data
        cur_thread = threading.currentThread()
        response = "Server send 1!"
        self.request.sendall(response)

if __name__ == "__main__":
    HOST = ''
    PORT = 8888
    ADDR = (HOST,PORT)
    server = ThreadingTCPServer(ADDR,Handler)
    server.serve_forever()
