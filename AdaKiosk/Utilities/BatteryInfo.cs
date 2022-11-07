using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Runtime.InteropServices;

namespace AdaKiosk.Utilities
{
    enum ACLineStatus : byte
    {
        Offline = 0,
        Online = 1,
        Unknown = 255
    }

    enum BatteryFlag : byte
    {
        High = 1,
        Low = 2,
        Critical = 4,
        Charging = 8,
        NoBattery = 128,
        Unknown = 255
    }

    enum SystemStatus : byte
    {
        SaverOff = 0,
        SaverOn = 1
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct SYSTEM_POWER_STATUS
    {
        public ACLineStatus ACLineStatus;
        public BatteryFlag BatteryFlag;

        /// <summary>
        /// The percentage of full battery charge remaining. This member can be a value in the range 0 to 100, or 255 if status is unknown.
        /// </summary>
        public byte BatteryLifePercent;

        public SystemStatus SystemStatusFlag;

        /// <summary>
        /// The number of seconds of battery life remaining, or –1 if remaining seconds are unknown or if the device is connected to AC power.
        /// </summary>
        public uint BatteryLifeTime;

        /// <summary>
        /// The number of seconds of battery life remaining, or –1 if remaining seconds are unknown or if the device is connected to AC power.
        /// </summary>
        public uint BatteryFullLifeTime;
    }

    internal class BatteryInfo
    {
        [DllImport("Kernel32")]
        internal static extern bool GetSystemPowerStatus(out SYSTEM_POWER_STATUS lpSystemPowerStatus);
    }
}
