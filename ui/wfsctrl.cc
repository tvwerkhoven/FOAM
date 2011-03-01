/*
 wfsctrl.h -- wavefront sensor network control class
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

#include "format.h"
#include "protocol.h"
#include "devicectrl.h"
#include "wfsctrl.h"
#include "log.h"

using namespace std;

WfsCtrl::WfsCtrl(Log &log, const string h, const string p, const string n):
DeviceCtrl(log, h, p, n),
wfscam("")
{
	fprintf(stderr, "%x:WfsCtrl::WfsCtrl()\n", (int) pthread_self());
	
}

WfsCtrl::~WfsCtrl() {
	fprintf(stderr, "%x:WfsCtrl::~WfsCtrl()\n", (int) pthread_self());
}

void WfsCtrl::connect() {
	DeviceCtrl::connect();
}

void WfsCtrl::on_connected(bool conn) {
	DeviceCtrl::on_connected(conn);
	fprintf(stderr, "%x:WfsCtrl::on_connected(conn=%d)\n", (int) pthread_self(), conn);
	
	if (conn) {
		send_cmd("measuretest");
		send_cmd("get modes");
		send_cmd("get basis");
		send_cmd("get camera");
	}
}

void WfsCtrl::on_message(string line) {
	fprintf(stderr, "%x:WfsCtrl::on_message(line=%s)\n", (int) pthread_self(), line.c_str());

	// Save original line in case this function does not know what to do
	string orig = line;
	bool parsed = true;

	// Discard first 'ok' or 'err' (DeviceCtrl::on_message_common() already parsed this)
	string stat = popword(line);
	
	// Get command
	string what = popword(line);

	if (what == "modes") {
		wf.nmodes = popint(line);

		// Check nmodes sane
		if (wf.nmodes <= 0) {
			ok = false;
			errormsg = format("Got %d<=0 modes", wf.nmodes);
			signal_message();
			return;
		}
		
		// Check allocation ok
		if (!wf.wfamp)
			wf.wfamp = gsl_vector_float_alloc(wf.nmodes);
		else if (wf.wfamp->size != wf.nmodes) {
			gsl_vector_float_free(wf.wfamp);
			wf.wfamp = gsl_vector_float_alloc(wf.nmodes);
		}
		
		// Copy wavefront data
		for (int n=0; n<wf.nmodes; n++) {
			double tmp = popdouble(line);
			gsl_vector_float_set(wf.wfamp, n, tmp);
		}
		signal_wavefront();
	} else if (what == "camera") {
		wfscam = popword(line);
		signal_wfscam();
		return;
	} else if (what == "basis") {
			wf.basis = popword(line);
	} else if (what == "measuretest" || what == "measure") {
		// If Wfs did a measurement or measuretest, get the results
		send_cmd("get modes");
	} else
		parsed = false;
	
	if (!parsed)
		DeviceCtrl::on_message(orig);
	else
		signal_message();
}
