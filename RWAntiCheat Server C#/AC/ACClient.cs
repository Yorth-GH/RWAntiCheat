using RetroWar.Shared;
using System.Net;
using System.Net.Sockets;

namespace RetroWar.ACSrv
{ 
    internal class ACClient
    {
        private Socket _client;
        private int _sessionId = -1;

        private bool _isDisconnected = false; 

        private List<byte> _packetCache = new List<byte>();
        private byte[] _buffer = new byte[1024];

        public int _userId = -1;
        public int _rank = -1;
        public int _serverId = -1;

        public void Disconnect()
        {
            if (_isDisconnected) return;
            _isDisconnected = true;

            try { _client.Close(); }
            catch { }

            try { Program.ACServer._clientList.Remove(this); }
            catch { }
        }

        public string _userDir = "NULL";

        public void SetUserDir()
        {
            string ipAddr = ((IPEndPoint)_client.RemoteEndPoint).Address.ToString();
            string userDir = Path.Combine(Environment.CurrentDirectory + $"/{ipAddr}/");

            if (Directory.Exists(userDir) == false) Directory.CreateDirectory(userDir);

            _userDir = userDir;
        }

        public ACClient(Socket client, int sessionId, bool isClassic = false)
        {
            _client = client;
            _sessionId = sessionId; 
            _isDisconnected = false;
            _buffer = new byte[1024 * 16];
            SetUserDir();
            _client.BeginReceive(_buffer, 0, _buffer.Length, SocketFlags.None, new AsyncCallback(OnReceive), null); 
        }

        public void Send(RetroWar.Game.Packet p)
        {
            var output = p.Writer.Build();
            Send(output);
        }

        public void Send(string s)
        {
            var output = System.Text.Encoding.UTF8.GetBytes(s);
            Send(output);
        }

        public void Send(byte[] data)
        {
            for (int i = 0; i < data.Length; i++) { data[i] ^= 0x96; }
            try { _client.SendAsync(data); }
            catch { Disconnect(); }
        }

        //public void MakeLoginError(E_LoginErr err)
        //{
        //    var packet = new Game.Packet(4352, Game.Packet.E_PACKETTYPE.Text);
        //    packet.AddBlock((int)err);
        //    Send(packet);
        //}

        public static void HexStringToFile(string hexString, string outputFilePath)
        {
            byte[] bytes = new byte[hexString.Length / 2];
            for (int i = 0; i < hexString.Length; i += 2)
            {
                bytes[i / 2] = Convert.ToByte(hexString.Substring(i, 2), 16);
            }
            if (File.Exists(outputFilePath)) File.Delete(outputFilePath); 
            File.WriteAllBytes(outputFilePath, bytes);
        }

        private void ProcessMsg(byte[] data)
        {
            try
            {
                var packet = System.Text.Encoding.UTF8.GetString(data).Split(' ');

                var timeStamp = int.Parse(packet[0]);
                var opCode = int.Parse(packet[1]);

                var blocks = new string[packet.Length - 2];
                Array.Copy(packet, 2, blocks, 0, blocks.Length);
               

                switch (opCode)
                {
                    case 4352: // LOGIN
                        {
                            var username = blocks[2].Replace('\x1D', ' ').Trim();
                            var password = blocks[3].Replace('\x1D', ' ').Trim();
                             
                            break;
                        }
                    case 11001: // AC REPORT
                        {
                            var blocks2 = new string[packet.Length - 3];
                            Array.Copy(blocks, 1, blocks2, 0, blocks2.Length);
                            switch (blocks[0])
                            {
                                case "7":
                                    Logging.Instance.Debug("AC REPORT - Unverified module: " + string.Join(" ", blocks2) + " SENDING TO SERVER!");
                                break;
                                case "6":
                                    Logging.Instance.Debug("AC REPORT - Connection to server closing!");
                                break;
                                case "5":
                                    Logging.Instance.Debug("AC REPORT - Heartbeat callback! " + string.Join(" ", blocks2));
                                break;
                                case "4":
                                    Logging.Instance.Debug("AC REPORT - Unknown module: " + string.Join(" ", blocks2));
                                break;
                                case "3":
                                    Logging.Instance.Debug("AC REPORT - Unverified module: " + string.Join(" ", blocks2));
                                break;
                                case "2":
                                    Logging.Instance.Debug("AC REPORT - Unverified module: " + string.Join(" ", blocks2));
                                break;
                                case "1":
                                    Logging.Instance.Debug("AC REPORT - Unverified module: " + string.Join(" ", blocks2));
                                break;
                                case "0":
                                    Logging.Instance.Debug("AC REPORT - Unverified module: " + string.Join(" ", blocks2));
                                break;
                            }
                             
                            break;
                        }
                    case 11002: // MODULE TRANSFER
                        {
                            string fileName = Path.GetFileName(blocks[0]);
                            string newName = $"{Path.GetFileNameWithoutExtension(fileName)}_{DateTime.Now.ToString("dd.MM.yyyy-HH.mm")}{Path.GetExtension(fileName)}";
                            string filePath = Path.Combine(_userDir + newName);

                            if (File.Exists(filePath) == true) return; // Dont double safe

                            HexStringToFile(blocks[1], newName);

                            Logging.Instance.Debug($"Saved dumped module {fileName} to {filePath}");
                            break;
                        }
                    default:
                        {
                            Logging.Instance.Debug("Unhandled opCode (opc: " + opCode + ", blocks: " + string.Join(" ", blocks) + ")");
                            break;
                        }
                }
            }
            catch { Disconnect(); }
        }

        private void OnReceive(IAsyncResult ar)
        {
            OnReceive(ar, _packetCache);
        }

        private void OnReceive(IAsyncResult ar, List<byte> _packetCache)
        {
            try
            {
                int length = _client.EndReceive(ar);
                if (length <= 0) { Disconnect(); }
                else
                {
                    for (int i = 0; i < length; i++)
                        _packetCache.Add((byte)(_buffer[i]));

                    if (_buffer[length-1] != '\n')
                        _client.BeginReceive(_buffer, 0, _buffer.Length, SocketFlags.None, new AsyncCallback(OnReceive), null);

                    int startIndex = 0;
                    for (int i = 0; i < _packetCache.Count; i++)
                    {   
                        if (_packetCache[i] == (byte)'\n')
                        {
                            byte[] packetBuffer = new byte[i - startIndex];
                            _packetCache.CopyTo(startIndex, packetBuffer, 0, i - startIndex);

                            ProcessMsg(packetBuffer);

                            startIndex = i + 1;
                        }
                    }

                    if (startIndex < _packetCache.Count)
                    {
                        _packetCache.RemoveRange(0, startIndex );
                    }
                    else
                    {
                        _packetCache.Clear();
                    }

                    _client.BeginReceive(_buffer, 0, _buffer.Length, SocketFlags.None, new AsyncCallback(OnReceive), null);
                }
            }
            catch { Disconnect(); }
        }
    }
}
