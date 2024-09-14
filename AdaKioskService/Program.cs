using System;
using System.Collections.Generic;
using System.Linq;
using System.ServiceProcess;
using System.Text;
using System.Threading.Tasks;

namespace AdaKioskService
{
    static class Program
    {
        /// <summary>
        /// The main entry point for the application.
        /// </summary>
        static async Task Main()
        {
            if (System.Diagnostics.Debugger.IsAttached)
            {
                await Task.Run(async () =>
                {
                    var service = new AdaKioskService();
                    service.UpdateCheckDelay = 1000; // 1 second!
                    service.DebugStart();
                    await Task.Delay(1000 * 60 * 30);
                });             
            }
            else
            {
                ServiceBase[] ServicesToRun;
                ServicesToRun = new ServiceBase[]
                {
                    new AdaKioskService()
                };
                ServiceBase.Run(ServicesToRun);
            }
        }
    }
}
