// code taken from: https://docs.microsoft.com/en-us/windows/desktop/services/the-complete-service-sample

#include <windows.h>
#include <tchar.h>
#include <strsafe.h>
#include <netlistmgr.h>
#include <atlbase.h>
#include "RegistrationLogic.h"

#pragma comment(lib, "advapi32.lib")

#define SVCNAME TEXT("IntelMPAService")
#define SVCDISPLAYNAME TEXT("Intel(R) SGX Multi-Package Registration Service")
#define SVCDDESCRIPTION TEXT("Intel(R) SGX Multi-Package Attestation Registration Service") 
#define SVCDEPEND TEXT("Tcpip")

SERVICE_STATUS          gSvcStatus;
SERVICE_STATUS_HANDLE   gSvcStatusHandle;
BOOLEAN					gSvcExit = FALSE;
NLM_CONNECTIVITY		gConnectivity = NLM_CONNECTIVITY_DISCONNECTED;

VOID WINAPI SvcCtrlHandler(DWORD);
VOID WINAPI SvcMain(DWORD, LPTSTR *);

VOID ReportSvcStatus(DWORD, DWORD, DWORD);
VOID SvcInit(DWORD, LPTSTR *);
VOID SvcReportEvent(LPCTSTR);
BOOL SvcInstall();
bool SvcUninstall();


//
// Purpose: 
//   Entry point for the process
//
// Parameters:
//   None
// 
// Return value:
//   None
//
int __cdecl _tmain(int argc, TCHAR *argv[])
{

	if (argc > 1 && lstrcmpi(argv[1], TEXT("/Install")) == 0)
	{
		if (SvcInstall() == FALSE)
			return 1;
		return 0;
	}
	
	if (argc > 1 && lstrcmpi(argv[1], TEXT("/Uninstall")) == 0)
	{
		if (SvcUninstall() == false)
			return 1;
		return 0;
	}
	
	SERVICE_TABLE_ENTRY DispatchTable[] =
	{
		{ (LPWSTR)SVCNAME, (LPSERVICE_MAIN_FUNCTION)SvcMain },
		{ NULL, NULL }
	};

	// for attaching debugger to the service
	//while (true) 
	//	Sleep(1000);

	// This call returns when the service has stopped. 
	// The process should simply terminate when the call returns.
	if (!StartServiceCtrlDispatcher(DispatchTable))
	{
		SvcReportEvent(TEXT("StartServiceCtrlDispatcher"));
	}

	return 0;
}


//
// Purpose: 
//   Entry point for the service
//
// Parameters:
//   dwArgc - Number of arguments in the lpszArgv array
//   lpszArgv - Array of strings. The first string is the name of
//     the service and subsequent strings are passed by the process
//     that called the StartService function to start the service.
// 
// Return value:
//   None.
//
VOID WINAPI SvcMain(DWORD dwArgc, LPTSTR *lpszArgv)
{
	// Register the handler function for the service
	gSvcStatusHandle = RegisterServiceCtrlHandler(SVCNAME, SvcCtrlHandler);
	if (!gSvcStatusHandle)
	{
		SvcReportEvent(TEXT("RegisterServiceCtrlHandler"));
		return;
	}

	// These SERVICE_STATUS members remain as set here
	gSvcStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	gSvcStatus.dwServiceSpecificExitCode = 0;

	// Report initial status to the SCM
	ReportSvcStatus(SERVICE_START_PENDING, NO_ERROR, 3000);

	// Wait for Internet connection
	SvcInit(dwArgc, lpszArgv);

	if ((gConnectivity & NLM_CONNECTIVITY_IPV4_INTERNET) == 0) // no Internet connection
	{
		ReportSvcStatus(SERVICE_STOPPED, ERROR_NO_NETWORK, 0);
		return;
	}

	// Report running status
	ReportSvcStatus(SERVICE_RUNNING, NO_ERROR, 0);

	// Perform work
	RegistrationLogic::registerPlatform();
	
	// todo - check maybe there was an error...
	ReportSvcStatus(SERVICE_STOPPED, NO_ERROR, 0);
}

VOID GetConnectivityStatus()
{
	CComPtr<INetworkListManager> pNLM;
	HRESULT hr = CoCreateInstance(CLSID_NetworkListManager, NULL, CLSCTX_ALL, __uuidof(INetworkListManager), (LPVOID*)&pNLM);
	if (FAILED(hr))
	{
		SvcReportEvent(TEXT("CoCreateInstance"));
		return;
	}

	DWORD loop_counter = 60; // assuming that no connectivity for 1 minute means that this machine will not be connected to the Internet

	// Wait until network connectivity is available
	while (((gConnectivity & NLM_CONNECTIVITY_IPV4_INTERNET) == 0) && (gSvcExit == FALSE) && (loop_counter-- > 0))
	{
		hr = pNLM->GetConnectivity(&gConnectivity);
		if (FAILED(hr))
		{
			SvcReportEvent(TEXT("GetConnectivity"));
			break;
		}

		if ((gConnectivity & NLM_CONNECTIVITY_IPV4_INTERNET) == 0)
		{
			ReportSvcStatus(SERVICE_START_PENDING, NO_ERROR, 3000);
			Sleep(1000);
		}
	}
}

//
// Purpose: 
//   The service code
//
// Parameters:
//   dwArgc - Number of arguments in the lpszArgv array
//   lpszArgv - Array of strings. The first string is the name of
//     the service and subsequent strings are passed by the process
//     that called the StartService function to start the service.
// 
// Return value:
//   None
//
VOID SvcInit(DWORD dwArgc, LPTSTR *lpszArgv)
{
	HRESULT hr = CoInitialize(NULL);
	if (FAILED(hr))
	{
		SvcReportEvent(TEXT("CoInitialize"));
		return;
	}
	
	GetConnectivityStatus();
	
	CoUninitialize();
}

//
// Purpose: 
//   Sets the current service status and reports it to the SCM.
//
// Parameters:
//   dwCurrentState - The current state (see SERVICE_STATUS)
//   dwWin32ExitCode - The system error code
//   dwWaitHint - Estimated time for pending operation, 
//     in milliseconds
// 
// Return value:
//   None
//
VOID ReportSvcStatus(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint)
{
	static DWORD dwCheckPoint = 1;

	// Fill in the SERVICE_STATUS structure.
	gSvcStatus.dwCurrentState = dwCurrentState;
	gSvcStatus.dwWin32ExitCode = dwWin32ExitCode;
	gSvcStatus.dwWaitHint = dwWaitHint;

	if (dwCurrentState == SERVICE_START_PENDING)
		gSvcStatus.dwControlsAccepted = 0;
	else gSvcStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;

	if ((dwCurrentState == SERVICE_RUNNING) ||
		(dwCurrentState == SERVICE_STOPPED))
		gSvcStatus.dwCheckPoint = 0;
	else gSvcStatus.dwCheckPoint = dwCheckPoint++;

	// Report the status of the service to the SCM.
	SetServiceStatus(gSvcStatusHandle, &gSvcStatus);
}

//
// Purpose: 
//   Called by SCM whenever a control code is sent to the service
//   using the ControlService function.
//
// Parameters:
//   dwCtrl - control code
// 
// Return value:
//   None
//
VOID WINAPI SvcCtrlHandler(DWORD dwCtrl)
{
	// Handle the requested control code. 
	switch (dwCtrl)
	{
	case SERVICE_CONTROL_STOP:
		gSvcExit = TRUE; // stop the waiting loop for the network connectivity
		ReportSvcStatus(SERVICE_ACCEPT_STOP, NO_ERROR, 1500);
		return;

	case SERVICE_CONTROL_INTERROGATE:
		break;

	default:
		break;
	}

}

//
// Purpose: 
//   Logs messages to the event log
//
// Parameters:
//   szFunction - name of function that failed
// 
// Return value:
//   None
//
// Remarks:
//   The service must have an entry in the Application event log.
//
VOID SvcReportEvent(LPCTSTR szFunction)
{
	HANDLE hEventSource;
	LPCTSTR lpszStrings[2];
	TCHAR Buffer[80];

	hEventSource = RegisterEventSource(NULL, SVCNAME);

	if (NULL != hEventSource)
	{
		StringCchPrintf(Buffer, 80, TEXT("%s failed with %d"), szFunction, GetLastError());

		lpszStrings[0] = SVCNAME;
		lpszStrings[1] = Buffer;

		ReportEvent(hEventSource,  // event log handle
			EVENTLOG_ERROR_TYPE, // event type
			0,                   // event category
			0,		             // event identifier
			NULL,                // no security identifier
			2,                   // size of lpszStrings array
			0,                   // no binary data
			lpszStrings,         // array of strings
			NULL);               // no binary data

		DeregisterEventSource(hEventSource);
	}
}



//
// Purpose: 
//   Installs a service in the SCM database
//
// Parameters:
//   None
// 
// Return value:
//   BOOL
//
BOOL SvcInstall()
{
	SC_HANDLE schSCManager;
	SC_HANDLE schService;
	SERVICE_DESCRIPTION info;
	TCHAR szPath[MAX_PATH];

	if (!GetModuleFileName(NULL, szPath, MAX_PATH))
	{
		printf("Cannot install service (%d)\n", GetLastError());
		return FALSE;
	}

	// Get a handle to the SCM database. 

	schSCManager = OpenSCManager(
		NULL,                    // local computer
		NULL,                    // ServicesActive database 
		SC_MANAGER_ALL_ACCESS);  // full access rights 

	if (NULL == schSCManager)
	{
		printf("OpenSCManager failed (%d)\n", GetLastError());
		return FALSE;
	}

	// Create the service

	schService = CreateService(
		schSCManager,              // SCM database 
		SVCNAME,                   // name of service 
		SVCDISPLAYNAME,            // service name to display 
		SERVICE_ALL_ACCESS,        // desired access 
		SERVICE_WIN32_OWN_PROCESS, // service type 
		SERVICE_AUTO_START,        // start type 
		SERVICE_ERROR_NORMAL,      // error control type 
		szPath,                    // path to service's binary 
		NULL,                      // no load ordering group 
		NULL,                      // no tag identifier 
		SVCDEPEND,                 // no dependencies 
		NULL,                      // LocalSystem account 
		NULL);                     // no password 

	if (schService == NULL)
	{
		printf("CreateService failed (%d)\n", GetLastError());
		CloseServiceHandle(schSCManager);
		return FALSE;
	}
	else printf("Service installed successfully\n");

	info.lpDescription = (wchar_t *)SVCDDESCRIPTION;
	if (ChangeServiceConfig2(schService,
		SERVICE_CONFIG_DESCRIPTION,
		&info) == FALSE)
	{
		printf("ChangeServiceConfig2 failed (%d)\n", GetLastError());
	}

	CloseServiceHandle(schService);
	CloseServiceHandle(schSCManager);

	return TRUE;
}


//
// Purpose: 
//   Deletes a service from the SCM database
//
// Parameters:
//   None
// 
// Return value:
//   bool
//
bool SvcUninstall()
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
		printf("OpenSCManager failed (%d)\n", GetLastError());
		return false;
	}

	// Get a handle to the service.

	schService = OpenService(
		schSCManager,       // SCM database 
		SVCNAME,            // name of service 
		DELETE);            // need delete access 

	if (schService == NULL)
	{
		printf("OpenService failed (%d)\n", GetLastError());
		CloseServiceHandle(schSCManager);
		return false;
	}

	// Delete the service.

	if (!DeleteService(schService))
	{
		printf("DeleteService failed (%d)\n", GetLastError());
	}
	else printf("Service deleted successfully\n");

	CloseServiceHandle(schService);
	CloseServiceHandle(schSCManager);

	return true;
}
