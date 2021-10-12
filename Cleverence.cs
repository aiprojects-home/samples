using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Threading;

namespace PowerLine
{
    public static class Server
    {
        private static int Count = 0;
        private static object WriteToken = new object();
        private static long WriteFlag = 0;

        public static int GetCount()
        {
            int c = 0;

            if (Interlocked.Read(ref WriteFlag) == 1)
            {
                lock (WriteToken)
                {
                    c = Count;
                }
            }
            else
            {
                c = Count;
            }

            return c;
        }

        public static void AddToCount(int value)
        {
            lock (WriteToken)
            {
                Interlocked.Exchange(ref WriteFlag, 1);
                Count += value;
                Interlocked.Exchange(ref WriteFlag, 0);
            }
        }
    }

    public class AsyncCaller
    {
        private EventHandler Handler = null;
        public AsyncCaller(EventHandler h)
        {
            if (h == null)
                throw new ArgumentNullException();

            Handler = h;
        }

        public bool Invoke(int timeout, object sender, EventArgs args)
        {
            IAsyncResult res = Handler.BeginInvoke(sender, args, null, null);

            while (timeout > 0)
            {
                if (res.IsCompleted == true)
                {
                    Handler.EndInvoke(res);

                    return true;
                } else
                {
                    Thread.Sleep(10);
                    timeout -= 10;
                }
            }

            return false;
        }
    }
}
