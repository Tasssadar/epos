/*
 *	epos/src/service.h
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
 *	This file declares the common information for instserv.exe
 *	and epos.exe if running as an NT service.  
 *	It is also included by the say utility to start the Epos service.
 */

/* NT service */

#define SERVICE_NAME  "ttscp"
#define SERVICE_DISPLAY_NAME "Text-to-Speech system Epos"

/* Registry subkey to locate configuration files */

#define EPOS_CFG_HKEY	HKEY_LOCAL_MACHINE
#define EPOS_CFG_SUBKEY "SOFTWARE\\Epos\\Setup"
#define EPOS_CFG_VALUE	"Path"


