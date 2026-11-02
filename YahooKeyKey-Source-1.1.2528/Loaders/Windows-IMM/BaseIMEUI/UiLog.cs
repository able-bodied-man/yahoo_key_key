// [AUTO_HEADER]

using System;
using System.IO;
using System.Text;
using System.Threading;

namespace BaseIMEUI
{
    /// <summary>
    /// Tiny diagnostic log for the UI assembly. KeyKeyServer calls into these
    /// forms from RPC threads, and when such a call throws there is nowhere to
    /// see it: the exception crosses back to the client as 0xE0434F4D and the
    /// server keeps running. This writes the real exception to
    /// %LOCALAPPDATA%\KeyKeyTSF\keykeyui.log.
    /// </summary>
    internal static class UiLog
    {
        private static readonly object Gate = new object();
        private static string _path = null;

        public static string LogPath
        {
            get
            {
                if (_path == null)
                {
                    string directory = Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData);
                    directory = Path.Combine(directory, "KeyKeyTSF");
                    try { Directory.CreateDirectory(directory); }
                    catch { }
                    _path = Path.Combine(directory, "keykeyui.log");
                }
                return _path;
            }
        }

        public static void Write(string message)
        {
            lock (Gate)
            {
                try
                {
                    string line = DateTime.Now.ToString("yyyy-MM-dd HH:mm:ss.fff")
                                  + " [" + Thread.CurrentThread.ManagedThreadId + "] "
                                  + message + Environment.NewLine;
                    File.AppendAllText(LogPath, line, Encoding.UTF8);
                }
                catch
                {
                    // logging must never break the input method
                }
            }
        }
    }
}
