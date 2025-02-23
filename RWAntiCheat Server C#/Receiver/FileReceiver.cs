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
                    byte[] header = new byte[7]; // 1 byte magicByte + 2 bytes nameLen + 4 bytes fileLen
                    networkStream.Read(header, 0, header.Length);

                    if (header[0] != 0xCC)
                    {
                        Logging.Instance.Debug("Invalid magic byte received, ignoring request.");
                        return;
                    }

                    short nameLen = BitConverter.ToInt16(header, 1);
                    int fileLen = BitConverter.ToInt32(header, 3);

                    byte[] dataBuffer = new byte[nameLen + fileLen];
                    networkStream.Read(dataBuffer, 0, dataBuffer.Length);

                    string fileName = Encoding.UTF8.GetString(dataBuffer, 0, nameLen);
                    byte[] fileContent = new byte[fileLen];
                    Array.Copy(dataBuffer, nameLen, fileContent, 0, fileLen);

                    FileData fileData = new FileData { Name = fileName, File = fileContent };

                    string clientFolder = Path.Combine(_baseDirectory, clientIp);
                    Directory.CreateDirectory(clientFolder);
                    string filePath = Path.Combine(clientFolder, fileData.Name);

                    File.WriteAllBytes(filePath, fileData.File);
                    Logging.Instance.Success($"File received from {clientIp}: {fileData.Name}");
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
