using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace RetroWar.ACSrv
{ 
    internal class SeededGenerator
    {
        private int seed;
        private int current;
        private const int A = 1664525;
        private const int C = 1013904223;
        private const int M = int.MaxValue;

        public SeededGenerator(int seed)
        {
            this.seed = seed;
            this.current = seed;
        }

        public int Next()
        {
            current = (A * current + C) % M;
            return current;
        }

        public List<int> GenerateSequence(int length)
        {
            List<int> sequence = new List<int>();
            for (int i = 0; i < length; i++)
            {
                sequence.Add(Next());
            }
            return sequence;
        }
    }  
}
