using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using WUApiLib;

namespace AdaKioskService
{
    class WindowsUpdater
    {
        public async Task UpdateAndRestart(CancellationTokenSource source)
        {
            var criteria = "Type != 'Driver'";
            var session = new UpdateSession();
            var searcher = session.CreateUpdateSearcher();
            Debug.WriteLine("SEARCH START.");
            ISearchResult result = string.IsNullOrEmpty(criteria)
                ? await searcher.SearchAsync(source.Token)
                : await searcher.SearchAsync(criteria, source.Token);
            Debug.WriteLine("SEARCH STOP.");
            Debug.WriteLine($"SEARCH RESULT: {result.ISearchResultToString()}");
            Debug.WriteLine("SEARCH RESULT ALL OUTPUT START.");
            foreach (var update in result.Updates.Cast<IUpdate>())
                Debug.WriteLine($"UPDATE: {update.IUpdateToString()}");
            Debug.WriteLine("SEARCH RESULT ALL OUTPUT STOP.");
        }
    }

    public static class IUpdateSearcherAsyncExtension
    {
        /// <summary>
        /// Find updates asynchronously
        /// </summary>
        /// <param name="searcher"></param>
        /// <param name="cancellationToken"></param>
        /// <returns></returns>
        public static Task<ISearchResult> SearchAsync(this IUpdateSearcher searcher, CancellationToken cancellationToken = default(CancellationToken)) => SearchAsync(searcher, null, cancellationToken);

        /// <summary>
        /// Find updates asynchronously
        /// </summary>
        /// <param name="searcher"></param>
        /// <param name="criteria"></param>
        /// <param name="cancellationToken"></param>
        /// <returns></returns>
        public static Task<ISearchResult> SearchAsync(this IUpdateSearcher searcher, string criteria, CancellationToken cancellationToken = default(CancellationToken))
        {
            var task = new TaskCompletionSource<ISearchResult>();
            if (cancellationToken.IsCancellationRequested)
            {
                task.TrySetCanceled(cancellationToken);
                return task.Task;
            }
            var job = null as ISearchJob;
            var reg = null as IDisposable;
            job = searcher.BeginSearch(criteria, new SearchCompletedCallback((_job, args) =>
            {
                try
                {
                    try
                    {
                        task.TrySetResult(searcher.EndSearch(_job));
                    }
                    catch (Exception e)
                    {
                        task.TrySetException(e);
                    }
                }
                finally
                {
                    job = null;
                    reg?.Dispose();
                }
            }), null);
            reg = cancellationToken.Register(() =>
            {
                task.TrySetCanceled(cancellationToken);
                job?.RequestAbort();
            });
            return task.Task;
        }

        /// <summary>
        /// Implementing callbacks when a search is complete
        /// </summary>
        internal class SearchCompletedCallback : ISearchCompletedCallback
        {
            Action<ISearchJob, ISearchCompletedCallbackArgs> Action;
            public SearchCompletedCallback(Action<ISearchJob, ISearchCompletedCallbackArgs> Action) => this.Action = Action;
            public void Invoke(ISearchJob searchJob, ISearchCompletedCallbackArgs callbackArgs) => Action?.Invoke(searchJob, callbackArgs);
        }
    }

    internal static class ToStringExtensions
    {
        internal static string ISearchResultToString(this ISearchResult s)
        {
            return $"{nameof(s.ResultCode)}:{s.ResultCode}";
        }

        internal static string IUpdateToString(this IUpdate u)
        {
            return $"{nameof(IUpdate)}{{{nameof(u.Title)}:\"{u.Title}\""
                + $", {nameof(u.KBArticleIDs)}:[{string.Join(", ", u.KBArticleIDs.Cast<string>())}]"
                + $", {nameof(u.IsDownloaded)}:{u.IsDownloaded}"
                + $", {nameof(u.IsInstalled)}:{u.IsInstalled}}}";
        }
    }
}
