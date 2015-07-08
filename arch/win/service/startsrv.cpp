/*
 *	(c) 1998-01 Jirka Hanika <geo@cuni.cz>
 *
 *	This single source file src/startsrv.cc, but NOT THE REST OF THIS PACKAGE,
 *	is considered to be in Public Domain. Parts of this single source file may be
 *	freely incorporated into any commercial or other software.
 *
 *	Most files in this package are strictly covered by the General Public
 *	License version 2, to be found in doc/COPYING. Should GPL and the paragraph
 *	above come into any sort of legal conflict, GPL takes precedence.
 *
 *	This file supplements "say", a simple TTSCP client, with
 *	the ability to start the TTSCP service on Windows NT.
 *	Epos must already be installed as the TTSCP service.
 *	You are expected to happily ignore this file.
 */

#include <windows.h>
#include <winsvc.h>
#include "service.h"

/* start_service ignores any errors, but returns false if they occur */

bool start_service()
{
	SERVICE_STATUS r;

	SC_HANDLE m = OpenSCManager(NULL, SERVICES_ACTIVE_DATABASE, SC_MANAGER_CONNECT | SC_MANAGER_ENUMERATE_SERVICE | SC_MANAGER_QUERY_LOCK_STATUS);
	if (!m) return false;
	SC_HANDLE s = OpenService(m, SERVICE_NAME, SERVICE_START | SERVICE_INTERROGATE);
	if (!s) return false;
	if (QueryServiceStatus(s, &r) && r.dwCurrentState == SERVICE_RUNNING)
		return true;
	if (!StartService(s, 0, NULL)) return false;
	while (QueryServiceStatus(s, &r) && r.dwCurrentState != SERVICE_RUNNING)
		SleepEx(1000, true);
	SleepEx(2000, true);	/* about 400 ms is necessary */
	if (!CloseServiceHandle(s)) return false;

	return true;
}


bool stop_service()
{
	SERVICE_STATUS r;

	SC_HANDLE m = OpenSCManager(NULL, SERVICES_ACTIVE_DATABASE, SC_MANAGER_CONNECT | SC_MANAGER_ENUMERATE_SERVICE | SC_MANAGER_QUERY_LOCK_STATUS);
	if (!m) return false;
	SC_HANDLE s = OpenService(m, SERVICE_NAME, SERVICE_STOP | SERVICE_INTERROGATE);
	if (!s) return false;
	if (!ControlService(s, SERVICE_CONTROL_STOP, &r)) return false;
	if (!CloseServiceHandle(s)) return false;

	return true;
}
