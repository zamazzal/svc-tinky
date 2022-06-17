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
        if (strcmp(pEntry.szExeFile, filename) == 0)
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


int SvcStop()
{
    SERVICE_STATUS_PROCESS ssp;
    DWORD dwStartTime = GetTickCount();
    DWORD dwBytesNeeded;
    DWORD dwTimeout = 30000; // 30-second time-out
    DWORD dwWaitTime;

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
        schSCManager,         // SCM database 
        SVCNAME,            // name of service 
        SERVICE_STOP |
        SERVICE_QUERY_STATUS |
        SERVICE_ENUMERATE_DEPENDENTS);

    if (schService == NULL)
    {
        if (GetLastError() != ERROR_SERVICE_EXISTS)
            printf("service not exists: %lu\n", GetLastError());
        else
            printf("open service failed: %lu\n", GetLastError());
        CloseServiceHandle(schSCManager);
        return -1;
    }

    // Make sure the service is not already stopped.

    if (!QueryServiceStatusEx(
        schService,
        SC_STATUS_PROCESS_INFO,
        (LPBYTE)&ssp,
        sizeof(SERVICE_STATUS_PROCESS),
        &dwBytesNeeded))
    {
        printf("QueryServiceStatusEx failed (%lu)\n", GetLastError());
        goto stop_cleanup;
    }

    if (ssp.dwCurrentState == SERVICE_STOPPED)
    {
        printf("Service is already stopped.\n");
        goto stop_cleanup;
    }

    // If a stop is pending, wait for it.

    while (ssp.dwCurrentState == SERVICE_STOP_PENDING)
    {
        printf("Service stop pending...\n");

        // Do not wait longer than the wait hint. A good interval is 
        // one-tenth of the wait hint but not less than 1 second  
        // and not more than 10 seconds. 

        dwWaitTime = ssp.dwWaitHint / 10;

        if (dwWaitTime < 1000)
            dwWaitTime = 1000;
        else if (dwWaitTime > 10000)
            dwWaitTime = 10000;

        Sleep(dwWaitTime);

        if (!QueryServiceStatusEx(
            schService,
            SC_STATUS_PROCESS_INFO,
            (LPBYTE)&ssp,
            sizeof(SERVICE_STATUS_PROCESS),
            &dwBytesNeeded))
        {
            printf("QueryServiceStatusEx failed (%lu)\n", GetLastError());
            goto stop_cleanup;
        }

        if (ssp.dwCurrentState == SERVICE_STOPPED)
        {
            printf("Service stopped successfully.\n");
            goto stop_cleanup;
        }

        if (GetTickCount() - dwStartTime > dwTimeout)
        {
            printf("Service stop timed out.\n");
            goto stop_cleanup;
        }
    }

    // If the service is running, keyloger must be stopped first.

    killProcessByName("winkey.exe");

    // Send a stop code to the service.

    if (!ControlService(
        schService,
        SERVICE_CONTROL_STOP,
        (LPSERVICE_STATUS)&ssp))
    {
        printf("ControlService failed (%lu)\n", GetLastError());
        goto stop_cleanup;
    }

    // Wait for the service to stop.

    while (ssp.dwCurrentState != SERVICE_STOPPED)
    {
        Sleep(ssp.dwWaitHint);
        if (!QueryServiceStatusEx(
            schService,
            SC_STATUS_PROCESS_INFO,
            (LPBYTE)&ssp,
            sizeof(SERVICE_STATUS_PROCESS),
            &dwBytesNeeded))
        {
            printf("QueryServiceStatusEx failed (%lu)\n", GetLastError());
            goto stop_cleanup;
        }

        if (ssp.dwCurrentState == SERVICE_STOPPED)
            break;

        if (GetTickCount() - dwStartTime > dwTimeout)
        {
            printf("Wait timed out\n");
            goto stop_cleanup;
        }
    }
    printf("Service stopped successfully\n");

stop_cleanup:
    CloseServiceHandle(schService);
    CloseServiceHandle(schSCManager);
    return(0);
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

int SvcStart()
{
    SERVICE_STATUS_PROCESS ssStatus;
    DWORD dwOldCheckPoint;
    DWORD dwStartTickCount;
    DWORD dwWaitTime;
    DWORD dwBytesNeeded;

    SC_HANDLE schSCManager;
    SC_HANDLE schService;

    schSCManager = OpenSCManager(
        NULL,                    // local computer
        NULL,                    // ServicesActive database 
        SC_MANAGER_ALL_ACCESS);  // full access rights 

    if (NULL == schSCManager)
    {
        printf("OpenSCManager failed (%lu)\n", GetLastError());
        return -2;
    }

    schService = OpenService(schSCManager, SVCNAME, GENERIC_ALL);
    if (!schService)
    {
        if (GetLastError() != ERROR_SERVICE_EXISTS)
            printf("service not exists: %lu\n", GetLastError());
        else
            printf("open service failed: %lu\n", GetLastError());
        CloseServiceHandle(schSCManager);
        return -1;
    }

    // Check the status in case the service is not stopped. 

    if (!QueryServiceStatusEx(
        schService,                     // handle to service 
        SC_STATUS_PROCESS_INFO,         // information level
        (LPBYTE)&ssStatus,             // address of structure
        sizeof(SERVICE_STATUS_PROCESS), // size of structure
        &dwBytesNeeded))              // size needed if buffer is too small
    {
        printf("QueryServiceStatusEx failed (%lu)\n", GetLastError());
        CloseServiceHandle(schService);
        CloseServiceHandle(schSCManager);
        return (-1);
    }

    // Check if the service is already running. It would be possible 
    // to stop the service here, but for simplicity this example just returns. 

    if (ssStatus.dwCurrentState != SERVICE_STOPPED && ssStatus.dwCurrentState != SERVICE_STOP_PENDING)
    {
        printf("Cannot start the service because it is already running\n");
        CloseServiceHandle(schService);
        CloseServiceHandle(schSCManager);
        return (-1);
    }

    // Save the tick count and initial checkpoint.

    dwStartTickCount = GetTickCount();
    dwOldCheckPoint = ssStatus.dwCheckPoint;

    // Wait for the service to stop before attempting to start it.

    while (ssStatus.dwCurrentState == SERVICE_STOP_PENDING)
    {
        // Do not wait longer than the wait hint. A good interval is 
        // one-tenth of the wait hint but not less than 1 second  
        // and not more than 10 seconds. 

        dwWaitTime = ssStatus.dwWaitHint / 10;

        if (dwWaitTime < 1000)
            dwWaitTime = 1000;
        else if (dwWaitTime > 10000)
            dwWaitTime = 10000;

        Sleep(dwWaitTime);

        // Check the status until the service is no longer stop pending. 

        if (!QueryServiceStatusEx(
            schService,                     // handle to service 
            SC_STATUS_PROCESS_INFO,         // information level
            (LPBYTE)&ssStatus,             // address of structure
            sizeof(SERVICE_STATUS_PROCESS), // size of structure
            &dwBytesNeeded))              // size needed if buffer is too small
        {
            printf("QueryServiceStatusEx failed (%lu)\n", GetLastError());
            CloseServiceHandle(schService);
            CloseServiceHandle(schSCManager);
            return (-1);
        }

        if (ssStatus.dwCheckPoint > dwOldCheckPoint)
        {
            // Continue to wait and check.

            dwStartTickCount = GetTickCount();
            dwOldCheckPoint = ssStatus.dwCheckPoint;
        }
    }

    if (!StartService(
        schService,  // handle to service 
        0,           // number of arguments 
        NULL))      // no arguments 
    {
        printf("StartService failed (%lu)\n", GetLastError());
        CloseServiceHandle(schService);
        CloseServiceHandle(schSCManager);
        return (-1);
    }
    else printf("Service start pending...\n");


    // Check the status until the service is no longer start pending. 

    if (!QueryServiceStatusEx(
        schService,                     // handle to service 
        SC_STATUS_PROCESS_INFO,         // info level
        (LPBYTE)&ssStatus,             // address of structure
        sizeof(SERVICE_STATUS_PROCESS), // size of structure
        &dwBytesNeeded))              // if buffer too small
    {
        printf("QueryServiceStatusEx failed (%lu)\n", GetLastError());
        CloseServiceHandle(schService);
        CloseServiceHandle(schSCManager);
        return (-1);
    }

    // Save the tick count and initial checkpoint.

    dwStartTickCount = GetTickCount();
    dwOldCheckPoint = ssStatus.dwCheckPoint;

    while (ssStatus.dwCurrentState == SERVICE_START_PENDING)
    {
        // Do not wait longer than the wait hint. A good interval is 
        // one-tenth the wait hint, but no less than 1 second and no 
        // more than 10 seconds. 

        dwWaitTime = ssStatus.dwWaitHint / 10;

        if (dwWaitTime < 1000)
            dwWaitTime = 1000;
        else if (dwWaitTime > 10000)
            dwWaitTime = 10000;

        Sleep(dwWaitTime);

        // Check the status again. 

        if (!QueryServiceStatusEx(
            schService,             // handle to service 
            SC_STATUS_PROCESS_INFO, // info level
            (LPBYTE)&ssStatus,             // address of structure
            sizeof(SERVICE_STATUS_PROCESS), // size of structure
            &dwBytesNeeded))              // if buffer too small
        {
            printf("QueryServiceStatusEx failed (%lu)\n", GetLastError());
            break;
        }

        if (ssStatus.dwCheckPoint > dwOldCheckPoint)
        {
            // Continue to wait and check.

            dwStartTickCount = GetTickCount();
            dwOldCheckPoint = ssStatus.dwCheckPoint;
        }
        else
        {
            if (GetTickCount() - dwStartTickCount > ssStatus.dwWaitHint)
            {
                // No progress made within the wait hint.
                break;
            }
        }
    }

    // Determine whether the service is running.

    if (ssStatus.dwCurrentState == SERVICE_RUNNING)
    {
        printf("Service started successfully.\n");
    }
    else
    {
        printf("Service not started. \n");
        printf("  Current State: %lu\n", ssStatus.dwCurrentState);
        printf("  Exit Code: %lu\n", ssStatus.dwWin32ExitCode);
        printf("  Check Point: %lu\n", ssStatus.dwCheckPoint);
        printf("  Wait Hint: %lu\n", ssStatus.dwWaitHint);
    }
    CloseServiceHandle(schSCManager);
    CloseServiceHandle(schService);
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
            SvcStart();
        }
        else if (strcmp(action, "stop") == 0)
        {
            SvcStop();
        }
        else if (strcmp(action, "delete") == 0)
        {
            SvcStop();
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
        printf("usage: svc.exe {action} [ install || start || stop || delete ]");
    }
    return 0;
}
