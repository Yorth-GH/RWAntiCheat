using System.Net;
using System.Net.Sockets;

namespace RetroWar.ACSrv
{
    internal class ACListener
    {
        public int _port = -1;
        private Socket _socket;

        public List<ACClient> _clientList = new List<ACClient>();

        private int _connectionId = 0;

        public ACListener(int port)
        {
            _port = port;
            _socket = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);
        }

        public bool Start()
        {
            try
            {
                _socket.Bind(new IPEndPoint(IPAddress.Any, _port));
                _socket.Listen(32);
                _socket.BeginAccept(new AsyncCallback(OnConnection), null);
                return true;
            }
            catch { }
            return false;
        }

        private void OnConnection(IAsyncResult iAr)
        {
            try
            {
                Socket ClientSock = _socket.EndAccept(iAr);
                if (ClientSock.Connected)
                {
                    _connectionId++;
                    ACClient Client = new ACClient(ClientSock, _connectionId, false);
                    _clientList.Add(Client);
                }
            }
            catch { }
            _socket.BeginAccept(new AsyncCallback(OnConnection), null);
        }
    }
}
