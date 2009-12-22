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

FoamControl::FoamControl(ControlPage &parent): parent(parent) {
	printf("%X:FoamControl::FoamControl()\n", pthread_self());

	init();
}

FoamControl::~FoamControl() {
	if (protocol) { delete protocol; protocol=NULL;}
}

void FoamControl::init() {
	printf("%X:FoamControl::init()\n", pthread_self());
	protocol = NULL;
	
	ok = false;
	connected = false;	
	errormsg = "Not connected";
	
	// Init variables
	state.mode = AO_MODE_UNDEF;
	state.numwfc = 0;
	state.numwfs = 0;
	
	host = "none";
	port = "-1";
	
	// register callbacks
	signal_conn_update.connect(sigc::mem_fun(*this, &FoamControl::on_connect_update));
	signal_msg_update.connect(sigc::mem_fun(*this, &FoamControl::on_message_update));
}

int FoamControl::connect(const string &hostvar, const string &portvar) {
	printf("%X:FoamControl::connect(%s, %s)\n", pthread_self(), host.c_str(), port.c_str());

	if (protocol) { delete protocol; protocol=NULL;}
	
	// Copy to class
	host = hostvar;
	port = portvar;
	connected = false;

	// connect a control connection
	protocol = new Protocol::Client(host, port, "");
	
	protocol->slot_message = sigc::mem_fun(this, &FoamControl::on_message);
	protocol->slot_connected = sigc::mem_fun(this, &FoamControl::on_connected);
	protocol->connect();
	
	return 0;
}

void FoamControl::set_mode(aomode_t mode) {
	if (!protocol) return;
	
	switch (mode) {
		case AO_MODE_LISTEN:
			printf("FoamControl::set_mode(AO_MODE_LISTEN)\n");
			protocol->write("mode listen");
			break;
		case AO_MODE_OPEN:
			printf("FoamControl::set_mode(AO_MODE_OPEN)\n");
			protocol->write("mode open");
			break;
		case AO_MODE_CLOSED:
			printf("FoamControl::set_mode(AO_MODE_CLOSED)\n");
			protocol->write("mode closed");
			break;
		default:
			printf("FoamControl::set_mode(UNKNOWN)\n");
			break;
	}
}

int FoamControl::disconnect() {
	printf("%X:FoamControl::disconnect()\n", pthread_self());
	if (protocol) { delete protocol; protocol=NULL;}
	
	// Init variables
	connected = false;
	state.mode = AO_MODE_UNDEF;
	state.numwfc = 0;
	state.numwfs = 0;
	
	signal_conn_update();
	return 0;
}

void FoamControl::on_connect_update() {
	printf("%X:FoamControl::on_connect_update()\n", pthread_self());
	if (!connected) {
		// Why is this? Socket keeps on reconnecting after error?
		if (protocol) { delete protocol; protocol=NULL; }
	}
	
	// GUI updating is done in parent (a ControlPage)
	parent.on_connect_update();
}

void FoamControl::on_message_update() {
	printf("%X:FoamControl::on_message_update()\n", pthread_self());
	// GUI updating is done in parent (a ControlPage)
	parent.on_message_update();
}

void FoamControl::on_connected(bool conn) {
	printf("%X:FoamControl::on_connected(bool=%d)\n", pthread_self(), conn);

	if (!conn) {
		ok = false;
		connected = false;
		errormsg = "Not connected";
		protocol->running = false;
		signal_conn_update();
		return;
	}
	
	ok = true;
	connected = true;
	
	protocol->write("get numwfs");
	protocol->write("get numwfc");
	signal_conn_update();
	return;
}

void FoamControl::on_message(string line) {
	printf("%X:FoamControl::on_message(string=%s)\n", pthread_self(), line.c_str());

	if (popword(line) != "OK") {
		ok = false;
		errormsg = popword(line);
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
	}
	else if (what == "CMD") {
		// command confirmation hook
		state.currcmd = popword(line);
	}
	else if (what == "CALIB") {
		// post-calibration hook
	}
	else if (what == "MODE") {
		string modename = popword(line);
		if (modename == "LISTEN")
			state.mode = AO_MODE_LISTEN;
		else if (modename == "CLOSED")
			state.mode = AO_MODE_CLOSED;
		else if (modename == "OPEN")
			state.mode = AO_MODE_OPEN;
		else if (modename == "CALIB")
			state.mode = AO_MODE_CAL;
		else {
			ok = false;
			errormsg = "Unexpected mode '" + modename + "'";
		}
	} else {
		ok = false;
		errormsg = "Unexpected response '" + what + "'";
	}
	
	signal_msg_update();
}

