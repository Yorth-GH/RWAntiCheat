using RetroWar.Shared;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Xml.Schema;

namespace RetroWar.ACSrv
{
    internal class SigScanner
    {
        public static byte?[] sig_parser(string sig_string)
        {
            string[] sig_bytes = sig_string.Split(" ");
            byte?[] sig = new byte?[sig_bytes.Length];
            
            for (int i = 0; i < sig_bytes.Length; i++)
            {
                if (sig_bytes[i] == "??")
                    sig[i] = (byte?)null;
                else
                    sig[i] = Convert.ToByte(sig_bytes[i], 16);
            }

            return sig;
        }

        public static bool scan(byte[] file_data, byte?[] sig)
        {
            for (int i = 0; i < file_data.Length - sig.Length; i++)
            {
                bool present = true;
                for (int j = 0; j < sig.Length; j++)
                {
                    if (sig[j] == null && file_data[i+j] != sig[j])
                    {
                        present = false; 
                        break;
                    }
                }

                if (present)
                {
                    Logging.Instance.Warning("Matching byte pattern found! KNOWN CHEAT DETECTED!");
                    return true;
                }   
            }
            return false;
        }
    }
}
