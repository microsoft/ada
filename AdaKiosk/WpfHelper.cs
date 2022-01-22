using System;
using System.Collections.Generic;
using System.Windows;
using System.Windows.Media;

namespace AdaKiosk
{

    public static class WpfHelper
    {
        /// <summary>
        /// Finds an ancestor of a given type starting with a given element in the visual tree.
        /// </summary>
        /// <typeparam name="T">The type of the item we are looking for.</typeparam>
        /// <param name="child">A direct or indirect child of the
        /// queried item.</param>
        /// <returns>The first parent item that matches the submitted
        /// type parameter. If not matching item can be found, a null
        /// reference is being returned.</returns>
        public static T FindAncestor<T>(this DependencyObject current) where T : DependencyObject
        {
            do
            {
                //check if the current matches the type we're looking for
                T parent = current as T;
                if (parent != null)
                {
                    return parent;
                }

                //get parent item
                current = GetParentObject(current);
            }
            while (current != null);

            return null;
        }

        /// <summary>
        /// This method is an alternative to WPF's
        /// <see cref="VisualTreeHelper.GetParent"/> method, which also
        /// supports content elements. Keep in mind that for content element,
        /// this method falls back to the logical tree of the element!
        /// </summary>
        /// <param name="child">The item to be processed.</param>
        /// <returns>The submitted item's parent, if available. Otherwise
        /// null.</returns>
        public static DependencyObject GetParentObject(this DependencyObject child)
        {
            if (child == null) return null;

            //handle content elements separately
            ContentElement contentElement = child as ContentElement;
            if (contentElement != null)
            {
                DependencyObject parent = ContentOperations.GetParent(contentElement);
                if (parent != null) return parent;

                FrameworkContentElement fce = contentElement as FrameworkContentElement;
                return fce != null ? fce.Parent : null;
            }

            //also try searching for parent in framework elements (such as DockPanel, etc)
            FrameworkElement frameworkElement = child as FrameworkElement;
            if (frameworkElement != null)
            {
                DependencyObject parent = frameworkElement.Parent;
                if (parent != null) return parent;
            }

            //if it's not a ContentElement/FrameworkElement, rely on VisualTreeHelper
            try
            {
                return VisualTreeHelper.GetParent(child);
            }
            catch
            {
                // exception is thrown if child is a FlowDocument "Inline" object (it is not a Visual).
                return null;
            }
        }


        public static DependencyObject GetAncestorObject(this DependencyObject child, Type ofType)
        {
            DependencyObject p = GetParentObject(child);
            if (p != null)
            {
                if (p.DependencyObjectType.SystemType == ofType)
                {
                    return p;
                }
                else
                {
                    return GetAncestorObject(p, ofType);
                }
            }
            return null;
        }

        // Enumerate all the descendants of the visual object.
        static public FrameworkElement FindDescendantElement(this Visual myVisual, string nameId)
        {
            for (int i = 0; i < VisualTreeHelper.GetChildrenCount(myVisual); i++)
            {
                // Retrieve child visual at specified index value.
                Visual childVisual = (Visual)VisualTreeHelper.GetChild(myVisual, i);

                FrameworkElement fe = childVisual as FrameworkElement;
                if (fe != null && fe.Name == nameId)
                {
                    return fe;
                }


                // Enumerate children of the child visual object.
                FrameworkElement nextChild = FindDescendantElement(childVisual, nameId);
                if (nextChild != null)
                {
                    return nextChild;
                }
            }
            return null;
        }

        /// <summary>
        /// Find all child elements of a given type.
        /// </summary>
        private static void GetDescendantsOfType<T>(this DependencyObject parent, List<T> found) where T : DependencyObject
        {
            if (typeof(T).IsAssignableFrom(parent.GetType()))
            {
                found.Add((T)parent);
            }
            for (int i = 0, n = VisualTreeHelper.GetChildrenCount(parent); i < n; i++)
            {
                DependencyObject child = VisualTreeHelper.GetChild(parent, i);
                GetDescendantsOfType<T>(child, found);
            }
        }

        /// <summary>
        /// Find all child elements of a given type.
        /// </summary>
        public static IEnumerable<T> FindDescendantsOfType<T>(this DependencyObject parent) where T : DependencyObject
        {
            List<T> found = new List<T>();
            parent.GetDescendantsOfType<T>(found);
            return found;
        }

        /// <summary>
        /// Find any child element of a given type.
        /// </summary>
        public static T FindFirstDescendantOfType<T>(this DependencyObject parent) where T : DependencyObject
        {
            if (typeof(T).IsAssignableFrom(parent.GetType()))
            {
                return (T)parent;
            }
            for (int i = 0, n = VisualTreeHelper.GetChildrenCount(parent); i < n; i++)
            {
                DependencyObject child = VisualTreeHelper.GetChild(parent, i);
                T result = FindFirstDescendantOfType<T>(child);
                if (result != null)
                {
                    return result;
                }
            }
            return null;
        }

        public static List<T> FindChildren<T>(this Visual parent, Point hitPoint, double radius = 10) where T : DependencyObject
        {
            var expandedHitTestArea = new GeometryHitTestParameters(new EllipseGeometry(hitPoint, radius, radius));

            List<T> result = new List<T>();

            // The following callback is called for every visual intersecting with the test area, in reverse z-index order.
            // If hitLinks is true, all links passing through the test area are considered, and the closest to the hitPoint is returned in visual.
            // The hit test search is stopped as soon as the first non-link graph object is encountered.
            var hitTestResultCallback = new HitTestResultCallback(r =>
            {
                T lc = r.VisualHit.FindAncestor<T>();
                if (lc != null)
                {
                    result.Add(lc);
                }
                return HitTestResultBehavior.Continue;
            });

            // start the search
            VisualTreeHelper.HitTest(parent, null, hitTestResultCallback, expandedHitTestArea);
            return result;
        }

    }
}
