/*
 shwfsctrl.h -- Shack-Hartmann wavefront sensor network control class
 Copyright (C) 2010--2011 Tim van Werkhoven <werkhoven@strw.leidenuniv.nl>
 
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
	log.term(format("%s", __PRETTY_FUNCTION__));
	
	mlacfg.reserve(128);
}

ShwfsCtrl::~ShwfsCtrl() {
	log.term(format("%s", __PRETTY_FUNCTION__));
}

void ShwfsCtrl::connect() {
	WfsCtrl::connect();
}

void ShwfsCtrl::on_connected(bool conn) {
	WfsCtrl::on_connected(conn);
	log.term(format("%s (%d)", __PRETTY_FUNCTION__, conn));

	if (conn)
		cmd_get_mla();
}

void ShwfsCtrl::on_message(string line) {
	double x0, y0, x1, y1;
	double refx, refy, shiftx, shifty, subapx, subapy;
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
	} else if (what == "shifts") {			// ok shifts see See Shwfs::get_shifts_str() for format.
		n = popint(line);
		
		shifts_v.clear();
		refshift_v.clear();
		for (int i=0; i<n; i++) {
			popint(line);										// discard idx
			subapx = popdouble(line); subapy = popdouble(line);
			refx = popdouble(line); refy = popdouble(line);
			shiftx = popdouble(line); shifty = popdouble(line);
			refshift_v.push_back(fvector_t(subapx, subapy, subapx+refx, subapy+refy));
			shifts_v.push_back(fvector_t(subapx+refx, subapy+refy, subapx+refx+shiftx, subapy+refy+shifty));
		}
		signal_sh_shifts();
	} else
		parsed = false;

	if (!parsed)
		WfsCtrl::on_message(orig);
	else
		signal_message();
}

