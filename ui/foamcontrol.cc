/*
 foamcontrol.cc -- FOAM control connection 
 Copyright (C) 2009--2010 Tim van Werkhoven <t.i.m.vanwerkhoven@xs4all.nl>
 
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
/*!
 @file foamcontrol.cc
 @author Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)
 
 @brief This is the FOAM control connection class
 */

#include "foamcontrol.h"
#include "controlview.h"

using namespace Gtk;

FoamControl::FoamControl() {
	printf("%x:FoamControl::FoamControl()\n", (int) pthread_self());
	
	ok = false;
	errormsg = "Not connected";
	
	// Init variables
	state.mode = AO_MODE_UNDEF;
	state.numdev = 0;
	state.devices[0].name = "undef";
	state.devices[0].type = "undef";
	state.numframes = 0;
	state.numcal = 0;
	state.calmodes[0] = "undef";
	state.lastreply = "undef";
	state.lastcmd = "undef";
}

int FoamControl::connect(const string &h, const string &p) {
	host = h;
	port = p;
	printf("%x:FoamControl::connect(%s, %s)\n", (int) pthread_self(), host.c_str(), port.c_str());
	
	if (protocol.is_connected()) 
		return -1;
	
	// connect a control connection
	protocol.slot_message = sigc::mem_fun(this, &FoamControl::on_message);
	protocol.slot_connected = sigc::mem_fun(this, &FoamControl::on_connected);
	protocol.connect(host, port, "");
	
	return 0;
}

void FoamControl::set_mode(aomode_t mode) {
	if (!protocol.is_connected()) return;
	
	printf("%x:FoamControl::set_mode(%s)\n", (int) pthread_self(), mode2str(mode).c_str());
	
	switch (mode) {
		case AO_MODE_LISTEN:
			protocol.write("mode listen");
			break;
		case AO_MODE_OPEN:
			protocol.write("mode open");
			break;
		case AO_MODE_CLOSED:
			protocol.write("mode closed");
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
	protocol.write("get mode");
	protocol.write("get calib");
	protocol.write("get devices");

	signal_connect();
	return;
}

void FoamControl::on_message(string line) {
	printf("%x:FoamControl::on_message(string=%s)\n", (int) pthread_self(), line.c_str());

	if (popword(line) != "ok") {
		ok = false;
		state.lastreply = line;
		printf("%x:FoamControl::on_message(): err\n", (int) pthread_self());
		signal_message();
		return;
	}
	
	ok = true;
	state.lastreply = line;
	string what = popword(line);
	
	if (what == "var") {
		string var = popword(line);
		if (var == "frames")
			state.numframes = popint32(line);
		else if (var == "mode")
			state.mode = str2mode(popword(line));
		else if (var == "calib") {
			state.numcal = popint32(line);
			for (int i=0; i<state.numcal; i++)
				state.calmodes[i] = popword(line);
		}
		else if (var == "devices") {
			state.numdev = popint32(line);
			for (int i=0; i<state.numdev; i++) {
				state.devices[i].name = popword(line);
				state.devices[i].type = popword(line);
			}
			// Signal device update to main GUI thread
			signal_device();
		}
	}
	else if (what == "cmd") {
		// TODO command confirmation hook
	}
	else if (what == "calib") {
		// TODO post-calibration hook
	}
	else if (what == "mode") {
		state.mode = str2mode(popword(line));
	} else {
		ok = false;
		errormsg = "Unexpected response '" + what + "'";
	}
	
	printf("%x:FoamControl::on_message(): ok\n", (int) pthread_self());
	signal_message();
}

