﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Security;
using System.Text;
using System.Threading.Tasks;

namespace AdaKioskService
{
    internal class ApplicationLauncher
    {
        /// <summary>
        /// butbut: for some reason this is not working, the service does not have permission saying
        /// "The application-specific permission settings do not grant Local Launch permission..."
        /// So we are currently not using this, instead we force a reboot which has the same effect 
        /// of restarting the Kiosk app and I refuse to mess around with ServiceBroker ACL's.
        /// </summary>
        public static bool CreateProcessInConsoleSession(ServiceLog log, String commandLine, bool bElevate)
        {
            PROCESS_INFORMATION pi;
            bool bResult = false;
            uint dwSessionId, winlogonPid = 0;
            IntPtr hUserToken = IntPtr.Zero, hUserTokenDup = IntPtr.Zero, hPToken = IntPtr.Zero, hProcess = IntPtr.Zero;

            // Log the client on to the local computer.
            dwSessionId = WTSGetActiveConsoleSessionId();

            // Find the winlogon process
            var procEntry = new PROCESSENTRY32();

            uint hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
            if (hSnap == INVALID_HANDLE_VALUE)
            {
                return false;
            }

            procEntry.dwSize = (uint)Marshal.SizeOf(procEntry); //sizeof(PROCESSENTRY32);

            if (Process32First(hSnap, ref procEntry) == 0)
            {
                return false;
            }

            String strCmp = "explorer.exe";
            do
            {
                if (strCmp.IndexOf(procEntry.szExeFile) == 0)
                {
                    // We found a winlogon process...make sure it's running in the console session
                    uint winlogonSessId = 0;
                    if (ProcessIdToSessionId(procEntry.th32ProcessID, out winlogonSessId) &&
                        winlogonSessId == dwSessionId)
                    {
                        winlogonPid = procEntry.th32ProcessID;
                        break;
                    }
                    else
                    {
                        var hr = Marshal.GetLastWin32Error();
                        log.WriteMessage(String.Format("ProcessIdToSessionId error: {0}", hr));
                    }
                }
            }
            while (Process32Next(hSnap, ref procEntry) != 0);

            //Get the user token used by DuplicateTokenEx
            WTSQueryUserToken(dwSessionId, out hUserToken);

            var si = new STARTUPINFO();
            si.cb = Marshal.SizeOf(si);
            si.lpDesktop = "winsta0\\default";
            var tp = new TOKEN_PRIVILEGES();
            var luid = new LUID();
            hProcess = OpenProcess(MAXIMUM_ALLOWED, false, winlogonPid);

            if (!OpenProcessToken(hProcess,
                    TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY | TOKEN_DUPLICATE | TOKEN_ASSIGN_PRIMARY
                    | TOKEN_ADJUST_SESSIONID | TOKEN_READ | TOKEN_WRITE, ref hPToken))
            {
                var hr = Marshal.GetLastWin32Error();
                log.WriteMessage(String.Format("CreateProcessInConsoleSession OpenProcessToken error: {0}", hr));
            }

            if (!LookupPrivilegeValue(IntPtr.Zero, SE_TCB_NAME, ref luid))
            {
                var hr = Marshal.GetLastWin32Error();
                log.WriteMessage(String.Format("CreateProcessInConsoleSession LookupPrivilegeValue error: {0}", hr));
            }

            var sa = new SECURITY_ATTRIBUTES();
            sa.Length = Marshal.SizeOf(sa);

            if (!DuplicateTokenEx(hPToken, MAXIMUM_ALLOWED, ref sa,
                    (int)SECURITY_IMPERSONATION_LEVEL.SecurityIdentification, (int)TOKEN_TYPE.TokenPrimary,
                    ref hUserTokenDup))
            {
                var hr = Marshal.GetLastWin32Error();
                log.WriteMessage("CreateProcessInConsoleSession DuplicateTokenEx error: {0} Token does not have the privilege.", hr);
                CloseHandle(hProcess);
                CloseHandle(hUserToken);
                CloseHandle(hPToken);
                return false;
            }

            if (bElevate)
            {
                //tp.Privileges[0].Luid = luid;
                //tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

                tp.PrivilegeCount = 1;
                tp.Privileges = new int[3];
                tp.Privileges[2] = SE_PRIVILEGE_ENABLED;
                tp.Privileges[1] = luid.HighPart;
                tp.Privileges[0] = luid.LowPart;

                //Adjust Token privilege
                if (!SetTokenInformation(hUserTokenDup, TOKEN_INFORMATION_CLASS.TokenSessionId, ref dwSessionId,
                        (uint)IntPtr.Size))
                {
                    var hr = Marshal.GetLastWin32Error();
                    log.WriteMessage(
                            "CreateProcessInConsoleSession SetTokenInformation error: {0} Token does not have the privilege.", hr);
                    //CloseHandle(hProcess);
                    //CloseHandle(hUserToken);
                    //CloseHandle(hPToken);
                    //CloseHandle(hUserTokenDup);
                    //return false;
                }

                if (!AdjustTokenPrivileges(hUserTokenDup, false, ref tp, Marshal.SizeOf(tp), /*(PTOKEN_PRIVILEGES)*/
                        IntPtr.Zero, IntPtr.Zero))
                {
                    int nErr = Marshal.GetLastWin32Error();

                    if (nErr == ERROR_NOT_ALL_ASSIGNED)
                    {
                        log.WriteMessage(
                                "CreateProcessInConsoleSession AdjustTokenPrivileges error: {0} Token does not have the privilege.",
                                nErr);
                    }
                    else
                    {
                        log.WriteMessage(String.Format("CreateProcessInConsoleSession AdjustTokenPrivileges error: {0}", nErr));
                    }
                }
            }

            uint dwCreationFlags = NORMAL_PRIORITY_CLASS | CREATE_NEW_CONSOLE;
            IntPtr pEnv = IntPtr.Zero;
            if (CreateEnvironmentBlock(ref pEnv, hUserTokenDup, true))
            {
                dwCreationFlags |= CREATE_UNICODE_ENVIRONMENT;
            }
            else
            {
                pEnv = IntPtr.Zero;
            }

            // Launch the process in the client's logon session.
            bResult = CreateProcessAsUser(hUserTokenDup, // client's access token
                null, // file to execute
                commandLine, // command line
                ref sa, // pointer to process SECURITY_ATTRIBUTES
                ref sa, // pointer to thread SECURITY_ATTRIBUTES
                false, // handles are not inheritable
                (int)dwCreationFlags, // creation flags
                pEnv, // pointer to new environment block 
                null, // name of current directory 
                ref si, // pointer to STARTUPINFO structure
                out pi // receives information about new process
                );
            // End impersonation of client.

            //GetLastError should be 0
            int iResultOfCreateProcessAsUser = Marshal.GetLastWin32Error();

            //Close handles task
            CloseHandle(hProcess);
            CloseHandle(hUserToken);
            CloseHandle(hUserTokenDup);
            CloseHandle(hPToken);

            return (iResultOfCreateProcessAsUser == 0) ? true : false;
        }

        public static bool TerminateProcessInUserSession(ServiceLog log, String processName, bool bElevate)
        {
            bool bResult = false;
            uint dwSessionId = 0, winlogonPid = 0;
            IntPtr hProcess = IntPtr.Zero;

            // Log the client on to the local computer.
            dwSessionId = WTSGetActiveConsoleSessionId();
            IntPtr phToken = IntPtr.Zero;
            if (!WTSQueryUserToken(dwSessionId, out phToken))
            {
                var hr = Marshal.GetLastWin32Error();
                log.WriteMessage(String.Format("WTSQueryUserToken error: {0}", hr));
            }

            // Find the winlogon process
            var procEntry = new PROCESSENTRY32();

            uint hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
            if (hSnap == INVALID_HANDLE_VALUE)
            {
                return false;
            }

            procEntry.dwSize = (uint)Marshal.SizeOf(procEntry); //sizeof(PROCESSENTRY32);

            if (Process32First(hSnap, ref procEntry) == 0)
            {
                return false;
            }

            do
            {
                string filename = System.IO.Path.GetFileName(procEntry.szExeFile);
                if (string.Compare(filename, processName, StringComparison.OrdinalIgnoreCase) == 0)
                {
                    // We found a winlogon process...make sure it's running in the console session
                    uint winlogonSessId = 0;
                    if (ProcessIdToSessionId(procEntry.th32ProcessID, out winlogonSessId) &&
                        winlogonSessId == dwSessionId)
                    {
                        winlogonPid = procEntry.th32ProcessID;
                        break;
                    }
                    else
                    {
                        var hr = Marshal.GetLastWin32Error();
                        log.WriteMessage(String.Format("ProcessIdToSessionId error: {0}", hr));
                    }
                }
            }
            while (Process32Next(hSnap, ref procEntry) != 0);

            if (winlogonPid == 0)
            {
                // not found
                return false;
            }

            hProcess = OpenProcess(MAXIMUM_ALLOWED, false, winlogonPid);

            if (!TerminateProcess(hProcess, 1))
            {
                int nErr = Marshal.GetLastWin32Error();
                log.WriteMessage(String.Format("TerminateProcess error: {0}", nErr));
                bResult = false;
            }
            else
            {
                bResult = true;
            }

            //Close handles task
            CloseHandle(hProcess);

            return bResult;
        }

        #region Native Methods

        [DllImport("Shell32.dll", EntryPoint = "ShellExecuteW",
            SetLastError = true, CharSet = CharSet.Unicode, ExactSpelling = true,
            CallingConvention = CallingConvention.StdCall)]
        public static extern int ShellExecute(IntPtr handle, string verb, string file, string args, string dir, int show);

        public enum TOKEN_INFORMATION_CLASS
        {
            TokenUser = 1,
            TokenGroups,
            TokenPrivileges,
            TokenOwner,
            TokenPrimaryGroup,
            TokenDefaultDacl,
            TokenSource,
            TokenType,
            TokenImpersonationLevel,
            TokenStatistics,
            TokenRestrictedSids,
            TokenSessionId,
            TokenGroupsAndPrivileges,
            TokenSessionReference,
            TokenSandBoxInert,
            TokenAuditPolicy,
            TokenOrigin,
            MaxTokenInfoClass // MaxTokenInfoClass should always be the last enum
        }

        public const int READ_CONTROL = 0x00020000;

        public const int STANDARD_RIGHTS_REQUIRED = 0x000F0000;

        public const int STANDARD_RIGHTS_READ = READ_CONTROL;
        public const int STANDARD_RIGHTS_WRITE = READ_CONTROL;
        public const int STANDARD_RIGHTS_EXECUTE = READ_CONTROL;

        public const int STANDARD_RIGHTS_ALL = 0x001F0000;

        public const int SPECIFIC_RIGHTS_ALL = 0x0000FFFF;

        public const int TOKEN_ASSIGN_PRIMARY = 0x0001;
        public const int TOKEN_DUPLICATE = 0x0002;
        public const int TOKEN_IMPERSONATE = 0x0004;
        public const int TOKEN_QUERY = 0x0008;
        public const int TOKEN_QUERY_SOURCE = 0x0010;
        public const int TOKEN_ADJUST_PRIVILEGES = 0x0020;
        public const int TOKEN_ADJUST_GROUPS = 0x0040;
        public const int TOKEN_ADJUST_DEFAULT = 0x0080;
        public const int TOKEN_ADJUST_SESSIONID = 0x0100;

        public const int TOKEN_ALL_ACCESS_P = (STANDARD_RIGHTS_REQUIRED |
                                               TOKEN_ASSIGN_PRIMARY |
                                               TOKEN_DUPLICATE |
                                               TOKEN_IMPERSONATE |
                                               TOKEN_QUERY |
                                               TOKEN_QUERY_SOURCE |
                                               TOKEN_ADJUST_PRIVILEGES |
                                               TOKEN_ADJUST_GROUPS |
                                               TOKEN_ADJUST_DEFAULT);

        public const int TOKEN_ALL_ACCESS = TOKEN_ALL_ACCESS_P | TOKEN_ADJUST_SESSIONID;

        public const int TOKEN_READ = STANDARD_RIGHTS_READ | TOKEN_QUERY;

        public const int TOKEN_WRITE = STANDARD_RIGHTS_WRITE |
                                       TOKEN_ADJUST_PRIVILEGES |
                                       TOKEN_ADJUST_GROUPS |
                                       TOKEN_ADJUST_DEFAULT;

        public const int TOKEN_EXECUTE = STANDARD_RIGHTS_EXECUTE;

        public const uint MAXIMUM_ALLOWED = 0x2000000;

        public const int CREATE_NEW_PROCESS_GROUP = 0x00000200;
        public const int CREATE_UNICODE_ENVIRONMENT = 0x00000400;

        public const int IDLE_PRIORITY_CLASS = 0x40;
        public const int NORMAL_PRIORITY_CLASS = 0x20;
        public const int HIGH_PRIORITY_CLASS = 0x80;
        public const int REALTIME_PRIORITY_CLASS = 0x100;

        public const int CREATE_NEW_CONSOLE = 0x00000010;

        public const string SE_TCB_NAME = "SeTcbPrivilege";
        public const string SE_DEBUG_NAME = "SeDebugPrivilege";
        public const string SE_RESTORE_NAME = "SeRestorePrivilege";
        public const string SE_BACKUP_NAME = "SeBackupPrivilege";

        public const int SE_PRIVILEGE_ENABLED = 0x0002;

        public const int ERROR_NOT_ALL_ASSIGNED = 1300;

        private const uint TH32CS_SNAPPROCESS = 0x00000002;

        public static int INVALID_HANDLE_VALUE = -1;

        [DllImport("advapi32.dll", SetLastError = true)]
        public static extern bool LookupPrivilegeValue(IntPtr lpSystemName, string lpname,
            [MarshalAs(UnmanagedType.Struct)] ref LUID lpLuid);

        [DllImport("advapi32.dll", EntryPoint = "CreateProcessAsUser", SetLastError = true, CharSet = CharSet.Auto,
            CallingConvention = CallingConvention.StdCall)]
        public static extern bool CreateProcessAsUser(IntPtr hToken, String lpApplicationName, String lpCommandLine,
            ref SECURITY_ATTRIBUTES lpProcessAttributes,
            ref SECURITY_ATTRIBUTES lpThreadAttributes, bool bInheritHandle, int dwCreationFlags, IntPtr lpEnvironment,
            String lpCurrentDirectory, ref STARTUPINFO lpStartupInfo, out PROCESS_INFORMATION lpProcessInformation);


        [DllImport("advapi32.dll", EntryPoint = "CreateProcessWithLogon", SetLastError = true, CharSet = CharSet.Auto,
            CallingConvention = CallingConvention.StdCall)]
        public static extern bool CreateProcessWithLogon(string lpUsername, string lpDomain, string lpPassword, int dwLogonFlags,
            String lpApplicationName, String lpCommandLine, int dwCreationFlags, IntPtr lpEnvironment,
            String lpCurrentDirectory, ref STARTUPINFO lpStartupInfo, out PROCESS_INFORMATION lpProcessInformation);

        [DllImport("advapi32.dll", CharSet = CharSet.Auto, SetLastError = true)]
        public static extern bool DuplicateToken(IntPtr ExistingTokenHandle,
            int SECURITY_IMPERSONATION_LEVEL, ref IntPtr DuplicateTokenHandle);

        [DllImport("advapi32.dll", EntryPoint = "DuplicateTokenEx")]
        public static extern bool DuplicateTokenEx(IntPtr ExistingTokenHandle, uint dwDesiredAccess,
            ref SECURITY_ATTRIBUTES lpThreadAttributes, int TokenType,
            int ImpersonationLevel, ref IntPtr DuplicateTokenHandle);

        [DllImport("advapi32.dll", SetLastError = true)]
        public static extern bool AdjustTokenPrivileges(IntPtr TokenHandle, bool DisableAllPrivileges,
            ref TOKEN_PRIVILEGES NewState, int BufferLength, IntPtr PreviousState, IntPtr ReturnLength);

        [DllImport("advapi32.dll", SetLastError = true)]
        public static extern bool SetTokenInformation(IntPtr TokenHandle, TOKEN_INFORMATION_CLASS TokenInformationClass,
            ref uint TokenInformation, uint TokenInformationLength);

        [DllImport("userenv.dll", SetLastError = true)]
        public static extern bool CreateEnvironmentBlock(ref IntPtr lpEnvironment, IntPtr hToken, bool bInherit);

        [DllImport("kernel32.dll")]
        private static extern int Process32First(uint hSnapshot, ref PROCESSENTRY32 lppe);

        [DllImport("kernel32.dll")]
        private static extern int Process32Next(uint hSnapshot, ref PROCESSENTRY32 lppe);

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern uint CreateToolhelp32Snapshot(uint dwFlags, uint th32ProcessID);

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern bool CloseHandle(IntPtr hSnapshot);

        [DllImport("kernel32.dll")]
        private static extern uint WTSGetActiveConsoleSessionId();

        [DllImport("Wtsapi32.dll")]
        private static extern bool WTSQueryUserToken(uint SessionId, out IntPtr phToken);

        [DllImport("kernel32.dll")]
        private static extern bool ProcessIdToSessionId(uint dwProcessId, out uint pSessionId);

        [DllImport("kernel32.dll")]
        private static extern IntPtr OpenProcess(uint dwDesiredAccess, bool bInheritHandle, uint dwProcessId);

        [DllImport("advapi32", SetLastError = true)]
        [SuppressUnmanagedCodeSecurity]
        private static extern bool OpenProcessToken(IntPtr ProcessHandle, // handle to process
            int DesiredAccess, // desired access to process
            ref IntPtr TokenHandle);

        [DllImport("kernel32.dll")]
        private static extern bool TerminateProcess(IntPtr hProcess, uint exitCode);

        #region Nested type: LUID

        [StructLayout(LayoutKind.Sequential)]
        internal struct LUID
        {
            public int LowPart;
            public int HighPart;
        }

        #endregion

        //end struct

        #region Nested type: LUID_AND_ATRIBUTES

        [StructLayout(LayoutKind.Sequential)]
        internal struct LUID_AND_ATRIBUTES
        {
            public LUID Luid;
            public int Attributes;
        }

        #endregion

        #region Nested type: PROCESSENTRY32

        [StructLayout(LayoutKind.Sequential)]
        private struct PROCESSENTRY32
        {
            public uint dwSize;
            public readonly uint cntUsage;
            public readonly uint th32ProcessID;
            public readonly IntPtr th32DefaultHeapID;
            public readonly uint th32ModuleID;
            public readonly uint cntThreads;
            public readonly uint th32ParentProcessID;
            public readonly int pcPriClassBase;
            public readonly uint dwFlags;

            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 260)]
            public readonly string szExeFile;
        }

        #endregion

        #region Nested type: PROCESS_INFORMATION

        [StructLayout(LayoutKind.Sequential)]
        public struct PROCESS_INFORMATION
        {
            public IntPtr hProcess;
            public IntPtr hThread;
            public uint dwProcessId;
            public uint dwThreadId;
        }

        #endregion

        #region Nested type: SECURITY_ATTRIBUTES

        [StructLayout(LayoutKind.Sequential)]
        public struct SECURITY_ATTRIBUTES
        {
            public int Length;
            public IntPtr lpSecurityDescriptor;
            public bool bInheritHandle;
        }

        #endregion

        #region Nested type: SECURITY_IMPERSONATION_LEVEL

        private enum SECURITY_IMPERSONATION_LEVEL
        {
            SecurityAnonymous = 0,
            SecurityIdentification = 1,
            SecurityImpersonation = 2,
            SecurityDelegation = 3,
        }

        #endregion

        #region Nested type: STARTUPINFO

        [StructLayout(LayoutKind.Sequential)]
        public struct STARTUPINFO
        {
            public int cb;
            public String lpReserved;
            public String lpDesktop;
            public String lpTitle;
            public uint dwX;
            public uint dwY;
            public uint dwXSize;
            public uint dwYSize;
            public uint dwXCountChars;
            public uint dwYCountChars;
            public uint dwFillAttribute;
            public uint dwFlags;
            public short wShowWindow;
            public short cbReserved2;
            public IntPtr lpReserved2;
            public IntPtr hStdInput;
            public IntPtr hStdOutput;
            public IntPtr hStdError;
        }

        #endregion

        #region Nested type: TOKEN_PRIVILEGES

        [StructLayout(LayoutKind.Sequential)]
        internal struct TOKEN_PRIVILEGES
        {
            internal int PrivilegeCount;
            //LUID_AND_ATRIBUTES
            [MarshalAs(UnmanagedType.ByValArray, SizeConst = 3)]
            internal int[] Privileges;
        }

        #endregion

        #region Nested type: TOKEN_TYPE

        private enum TOKEN_TYPE
        {
            TokenPrimary = 1,
            TokenImpersonation = 2
        }

        #endregion

        #endregion 
        // handle to open access token
    }
}
