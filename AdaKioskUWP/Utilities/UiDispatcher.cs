// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
using System;
using Windows.UI.Core;

namespace AdaSimulation
{
    /// <summary>
    /// A simple helper class that gives a way to run things on the UI thread.  The app must call Initialize once during app start, using inside OnLaunch.
    /// </summary>
    public class UiDispatcher
    {
        static UiDispatcher instance;
        CoreDispatcher dispatcher;


        public static UiDispatcher Initialize(CoreDispatcher dispatcher)
        {
            instance = new UiDispatcher()
            {
                dispatcher = dispatcher
            };
            return instance;
        }

        public void RunOnUIThread(Action a)
        {
            _ = dispatcher.RunAsync(CoreDispatcherPriority.Normal, new DispatchedHandler(() => a()));
        }

        public static UiDispatcher Instance { get { return instance; } }

    }
}
