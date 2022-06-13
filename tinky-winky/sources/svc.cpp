#pragma warning(disable:4668 4577)

#define SVCNAME TEXT("tinky")

#include <windows.h>
#include <tchar.h>
#include <strsafe.h>
#include <string.h>
#include <iostream>
#include <windows.h>
#include <process.h>
#include <Tlhelp32.h>
#include <winbase.h>
#include <string.h>
#include <comdef.h>

#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "User32.lib")

void killProcessByName(const char* filename)
{
    HANDLE hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPALL, NULL);
    PROCESSENTRY32 pEntry;
    pEntry.dwSize = sizeof(pEntry);
    BOOL hRes = Process32First(hSnapShot, &pEntry);
    while (hRes)
    {
        _bstr_t StrSwap(pEntry.szExeFile);
        const char* ProccessFilename = StrSwap;
        if (strcmp(ProccessFilename, filename) == 0)
        {
            HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, 0,
                (DWORD)pEntry.th32ProcessID);
            if (hProcess != NULL)
            {
                TerminateProcess(hProcess, 9);
                CloseHandle(hProcess);
            }
        }
        hRes = Process32Next(hSnapShot, &pEntry);
    }
    CloseHandle(hSnapShot);
}

int SvcDelete()
{
    SC_HANDLE schSCManager;
    SC_HANDLE schService;

    // Get a handle to the SCM database. 

    schSCManager = OpenSCManager(
        NULL,                    // local computer
        NULL,                    // ServicesActive database 
        SC_MANAGER_ALL_ACCESS);  // full access rights 

    if (NULL == schSCManager)
    {
        printf("OpenSCManager failed (%lu)\n", GetLastError());
        return (-1);
    }

    // Get a handle to the service.

    schService = OpenService(
        schSCManager,       // SCM database 
        SVCNAME,          // name of service 
        DELETE);            // need delete access 

    if (schService == NULL)
    {
        if (GetLastError() == ERROR_SERVICE_EXISTS)
        {
            printf("OpenService failed (%lu)\n", GetLastError());
        }
        else if (GetLastError() != ERROR_SERVICE_EXISTS)
        {
            printf("service not exists: %lu\n", GetLastError());
        }
        CloseServiceHandle(schSCManager);
        return (-1);
    }

    // Delete the service.

    if (!DeleteService(schService))
    {
        printf("DeleteService failed (%lu)\n", GetLastError());
        return (-1);
    }
    else printf("Service {%s} deleted successfully.\n", SVCNAME);

    CloseServiceHandle(schService);
    CloseServiceHandle(schSCManager);
    return (0);
}

int SvcInstall()
{
    SC_HANDLE schSCManager;
    SC_HANDLE schService;

    char szPath[MAX_PATH];


    if (!GetCurrentDirectory(MAX_PATH, szPath))
    {
        printf("Cannot install service (%lu)\n", GetLastError());
        return (-1);
    }
    strcat_s(szPath, "\\tinky.exe");
    // Get a handle to the SCM database. 

    schSCManager = OpenSCManager(
        NULL,                    // local computer
        NULL,                    // ServicesActive database 
        SC_MANAGER_ALL_ACCESS);  // full access rights 

    if (NULL == schSCManager)
    {
        printf("OpenSCManager failed (%lu)\n", GetLastError());
        return (-1);
    }

    // Create the service

    schService = CreateService(
        schSCManager,              // SCM database 
        SVCNAME,                   // name of service 
        SVCNAME,                   // service name to display 
        SERVICE_ALL_ACCESS,        // desired access 
        SERVICE_WIN32_OWN_PROCESS, // service type 
        SERVICE_DEMAND_START,      // start type //SERVICE_AUTO_START
        SERVICE_ERROR_NORMAL,      // error control type 
        szPath,                    // path to service's binary 
        NULL,                      // no load ordering group 
        NULL,                      // no tag identifier 
        NULL,                      // no dependencies 
        NULL,                      // LocalSystem account 
        NULL);                     // no password 

    if (schService == NULL)
    {
        if (GetLastError() == ERROR_SERVICE_EXISTS)
        {
            printf("service already exists (%lu)\n", GetLastError());
        }
        if (GetLastError() != ERROR_SERVICE_EXISTS)
        {
            printf("create service failed: %lu\n", GetLastError());
        }
        CloseServiceHandle(schSCManager);
        return (-1);
    }
    else printf("Service {%s} installed successfully.\n", SVCNAME);

    CloseServiceHandle(schService);
    CloseServiceHandle(schSCManager);
    return (0);
}


int main(int argc, char **argv)
{
    if (argc >= 2)
    {
        char* action = argv[1];

        if (strcmp(action, "install") == 0)
        {
            SvcInstall();
        }
        else if (strcmp(action, "start") == 0)
        {
            printf("start\n");
        }
        else if (strcmp(action, "stop") == 0)
        {
            printf("stop\n");
        }
        else if (strcmp(action, "delete") == 0)
        {
            SvcDelete();
        }
        else
        {
            printf("usage: svc.exe {action} [ install || start || stop || delete ]");
            return (-1);
        }
    }
    else
    {
        printf("this is the service\n");
    }
    return 0;
}
