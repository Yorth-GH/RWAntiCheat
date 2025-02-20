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

        public ACClient(Socket client, int sessionId, bool isClassic = false)
        {
            _client = client;
            _sessionId = sessionId; 
            _isDisconnected = false;
            _buffer = new byte[1024 * 16];
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
