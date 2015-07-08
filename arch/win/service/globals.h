/*
 *	epos/src/globals.h
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
 *	For clarity, and to keep this port as maintainance-free as possible,
 *	the very few interfaces to Epos proper are re-declared here, so that
 *	service.cpp doesn't include any other common Epos code.
 *
 *	It is the subset of src/common.h required by the NT service code.
 */
 
 
/* What the service code must necessarily know about Epos internals */
void epos_init();
extern volatile bool server_shutting_down;
void server();
bool running_at_localhost();
void lest_already_running();
void set_base_dir(char *);

#include "exc.h"
