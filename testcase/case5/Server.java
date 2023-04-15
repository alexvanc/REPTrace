import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.PrintStream;
import java.net.ServerSocket;
import java.net.Socket;
public class Server  implements Runnable {

    private Socket client = null;

    public Server(Socket client){
        this.client = client;
    }

    @Override
    public void run() {
        try{
            PrintStream write = new PrintStream(client.getOutputStream());
            BufferedReader in = new BufferedReader(new InputStreamReader(client.getInputStream()));

            String str =  in.readLine();
            System.out.println("Server receive : " + str);
            String outStr = "Server send 1!";
            write.println(outStr);

            write.close();
            in.close();
            client.close();
        }catch(Exception e){
            e.printStackTrace();
        }
    }


    public static void main(String[] args) throws Exception{
        int port = 8888;
        ServerSocket server = new ServerSocket(port);
        Socket client = null;
        boolean f = true;
        while(f){
            client = server.accept();
            new Thread(new Server(client)).start();
        }
        server.close();
    }
}