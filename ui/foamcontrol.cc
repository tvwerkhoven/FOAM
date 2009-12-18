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

FoamControl::FoamControl(const string &name, const string &host, const string &port): protocol(host, port, ""), dataprotocol(host, port, ""), name(name), host(host), port(port) {
	ok = false;
	errormsg = "Not connected";
	
	// Init variables
	mode = AO_MODE_UNDEF;
	numwfc = 0;
	numwfs = 0;
	
	// Register callback functions
	protocol.slot_message = sigc::mem_fun(this, &Camera::on_message);
	protocol.slot_connected = sigc::mem_fun(this, &Camera::on_connected);
	protocol.connect();
}

FoamControl::FoamControl() {
}

void FoamControl::on_connected(bool connected) {
	if(!connected) {
		ok = false;
		errormsg = "Not connected";
		signal_update();
		return;
	}
	
	protocol.write("get numwfs");
	protocol.write("get numwfc");
}

void FoamControl::on_message(string line) {
	if(popword(line) != "OK") {
		ok = false;
		errormsg = popword(line);
		return;
	}
	
	ok = true;
	string what = popword(line);
	
	if (what == "numwfs")
		numwfs = popint32(line);
	else if (what == "numwfc")
		numwfc = popint32(line);
	else if (what == "mode") {
		string modename = popword(line);
		if (modename == "listen")
			mode = AO_MODE_LISTEN;
		else if (modename == "closed")
			mode = AO_MODE_CLOSED
		else if (modename == "open")
			mode = AO_MODE_OPEN
		else if (modename == "cal")
			mode = AO_MODE_CAL
		else {
			ok = false;
			errormsg = "Unexpected mode '" + modename + "'";
		}
	} else {
		ok = false;
		errormsg = "Unexpected response '" + what + "'";
	}
	
	signal_update();
}

