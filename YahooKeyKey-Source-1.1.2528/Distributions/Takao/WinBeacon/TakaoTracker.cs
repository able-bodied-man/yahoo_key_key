// [AUTO_HEADER]

using System;

namespace Takao
{
    /// <summary>
    /// Stub for WinBeacon/TakaoTracker.cs, which was not part of the open-source
    /// release. KeyKeyUI referenced it for two things: reporting the process
    /// bitness for a URL, and firing a tracking beacon to Yahoo endpoints that
    /// have been dead for years. Both are answered here so the assembly builds;
    /// the beacon is deliberately a no-op.
    /// </summary>
    public static class Tracker
    {
        public static bool Is64BitMode()
        {
            return IntPtr.Size == 8;
        }

        public static void SendTrackingURL(string url)
        {
        }
    }
}
