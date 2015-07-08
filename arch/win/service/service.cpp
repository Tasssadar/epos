/*
 *	epos/src/service.cpp
 *	(c) geo@cuni.cz
 *
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License in doc/COPYING for more details.
 *	
 *	This file contains the Windows NT (2000 etc.) specific code
 *	to run Epos as an NT service and to find its configuration
 *	files based on a registry setting (if it happens to exist).
 */

#include <afxwin.h>

#include "service.h"
#include "globals.h"		/* This includes all Epos-specific stuff */

SERVICE_STATUS ss;
SERVICE_STATUS_HANDLE ssh;
HANDLE hthread;		/* handle of the worker thread */

#define ERROR_UGLY_EXCEPTION	1001
#define ERROR_SOMETHING_WRONG	1002

#ifndef MB_SERVICE_NOTIFICATION
	#define MB_SERVICE_NOTIFICATION	0x00240000L
#endif

#define MBOX(x)  MessageBox(NULL, x, "TTS service Epos", MB_OK | MB_SERVICE_NOTIFICATION)

int stop_service()
{
	server_shutting_down = true;
	
	/* just make sure to interrupt a blocking select() call of the worker thread:    */
	running_at_localhost();
	return 0;
}

bool report_service_status(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwServiceSpecificExitCode, DWORD dwCheckPoint, DWORD dwWaitHint)
{
	ss.dwServiceType = SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS;
	if (dwCurrentState == SERVICE_START_PENDING)
		ss.dwControlsAccepted = 0;
	else
		ss.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_PAUSE_CONTINUE;

	ss.dwCurrentState = dwCurrentState;
	ss.dwWin32ExitCode = dwWin32ExitCode;
	ss.dwServiceSpecificExitCode = dwServiceSpecificExitCode;
	ss.dwCheckPoint = dwCheckPoint;
	ss.dwWaitHint = dwWaitHint;

	return (SetServiceStatus(ssh, &ss) ? true : false);
}

bool report_service_status(DWORD dwCurrentState, DWORD dwCheckPoint, DWORD dwWaitHint)
{
	return report_service_status(dwCurrentState, NO_ERROR, 0, dwCheckPoint, dwWaitHint);
}

HANDLE get_thread_handle()
{
	HANDLE tmp;
	if (!DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(),
				&tmp, THREAD_SUSPEND_RESUME, TRUE, 0)) {
		MBOX("Could not get current thread handle!");
		return NULL;
	}
	return tmp;
}

static VOID service_control(DWORD dwCtrlCode)
{
	DWORD  dwState = SERVICE_RUNNING;
	switch(dwCtrlCode) {
        case SERVICE_CONTROL_PAUSE:
            if (ss.dwCurrentState == SERVICE_RUNNING)
            {
                SuspendThread(hthread);
                dwState = SERVICE_PAUSED;
            }
            break;
        case SERVICE_CONTROL_CONTINUE:
            if (ss.dwCurrentState == SERVICE_PAUSED)
            {
                ResumeThread(hthread);
                dwState = SERVICE_RUNNING;
            }
            break;
        case SERVICE_CONTROL_STOP:
	case SERVICE_CONTROL_SHUTDOWN:
            dwState = SERVICE_STOP_PENDING;
            report_service_status(SERVICE_STOP_PENDING, 1, 2000);
	    stop_service();
            report_service_status(SERVICE_STOP_PENDING, 2, 3000);
	    WaitForSingleObject(hthread, INFINITE);
            report_service_status(SERVICE_STOPPED, 0, 0);
            return;
        case SERVICE_CONTROL_INTERROGATE:
            break;
        default:
            break;
	}
	report_service_status(dwState, 0, 0);
}

void service_main()
{
	ssh = RegisterServiceCtrlHandler(SERVICE_NAME,
                                  (LPHANDLER_FUNCTION)service_control);
	if (!ssh) goto cleanup;
	if (!report_service_status(SERVICE_START_PENDING, 1, 10000))
		goto cleanup;
	try {
		epos_init();
		if (!report_service_status(SERVICE_START_PENDING, 2, 5000))
			goto cleanup;
		lest_already_running();
	} catch (any_exception *e) {
		MBOX(e->msg);
		report_service_status(SERVICE_STOPPED, ERROR_SERVICE_SPECIFIC_ERROR, e->code, 0, 0);
		return;
	} catch (...) {
		MBOX("unknown_exception");
		report_service_status(SERVICE_STOPPED, ERROR_SERVICE_SPECIFIC_ERROR, ERROR_UGLY_EXCEPTION, 0, 0);
		return;
	}
	hthread = get_thread_handle();

	if (!report_service_status(SERVICE_RUNNING, 0, 0))
		goto cleanup;
	server();	/* You return only during the shutdown */

	return;
cleanup:
	if (ssh)
		report_service_status(SERVICE_STOPPED, ERROR_SERVICE_SPECIFIC_ERROR, ERROR_SOMETHING_WRONG, 0, 0);
	return;
}

#ifndef MAX_PATHNAME
	#define MAX_PATHNAME 3100
#endif

int start_nt_service()
{
	unsigned char basedir[MAX_PATHNAME];
	unsigned long int l = MAX_PATHNAME;
	HKEY hkey;

	if (RegOpenKeyEx(EPOS_CFG_HKEY, EPOS_CFG_SUBKEY, 0, KEY_ALL_ACCESS, &hkey) == ERROR_SUCCESS) {
		if (RegQueryValueEx(hkey, EPOS_CFG_VALUE, NULL, NULL, basedir, &l) == ERROR_SUCCESS) {
			set_base_dir((char *)basedir);
		} else {
			MBOX("Too recent registry configuration for Epos?!");
			return 1;
		}
		RegCloseKey(hkey);
	} /* else hope in yer command line and/or compilation parameters */
	
	SERVICE_TABLE_ENTRY disp_table[] =
	{
		{ SERVICE_NAME,(LPSERVICE_MAIN_FUNCTION)service_main },
		{ NULL, NULL }, 
	};
	int success =  StartServiceCtrlDispatcher(disp_table);
	if (!success) return GetLastError();
	else return 0;
}

