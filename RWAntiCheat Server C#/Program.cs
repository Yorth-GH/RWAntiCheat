using System;
using System.Net;
using System.Net.Sockets;
using System.IO;
using System.Threading;
using System.Text;

class Program
{
    const int PORT = 27500;

    static string GetMessage(string str)
    {
        if (string.IsNullOrEmpty(str))
            return "Empty message";

        char code = str[0];
        string extra = (str.Length > 1) ? str.Substring(1) : "";
        switch (code)
        {
            case '0':
                return "Connection accepted!";
            case '1':
                return "Loaded from within another game! Game Path: " + extra;
            case '2':
                return "Debugger detected!";
            case '3':
                return "Forbidden process open! " + extra;
            case '4':
                return "Unknown module loaded! " + extra;
            case '5':
                return "Heartbeat from client received!";
            case '6':
                return "Connection closing!";
            case '7':
                return "Unknown UNVERIFIED module loaded! " + extra;
            default:
                return "Incorrect message! Message: " + extra;
        }
    }


    static void ClientHandler(Socket clientSocket, string ip)
    {
        int count = 0;
        string logName = $"log_{ip}_{count}.log";
        // Ensure the log file name is unique.
        while (File.Exists(logName))
        {
            count++;
            logName = $"log_{ip}_{count}.log";
        }

        using (StreamWriter log = new StreamWriter(logName, true))
        {
            byte[] buffer = new byte[64];

            try
            {
                while (true)
                {
                    // Clear the buffer similar to ZeroMemory.
                    Array.Clear(buffer, 0, buffer.Length);

                    // Receive data from the client.
                    int received = clientSocket.Receive(buffer);
                    if (received <= 0)
                        break;

                    // Convert the received bytes to a string (assuming ASCII encoding).
                    string receivedStr = Encoding.ASCII.GetString(buffer, 0, received);
                    string message = GetMessage(receivedStr);

                    // Write the message to the console and the log file.
                    Console.WriteLine($"{message} - {ip}");
                    log.WriteLine(message);
                    log.Flush();
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Error handling client {ip}: {ex.Message}");
            }
            finally
            {
                // Close the client socket.
                clientSocket.Close();
            }
        }
    }

    static void Main(string[] args)
    {
        Socket serverSocket = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);
        serverSocket.Bind(new IPEndPoint(IPAddress.Any, PORT));
        serverSocket.Listen(32);

        Console.WriteLine("Server started...");

        while (true)
        {
            // Accept an incoming client connection.
            Socket clientSocket = serverSocket.Accept();
            string clientIP = ((IPEndPoint)clientSocket.RemoteEndPoint).Address.ToString();

            Console.WriteLine("Client connecting from IP: " + clientIP);


            Thread clientThread = new Thread(() => ClientHandler(clientSocket, clientIP))
            {
                IsBackground = true
            };
            clientThread.Start();
        }
    }
}