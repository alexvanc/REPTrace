import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.net.Socket;
public class Client {
    public static void main(String[] args) throws Exception{
        String host = "10.0.0.41";
        int port = 8888;
        Socket socket = new Socket(host,port);
        socket.setSoTimeout(10000);
        //BufferedReader input = new BufferedReader(new InputStreamReader(System.in));
        PrintWriter write=new PrintWriter(socket.getOutputStream());
        BufferedReader in=new BufferedReader(new InputStreamReader(socket.getInputStream()));

        String readline = "Client send 1!";
        write.println(readline);
        write.flush();
        // if waiting time > 10000, throw exception. Server doesn't send response;
        System.out.println("Client1 receive: "+in.readLine());

        write.close();
        in.close();
        socket.close();

    }
}