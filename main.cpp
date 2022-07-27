#include <iostream>
#include <windows.h>
#include <stdio.h>
#include <sstream>
#include <string>
#include <tlhelp32.h>
using namespace std;

LONG (*NtSuspendProcess)(HANDLE) = 0;
LONG (*NtResumeProcess)(HANDLE) = 0;
LONG (*NtQuerySystemInformation)(int, PVOID, ULONG, PULONG) = 0;
DWORD* pidpointer;
HANDLE h;
char mc[] = "mainclass";
HWND lb;
HWND staticctrl;
HWND setbtn;
HWND refbtn;
HWND runbtn;
HWND killbtn;
int cursel;

//the following typedefs are just nonsense required to make undocumented windows calls cooperate.
typedef enum _SYSTEM_INFORMATION_CLASS
{
    SystemBasicInformation = 0,
    SystemProcessorInformation = 1,
    SystemPerformanceInformation = 2,
    SystemTimeOfDayInformation = 3,
    SystemProcessInformation = 5,
    SystemProcessorPerformanceInformation = 8,
    SystemHandleInformation = 16,
    SystemPagefileInformation = 18,
    SystemInterruptInformation = 23,
    SystemExceptionInformation = 33,
    SystemRegistryQuotaInformation = 37,
    SystemLookasideInformation = 45
}SYSTEM_INFORMATION_CLASS;
typedef struct _UNICODE_STRING
{
    USHORT Length;
    USHORT MaximumLength;
    PWSTR Buffer;
}UNICODE_STRING;
typedef LONG KPRIORITY;
typedef struct _CLIENT_ID
{
    HANDLE UniqueProcess;
    HANDLE UniqueThread;
}CLIENT_ID, *PCLIENT_ID;
typedef enum _KWAIT_REASON
{
    Executive = 0,
    FreePage, PageIn, PoolAllocation, DelayExecution,
    Suspended, UserRequest, WrExecutive, WrFreePage, WrPageIn,
    WrPoolAllocation, WrDelayExecution, WrSuspended,
    WrUserRequest, WrEventPair, WrQueue, WrLpcReceive,
    WrLpcReply, WrVirtualMemory, WrPageOut, WrRendezvous,
    Spare2, Spare3, Spare4, Spare5, Spare6, WrKernel,
    MaximumWaitReason
}KWAIT_REASON;
typedef struct _VM_COUNTERS
{
    SIZE_T PeakVirtualSize;
    SIZE_T VirtualSize;
    ULONG PageFaultCount;
    SIZE_T PeakWorkingSetSize;
    SIZE_T WorkingSetSize;
    SIZE_T QuotaPeakPagedPoolUsage;
    SIZE_T QuotaPagedPoolUsage;
    SIZE_T QuotaPeakNonPagedPoolUsage;
    SIZE_T QuotaNonPagedPoolUsage;
    SIZE_T PagefileUsage;
    SIZE_T PeakPagefileUsage;
}VM_COUNTERS, *PVM_COUNTERS;
typedef struct _SYSTEM_THREAD
{
  LARGE_INTEGER           KernelTime;
  LARGE_INTEGER           UserTime;
  LARGE_INTEGER           CreateTime;
  ULONG                   WaitTime;
  PVOID                   StartAddress;
  CLIENT_ID               ClientId;
  KPRIORITY               Priority;
  LONG                    BasePriority;
  ULONG                   ContextSwitchCount;
  ULONG                   State;
  KWAIT_REASON            WaitReason;
}SYSTEM_THREAD, *PSYSTEM_THREAD;
typedef struct _SYSTEM_PROCESS_INFORMATION
{
  ULONG                   NextEntryOffset;
  ULONG                   NumberOfThreads;
  LARGE_INTEGER           Reserved[3];
  LARGE_INTEGER           CreateTime;
  LARGE_INTEGER           UserTime;
  LARGE_INTEGER           KernelTime;
  UNICODE_STRING          ImageName;
  LONG                    BasePriority;
  HANDLE                  ProcessId;
  HANDLE                  InheritedFromProcessId;
  ULONG                   HandleCount;
  ULONG                   Reserved2[2];
  ULONG                   PrivatePageCount;
  VM_COUNTERS             VirtualMemoryCounters;
  IO_COUNTERS             IoCounters;
  SYSTEM_THREAD           Threads[0];
}SYSTEM_PROCESS_INFORMATION, *PSYSTEM_PROCESS_INFORMATION;

bool issuspended(DWORD pid) //checks if process with process id stored in DWORD pid is in a suspended state and returns 1 if suspended or 0 if not
{
    NtQuerySystemInformation = (LONG(*)(int,PVOID,ULONG,PULONG))GetProcAddress(GetModuleHandle("ntdll"), "NtQuerySystemInformation");
    ULONG len = 0;
    unsigned int result = NtQuerySystemInformation(5, 0, 0, &len);
    void* buffer = malloc(len);
    while(true)
    {
        result = NtQuerySystemInformation(5, buffer, len, &len);
        if(result== 0)
            break;
        else if(result == 3221225476)
        {
            free(buffer);
            buffer = malloc(len);
        }
        else
        {
            MessageBox(0, "fatal error #3. Are you running in admin?", "fatal error #3. Are you running in admin?", 0);
        }
    }
    DWORD* pointer = (DWORD*)buffer;
    restart:
    DWORD offset = *pointer;
    DWORD numofthreads = *(pointer+0x1);
    DWORD curpid = *(pointer+0x14);
    if(curpid != pid)
    {
        if(offset == 0)
        {
            free(buffer);
            return false;
        }
        void* vpointer = pointer;
        char* cpointer = (char*)vpointer;
        cpointer = cpointer+offset;
        vpointer = cpointer;
        pointer = (DWORD*)vpointer;
        goto restart;
    }
    void* vpointer = pointer;
    char* cpointer = (char*)vpointer;
    cpointer = cpointer+0x100;
    vpointer = cpointer;
    pointer = (DWORD*)vpointer;
    for(uint32_t i = 0;i<numofthreads;i++)
    {
        DWORD susr = *(pointer+0x11);
        DWORD waitr = *(pointer+0x12);
        if(susr != 0x5 || waitr != 0x5)
        {
            free(buffer);
            return false;
        }
    }
    free(buffer);
    return true;
}

void Refresh() //refreshes the list of running processes and finds all copies of steam.exe or multisteam.exe and updates the listbox with their process ID's
{
    while(true)
    {
        cursel = SendMessage(lb, LB_GETCURSEL, 0, 0);
        SendMessage(lb, LB_RESETCONTENT, 0, 0);
        DWORD pid = *pidpointer;
        ostringstream stream2;
        stream2 << pid;
        string line2 = stream2.str();
        SetWindowTextA(staticctrl, line2.c_str());
        PROCESSENTRY32 proc32;
        HANDLE hSnap;
        if(hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0))
        {
            if(hSnap == INVALID_HANDLE_VALUE)
            {
                MessageBox(0, "Fatal error #1. Did you run as administrator?", "Fatal error #1. Did you run as administrator?", 0);
            }
            else
            {
                proc32.dwSize = sizeof(PROCESSENTRY32);
                while((Process32Next(hSnap, &proc32)) == TRUE)
                {
                    if(strcmp(proc32.szExeFile, "multisteam.exe") == 0 || strcmp(proc32.szExeFile, "steam.exe") == 0)
                    {
                        ostringstream stream;
                        stream << proc32.th32ProcessID;
                        string line = stream.str();
                        int listitem = SendMessage (lb, LB_ADDSTRING, 0, (LPARAM)line.c_str());
                        SendMessage(lb, LB_SETITEMDATA, listitem, (LPARAM)line.c_str());
                    }
                }
                CloseHandle(hSnap);
            }
        }
        SendMessage(lb, LB_SETCURSEL, cursel, 0);
        Sleep(400);
    }
    return;
}

LRESULT CALLBACK mainproc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    switch (message)
    {
        case WM_COMMAND:
            switch(HIWORD(wparam))
            {
                case BN_CLICKED:
                    refstart:
                    if((HWND)lparam == setbtn || (HWND)lparam == killbtn)
                    {
                        int strindx = SendMessage(lb, LB_GETCURSEL, 0, 0);
                        char str[200];
                        memset(&str[0], 0, sizeof(str));
                        void* vpointer;
                        vpointer = &str;
                        SendMessage(lb, LB_GETTEXT, strindx, (LPARAM)vpointer);
                        string str2= str;
                        stringstream str3(str2);
                        int x = 0;
                        str3 >> x;
                        *pidpointer = x;
                    }
                    SendMessage(lb, LB_RESETCONTENT, 0, 0);
                    DWORD pid = *pidpointer;
                    ostringstream stream2;
                    stream2 << pid;
                    string line2 = stream2.str();
                    SetWindowTextA(staticctrl, line2.c_str());
                    PROCESSENTRY32 proc32;
                    HANDLE hSnap;
                    if(hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0))
                    {
                        if(hSnap == INVALID_HANDLE_VALUE)
                        {
                            MessageBox(0, "Fatal error #1. Did you run as administrator?", "Fatal error #1. Did you run as administrator?", 0);
                        }
                        else
                        {
                            proc32.dwSize = sizeof(PROCESSENTRY32);
                            while((Process32Next(hSnap, &proc32)) == TRUE)
                            {
                                if(strcmp(proc32.szExeFile, "multisteam.exe") == 0 || strcmp(proc32.szExeFile, "steam.exe") == 0)
                                {
                                    ostringstream stream;
                                    stream << proc32.th32ProcessID;
                                    if((HWND)lparam == runbtn)
                                    {
                                        h = OpenProcess(PROCESS_ALL_ACCESS, TRUE, proc32.th32ProcessID);
                                        if(!h)
                                        {
                                            MessageBox(0, "Fatal error #2. Did you run as administrator?", "Fatal error #2. Did you run as administrator?", 0);
                                        }
                                        while(issuspended(proc32.th32ProcessID))
                                            NtResumeProcess(h);
                                        CloseHandle(h);
                                    }
                                    if((HWND)lparam == setbtn)
                                    {
                                        h = OpenProcess(PROCESS_ALL_ACCESS, TRUE, proc32.th32ProcessID);
                                        if(!h)
                                        {
                                            MessageBox(0, "Fatal error #2. Did you run as administrator?", "Fatal error #2. Did you run as administrator?", 0);
                                        }
                                        if(proc32.th32ProcessID == pid)
                                        {
                                            while(issuspended(proc32.th32ProcessID))
                                                NtResumeProcess(h);
                                        }
                                        else
                                        {
                                            while(!issuspended(proc32.th32ProcessID))
                                                NtSuspendProcess(h);
                                        }
                                        CloseHandle(h);
                                    }
                                    if((HWND)lparam == killbtn && proc32.th32ProcessID == pid)
                                    {
                                        h = OpenProcess(PROCESS_ALL_ACCESS, TRUE, proc32.th32ProcessID);
                                        if(!h)
                                            MessageBox(0, "Fatal error #2. Did you run as administrator?", "Fatal error #2. Did you run as administrator?", 0);
                                        TerminateProcess(h, -1);
                                        CloseHandle(h);
                                        lparam = (LPARAM)refbtn;
                                        goto refstart;
                                    }
                                    string line = stream.str();
                                    int listitem = SendMessage (lb, LB_ADDSTRING, 0, (LPARAM)line.c_str());
                                    SendMessage(lb, LB_SETITEMDATA, listitem, (LPARAM)line.c_str());
                                }
                            }
                            CloseHandle(hSnap);
                        }
                    }
                    break;
            }
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hwnd, message, wparam, lparam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hthisinst, HINSTANCE hprevinst, LPSTR lpszArgument, int nCmdShow)
{
    NtSuspendProcess = (LONG(*)(HANDLE))GetProcAddress(GetModuleHandle("ntdll"), "NtSuspendProcess");
    NtResumeProcess = (LONG(*)(HANDLE))GetProcAddress(GetModuleHandle("ntdll"), "NtResumeProcess");
    HWND mainwin;
    MSG messages;
    WNDCLASSEX wc;
    wc.hInstance = hthisinst;
    wc.lpszClassName = mc;
    wc.lpfnWndProc = mainproc;
    wc.style = CS_DBLCLKS;
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszMenuName = NULL;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hbrBackground = (HBRUSH)COLOR_BACKGROUND;
    if(!RegisterClassEx(&wc))
        return 0;
    mainwin = CreateWindowEx(0, mc, "main window", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 700, 500, HWND_DESKTOP, NULL, hthisinst, NULL);
    lb = CreateWindowEx (0, "listbox", 0, WS_CHILD|WS_VISIBLE|LBS_NOTIFY|LBS_HASSTRINGS|WS_VSCROLL|WS_BORDER, 20, 40, 240, 220, mainwin, 0, 0, 0);
    refbtn = CreateWindowEx (0, "button", 0, WS_CHILD|WS_VISIBLE, 20, 300, 80, 20, mainwin, 0, 0, 0);
    setbtn = CreateWindowEx (0, "button", 0, WS_CHILD|WS_VISIBLE, 140, 300, 80, 20, mainwin, 0, 0, 0);
    runbtn = CreateWindowEx (0, "button", 0, WS_CHILD|WS_VISIBLE, 20, 350, 80, 20, mainwin, 0, 0, 0);
    killbtn = CreateWindowEx (0, "button", 0, WS_CHILD|WS_VISIBLE, 140, 350, 80, 20, mainwin, 0, 0, 0);
    staticctrl = CreateWindowEx(0, "STATIC", 0, WS_CHILD|WS_VISIBLE, 300, 40, 124, 25, mainwin, 0, 0, 0);
    LPCTSTR pBuf;
    SECURITY_ATTRIBUTES secat;
    secat.nLength = 0xC;
    secat.bInheritHandle = 0;
    secat.lpSecurityDescriptor = 0;
    HANDLE fmh = CreateFileMapping((HANDLE) -1, &secat, 4, 0, 0x400, "Steam3Master_SharedMemFile");
    pBuf = (LPTSTR)MapViewOfFile(fmh, 0xF001F , 0, 0, 0);
    void* vpointer;
    vpointer = (void*)(pBuf+8);
    pidpointer = (DWORD*)vpointer;
    DWORD pid = *pidpointer;
    ostringstream stream;
    stream << pid;
    string line = stream.str();
    SetWindowTextA(staticctrl, line.c_str());
    SetWindowTextA(refbtn, "Refresh");
    SetWindowTextA(runbtn, "RunAll");
    SetWindowTextA(setbtn, "Set");
    SetWindowTextA(killbtn, "Kill");
    PROCESSENTRY32 proc32;
    HANDLE hSnap;
    if(hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0))
    {
        if(hSnap == INVALID_HANDLE_VALUE)
        {
            MessageBox(0, "Fatal error #1. Did you run as administrator?", "Fatal error #1. Did you run as administrator?", 0);
        }
        else
        {
            proc32.dwSize = sizeof(PROCESSENTRY32);
            while((Process32Next(hSnap, &proc32)) == TRUE)
            {
                if(strcmp(proc32.szExeFile, "multisteam.exe") == 0 || strcmp(proc32.szExeFile, "steam.exe") == 0)
                {
                    ostringstream stream2;
                    stream2 << proc32.th32ProcessID;
                    string line2 = stream2.str();
                    int listitem = SendMessage (lb, LB_ADDSTRING, 0, (LPARAM)line2.c_str());
                    SendMessage(lb, LB_SETITEMDATA, listitem, (LPARAM)line2.c_str());
                }
            }
            CloseHandle(hSnap);
        }
    }
    ShowWindow(mainwin, nCmdShow);
    CreateThread(0,0,(LPTHREAD_START_ROUTINE)&Refresh,0,0,0);
    while(GetMessage(&messages, NULL, 0, 0))
    {
        TranslateMessage(&messages);
        DispatchMessage(&messages);
    }
    return messages.wParam;
}
