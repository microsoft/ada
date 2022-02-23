using Microsoft.Azure.WebJobs;
using Microsoft.Azure.WebJobs.Hosting;
using Microsoft.Extensions.DependencyInjection;
using Microsoft.Extensions.DependencyInjection.Extensions;
using System;
using System.Collections.Generic;
using System.Text;

[assembly: WebJobsStartup(typeof(AdaServerRelay.Startup))]

namespace AdaServerRelay
{
    public class Startup : IWebJobsStartup
    {
        public void Configure(IWebJobsBuilder builder)
        {
            builder.Services.Add(ServiceDescriptor.Singleton<IWebPubSubGroup>(new WebPubSubGroup()));
        }
    }
}
