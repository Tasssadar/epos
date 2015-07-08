/*
 *	epos/src/install.cpp
 *	(c) 2001 geo@cuni.cz
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
 *	This is the Epos installer for Windows NT (2000 etc.).
 *	It adds a pointer to the configuration files to the registry
 *	and installs Epos as a service.  With "u" specified as the
 *	command line, it will uninstall the Epos service, but will
 *	leave any registry setting intact.
 */
 
#include <windows.h>
#include <winsvc.h>
#include "service.h"
#include <stdio.h>		//sprintf & fopen
#include <iostream.h>
#include "Shlwapi.h"

SC_HANDLE	scm;
SC_HANDLE	sch;

#define SERVICE_BINARY "epos.exe"
#define MAXPATH 1024

void report(char *caption, char *message)
{
	char msg[MAXPATH];
	LPVOID lpMsgBuf;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR) &lpMsgBuf, 0, NULL);
	sprintf(msg, message, lpMsgBuf);
	MessageBox( NULL, msg, caption, MB_OK | MB_ICONINFORMATION );
	LocalFree( lpMsgBuf );
}

void report(char *message)
{
	report("Error", message);
}

bool exists(char *filename)
{
	FILE *f = fopen(filename, "r");
	if (!f) return false;
	fclose(f);
	return true;
}

/*
 *	find_cfg() tries to locate Epos configuration files.
 *	If these are not found at the "standard" location (which
 *	is not a very elegant standard anyway on DOSoid OS's),
 *	we try out "cfg" subdirectories of the current directory
 *	and all its (grand-)parents.  Hopefully that may work.
 */

char *find_cfg(char *path)
{
	static char std[31] = "c:\\usr\\lib\\epos\\version";
	for (; std[0] <= 'z'; std[0]++) if (exists(std)) {
		strrchr(std, '\\')[1] = 0;
		return std;
	}
	while (strrchr(path, '\\')) {
		strcpy(strrchr(path, '\\'), "\\cfg\\version");
		if (exists(path)) {
			strrchr(path, '\\')[1] = 0;
			return path;
		} else {
			strrchr(path, '\\')[0] = 0;
			strrchr(path, '\\')[0] = 0;
		}
	}
	report("Could not find Epos configuration.  Please set\n the HKEY_LOCAL_MACHINE\\SOFTWARE\\Epos\\Setup\\Path\nregistry value manually.");
	return "..\\..\\cfg";
}

void install_service()
{
	char path[MAXPATH];
	char *p;

	if ( GetModuleFileName( NULL, path, MAXPATH ) == 0 ) {
		report("Unable to locate epos.exe - %s");
        return;
	}
	for (p = path + strlen(path) - 2; *p != '\\' && p > path; p--) ;
	strcpy(++p, SERVICE_BINARY);
	if (!exists(path)) {
		report(path, "Unable to find epos.exe - %s");
		return;
	}
	
	sch = CreateService(scm, SERVICE_NAME, SERVICE_DISPLAY_NAME, SERVICE_ALL_ACCESS,
	    SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS,
	    SERVICE_DEMAND_START,
	    SERVICE_ERROR_NORMAL, path, NULL, NULL, NULL, NULL, NULL);

	if (sch) {
		CloseServiceHandle(sch);
	} else {
	        report("CreateService failed - %s\n");
		return;
	}
	HKEY handle;
	unsigned long result;
	if (RegCreateKeyEx(EPOS_CFG_HKEY, EPOS_CFG_SUBKEY, 0,
		"REG_SZ", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
		&handle, &result) == ERROR_SUCCESS) {
		if (result == REG_CREATED_NEW_KEY) {
			char *basedir = find_cfg(path);
			RegSetValueEx(handle, EPOS_CFG_VALUE, 0, REG_EXPAND_SZ, (unsigned char *)basedir, strlen(basedir) + 1);
			report("Epos service installation", "Service " SERVICE_DISPLAY_NAME " installed.  New configuration.\n");
		} else report("Epos service installation", "Service " SERVICE_DISPLAY_NAME " installed. Existing configuration.\n");
	} else report("Failed to create registry key - %s");
}

void remove_service()
{
	sch = OpenService(scm, SERVICE_NAME, SERVICE_ALL_ACCESS);
	if (!sch) {
		report("Failed to open service - %s\n");
	        return;
	}
	if (DeleteService(sch)) {
	        report("Epos service removed", "Deleted service " SERVICE_NAME " \nRegistry entry not removed.");
		return;
	} else {
		report("Failed in attempt to delete service " SERVICE_NAME " - %s\n");
	}
}


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR cmdline, int nCmdShow)
{
	scm = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (!scm) {
		report("Service control manager could not be opened - %s");
		return 0;
	}

	if (stricmp(cmdline, "u") == 0) remove_service();
	else install_service();

	CloseServiceHandle(scm);
	return 0;
}
