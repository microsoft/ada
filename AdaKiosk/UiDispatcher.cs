using System;
using System.Windows.Threading;

namespace AdaSimulation
{
    /// <summary>
    /// A simple helper class that gives a way to run things on the UI thread.  The app must call Initialize once during app start, using inside OnLaunch.
    /// </summary>
    public class UiDispatcher 
    {
        static UiDispatcher instance;
        Dispatcher dispatcher;


        public static UiDispatcher Initialize()
        {
            instance = new UiDispatcher()
            {
                dispatcher = Dispatcher.CurrentDispatcher
            };
            return instance;
        }

        public void RunOnUIThread(Action a)
        {
            _ = dispatcher.BeginInvoke(a);
        }

        public static UiDispatcher Instance { get { return instance; } }

    }
}
