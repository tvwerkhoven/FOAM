/*
 shwfsctrl.h -- Shack-Hartmann wavefront sensor network control class
 Copyright (C) 2010--2011 Tim van Werkhoven <t.i.m.vanwerkhoven@xs4all.nl>
 
 This file is part of FOAM.
 
 FOAM is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 2 of the License, or
 (at your option) any later version.
 
 FOAM is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with FOAM.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <iostream>
#include <arpa/inet.h>
#include <string>
#include <vector>

#include "format.h"
#include "protocol.h"
#include "log.h"
#include "types.h"

#include "wfsctrl.h"
#include "shwfsctrl.h"

using namespace std;

ShwfsCtrl::ShwfsCtrl(Log &log, const string h, const string p, const string n):
	WfsCtrl(log, h, p, n)
{
	fprintf(stderr, "%x:ShwfsCtrl::ShwfsCtrl()\n", (int) pthread_self());
	
	mlacfg.reserve(128);
}

ShwfsCtrl::~ShwfsCtrl() {
	fprintf(stderr, "%x:ShwfsCtrl::~ShwfsCtrl()\n", (int) pthread_self());
}

void ShwfsCtrl::connect() {
	WfsCtrl::connect();
}

void ShwfsCtrl::on_connected(bool conn) {
	WfsCtrl::on_connected(conn);
	fprintf(stderr, "%x:ShwfsCtrl::on_connected(conn=%d)\n", (int) pthread_self(), conn);
	
	if (conn) {
		cmd_get_mla();
	}
}

void ShwfsCtrl::on_message(string line) {
	double x0, y0, x1, y1;
	int n;
	
	// Save original line in case this function does not know what to do
	string orig = line;
	bool parsed = true;
	
	// Discard first 'ok' or 'err' (DeviceCtrl::on_message_common() already parsed this)
	string stat = popword(line);
	
	// Get command
	string what = popword(line);

	if (what == "mla") {								// ok mla <N> [idx x0 y0 x1 y1 [idx x0 y0 x1 y1 [...]]]
		n = popint(line);
		
		if (n <= 0) {
			ok = false;
			errormsg = "Unexpected response for 'mla'";
			signal_message();
			return;
		}
		
		//! @todo this probably needs a mutex as well (Should we use a *Ctrl mutex for all DeviceCtrl classes?)
		mlacfg.clear();
		for (int i=0; i<n; i++) {
			popint(line);										// discard idx
			x0 = popdouble(line); y0 = popdouble(line);
			x1 = popdouble(line); y1 = popdouble(line);
			mlacfg.push_back(fvector_t(x0, y0, x1, y1));
		}
		
		signal_message();
		return;
	} else if (what == "shifts") {			// ok shifts [<N> [idx Sx0 Sy0 Sx1 Sy1 [idx Sx0 Sy0 Sx1 Sy1 [...]]]
		n = popint(line);
		
		shifts_v.clear();
		for (int i=0; i<n; i++) {
			popint(line);										// discard idx
			x0 = popdouble(line); y0 = popdouble(line);
			x1 = popdouble(line); y1 = popdouble(line);
			shifts_v.push_back(fvector_t(x0, y0, x1, y1));
		}
		signal_sh_shifts();
	} else
		parsed = false;

	if (!parsed)
		WfsCtrl::on_message(orig);
	else
		signal_message();
}

