using System;
using System.IO;
using System.Net;
using System.Net.Sockets;
using System.Text;
using RetroWar.Shared;

namespace RetroWar.ACSrv
{
    internal class FileData
    {
        public string Name { get; set; }
        public byte[] File { get; set; }
    }

    internal class FileReceiver
    {
        private readonly int _port;
        private readonly string _baseDirectory = "ReceivedFiles";
        private TcpListener _listener;

        public FileReceiver(int port)
        {
            _port = port;
            Directory.CreateDirectory(_baseDirectory); // Ensure base directory exists
        }

        public void Start()
        {
            _listener = new TcpListener(IPAddress.Any, _port);
            _listener.Start();

            Logging.Instance.Success($"File receiver listening on port {_port}...");

            new Thread(() =>
            {
                while (true)
                {
                    var client = _listener.AcceptTcpClient();
                    HandleClient(client);
                }
            })
            { Priority = ThreadPriority.AboveNormal }
            .Start();
        }

        private void HandleClient(TcpClient client)
        {
            var clientEndPoint = (IPEndPoint)client.Client.RemoteEndPoint;
            string clientIp = clientEndPoint.Address.ToString();

            using (var networkStream = client.GetStream())
            {
                try
                {
                    //magic byte
                    using BinaryReader reader = new BinaryReader(networkStream);
                    byte magic = reader.ReadByte();

                    if (magic != 0xCC)
                    {
                        Logging.Instance.Debug("Invalid magic byte received, ignoring request.");
                        return;
                    }

                    int fileNameLength = reader.ReadInt32();
                    string fileName = new string(reader.ReadChars(fileNameLength));

                    long fileSize = reader.ReadInt32();
                    string filePath1 = Path.Combine(_baseDirectory, clientIp);
                    string filePath = Path.Combine(filePath1, fileName);

                    using FileStream fs = new FileStream(filePath, FileMode.Create, FileAccess.Write);

                    byte[] buffer = new byte[4096];
                    long bytesReceived = 0;

                    while (bytesReceived < fileSize)
                    {
                        int bytesToRead = (int)Math.Min(buffer.Length, fileSize - bytesReceived);
                        int bytesRead = networkStream.Read(buffer, 0, bytesToRead);
                        if (bytesRead == 0) 
                            break; // Connection closed unexpectedly

                        fs.Write(buffer, 0, bytesRead);
                        bytesReceived += bytesRead;
                    }

                    Logging.Instance.Debug($"File {fileName} received!");

                }
                catch (Exception ex)
                {
                    Logging.Instance.Error($"Error handling client {clientIp}: {ex.Message}");
                }
            }
        }

        public void Stop()
        {
            _listener?.Stop(); 
        }
    }

}
