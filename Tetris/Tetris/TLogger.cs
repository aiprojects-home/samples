using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

using System.IO;

namespace Tetris
{
    internal class TLogger : IDisposable
    {
        private static TLogger m_Current = null;
        private bool m_bDisposed = false;
        private StreamWriter m_LogFile = null;
        private static object m_Token = new object();

        private TLogger()
        {

        }

        ~TLogger()
        {
            Dispose(false);
            GC.SuppressFinalize(this);
        }

        public static TLogger Current
        {
            get
            {
                lock (m_Token)
                {
                    if (m_Current == null)
                    {
                        m_Current = new TLogger();
                    }

                    return m_Current;
                }
            }
        }

        public bool CreateLogFile(string strLogFile)
        {
            lock (m_Token)
            {
                if (m_LogFile != null)
                {
                    m_LogFile.Close();
                }

                try
                {
                    m_LogFile = new StreamWriter(strLogFile);
                } catch
                {
                    m_LogFile = null;
                    return false;
                }

                return true;
            }
        }

        public bool WriteLine(string strLine)
        {
            lock (m_Token)
            {
                if (m_LogFile == null)
                {
                    return false;
                }

                m_LogFile.WriteLine(strLine);

                return true;
            };
        }

        public void Dispose()
        {
            lock (m_Token)
            {
                Dispose(true);
            }
        }

        private void Dispose(bool bDisposing)
        {
            if (m_bDisposed)
            {
                return;

            }
            if (bDisposing)
            {
                // Освобождение управляемых ресурсов.
            };

            if (m_LogFile != null)
            {
                m_LogFile.Close();
                m_LogFile = null;
            };

            m_bDisposed = true;
        }
    }
}
