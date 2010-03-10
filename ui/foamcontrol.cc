/* foamcontrol.cc - FOAM control connection
 Copyright (C) 2009 Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)
 
 This file is part of FOAM.
 
 FOAM is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 FOAM is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with FOAM.  If not, see <http://www.gnu.org/licenses/>.
 */
/*!
 @file foamcontrol.cc
 @author Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)
 
 @brief This is the FOAM control connection class
 */

#include "foamcontrol.h"
#include "fgui.h"
#include "controlview.h"

using namespace Gtk;

FoamControl::FoamControl() {
	printf("%x:FoamControl::FoamControl()\n", (int) pthread_self());
	
	ok = false;
	errormsg = "Not connected";
	
	// Init variables
	state.mode = AO_MODE_UNDEF;
	state.numwfc = -1;
	state.numwfs = -1;
	state.numframes = 0;
}

int FoamControl::connect(const string &host, const string &port) {
	printf("%x:FoamControl::connect(%s, %s)\n", (int) pthread_self(), host.c_str(), port.c_str());
	
	if (protocol.is_connected()) 
		return -1;
	
	// connect a control connection
	protocol.slot_message = sigc::mem_fun(this, &FoamControl::on_message);
	protocol.slot_connected = sigc::mem_fun(this, &FoamControl::on_connected);
	protocol.connect(host, port, "SYSTEM");
	
	return 0;
}

void FoamControl::set_mode(aomode_t mode) {
	if (!protocol.is_connected()) return;
	
	printf("%x:FoamControl::set_mode(%s)\n", (int) pthread_self(), mode2str(mode).c_str());
	
	switch (mode) {
		case AO_MODE_LISTEN:
			protocol.write("MODE LISTEN");
			break;
		case AO_MODE_OPEN:
			protocol.write("MODE OPEN");
			break;
		case AO_MODE_CLOSED:
			protocol.write("MODE CLOSED");
			break;
		default:
			break;
	}
}

int FoamControl::disconnect() {
	printf("%x:FoamControl::disconnect()\n", (int) pthread_self());
	if (protocol.is_connected())
		protocol.disconnect();
	
	signal_connect();
	return 0;
}

void FoamControl::on_connected(bool conn) {
	printf("%x:FoamControl::on_connected(bool=%d)\n", (int) pthread_self(), conn);

	if (!conn) {
		ok = false;
		errormsg = "Not connected";
		protocol.disconnect();
		signal_connect();
		return;
	}
	
	ok = true;
	
	// Get basic system information
	protocol.write("GET INFO");
//	protocol.write("GET NUMWFS");
//	protocol.write("GET NUMWFC");
//	protocol.write("GET MODE");
//	protocol.write("GET CALIB");

	signal_connect();
	return;
}

void FoamControl::on_message(string line) {
	printf("%x:FoamControl::on_message(string=%s)\n", (int) pthread_self(), line.c_str());

	if (popword(line) != "OK") {
		ok = false;
		errormsg = line;
		signal_message();
		return;
	}
	
	ok = true;
	string what = popword(line);
	
	if (what == "VAR") {
		string var = popword(line);
		if (var == "NUMWFC")
			state.numwfc = popint32(line);
		else if (var == "NUMWFS")
			state.numwfs = popint32(line);
		else if (var == "FRAMES")
			state.numframes = popint32(line);
		else if (var == "MODE")
			state.mode = str2mode(popword(line));
		else if (var == "CALIB") {
			int n = popint32(line);
			state.calmodes[n] = "__SENTINEL__";
			while (n>0) state.calmodes[--n] = popword(line);
		}
	}
	else if (what == "CMD") {
		// command confirmation hook
		state.currcmd = popword(line);
	}
	else if (what == "CALIB") {
		// post-calibration hook
	}
	else if (what == "MODE") {
		state.mode = str2mode(popword(line));
	} else {
		ok = false;
		errormsg = "Unexpected response '" + what + "'";
	}
	
	signal_message();
}

