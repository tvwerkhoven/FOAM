/*
 foamcontrol.cc -- FOAM control connection 
 Copyright (C) 2009--2011 Tim van Werkhoven <werkhoven@strw.leidenuniv.nl>
 
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

#include <getopt.h>

#include "autoconfig.h"

#include "foamcontrol.h"
#include "controlview.h"

#include "foamtypes.h"

using namespace Gtk;
using namespace std;

FoamControl::FoamControl(Log &log, string &conf, string &exec): 
log(log),
conffile(conf), execname(exec),
ok(false), errormsg("Not connected") {
	log.term(format("%s", __PRETTY_FUNCTION__));
}

int FoamControl::connect(const string &h, const string &p) {
	host = h;
	port = p;
	log.term(format("%s(%s, %s)\n", __PRETTY_FUNCTION__, host.c_str(), port.c_str()));

	
	if (protocol.is_connected()) 
		return -1;
	
	// connect a control connection
	protocol.slot_message = sigc::mem_fun(this, &FoamControl::on_message);
	protocol.slot_connected = sigc::mem_fun(this, &FoamControl::on_connected);
	protocol.connect(host, port, "");
	
	return 0;
}

//! @todo This should propagate through the whole GUI, also the device tabs
int FoamControl::disconnect() {
	log.term(format("%s(conn=%d)", __PRETTY_FUNCTION__, protocol.is_connected()));

	if (protocol.is_connected()) {
		protocol.disconnect();
		// Call on_connected() to handle other disconnect tasks for the GUI
		on_connected(protocol.is_connected());
	}
	
	return 0;
}

void FoamControl::send_cmd(const string &cmd) {
	state.lastcmd = cmd;
	protocol.write(cmd);
	log.add(Log::DEBUG, "FOAM: -> " + cmd);
	log.term(format("%s(%s)", __PRETTY_FUNCTION__, cmd.c_str()));

}

void FoamControl::set_mode(aomode_t mode) {
	if (!protocol.is_connected()) return;
	
	log.term(format("%s(%s)", __PRETTY_FUNCTION__, mode2str(mode).c_str()));
	
	switch (mode) {
		case AO_MODE_LISTEN:
			send_cmd("mode listen");
			break;
		case AO_MODE_OPEN:
			send_cmd("mode open");
			break;
		case AO_MODE_CLOSED:
			send_cmd("mode closed");
			break;
		default:
			break;
	}
}

void FoamControl::on_connected(bool conn) {
	log.term(format("%s(conn=%d)", __PRETTY_FUNCTION__, conn));

	if (!conn) {
		protocol.disconnect();
		ok = false;
		errormsg = "Not connected";
		signal_connect();
		return;
	}
	
	ok = true;
	
	// Get basic system information
	send_cmd("get mode");
	send_cmd("get calibmodes");
	send_cmd("get devices");

	signal_connect();
	return;
}

void FoamControl::on_message(string line) {
	log.term(format("%s(%s)", __PRETTY_FUNCTION__, line.c_str()));

	state.lastreply = line;
	
	// The first word is the prefix. This should be the status, or it can be a device name in which case we ignore it.
	string stat = popword(line);
	
	// FOAM may receive broadcast messages from Devices, which are prefixed by 
	// the device name like: '<devname> <status> <command> [parameters]' 
	// e.g. 'simcam ok is_calib 0'. We ignore device commands here.
	if (get_device(stat) != NULL) {
		log.add(Log::DEBUG, "FOAM: <- " + state.lastreply);
		signal_message();
		return;
	}
	// If the prefix is not a device, check if the status is 'ok'
	else if (stat != "ok") {
		ok = false;
		log.add(Log::ERROR, "FOAM: <- " + state.lastreply);
		signal_message();
		return;
	}
	
	log.add(Log::OK, "FOAM: <- " + state.lastreply);

	string what = popword(line);
	ok = true;
	
	if (what == "frames") {
		state.numframes = popint32(line);
	} else if (what == "mode")
		state.mode = str2mode(popword(line));
	else if (what == "calibmodes") {
		int tmp = popint32(line);
		state.calmodes.clear();
		for (int i=0; i<tmp; i++)
			state.calmodes.push_back(popword(line));
	}
	else if (what == "devices") {
		int tmp = popint32(line);
		for (int i=0; i<tmp; i++) {
			string name = popword(line);
			string type = popword(line);
			add_device(name, type);
		}
	}
	else if (what == "cmd") {
		//! \todo implement "cmd" confirmation hook
	}
	else if (what == "calib") {
		//! \todo implement post-calibration hook
	}
	else if (what == "mode") {
		state.mode = str2mode(popword(line));
	} else {
		errormsg = "Unexpected response '" + what + "'";
	}

	signal_message();
}

// Device management

bool FoamControl::add_device(const string name, const string type) {
	log.term(format("%s(%s,%s)", __PRETTY_FUNCTION__, name.c_str(), type.c_str()));
	// Check if already exists
	if (get_device(name) != NULL) {
		log.term(format("%s Exists!", __PRETTY_FUNCTION__));
		log.add(Log::ERROR, "Device " + name + " already exists, cannot add!");
		return false;
	}

	if (type.substr(0,3) != "dev") {
		log.term(format("%s Type wrong!", __PRETTY_FUNCTION__));
		log.add(Log::ERROR, "Device type wrong, should start with 'dev' (was: " + type + ")");
		return false;
	}
	
	//! @todo check that this works
	pthread::mutexholder h(&(gui_mutex));

	// Does not exist, add and init
	log.term(format("%s @ index %d", __PRETTY_FUNCTION__, state.numdev));
	device_t *newdev = &(state.devices[state.numdev]);
	newdev->name = name;
	newdev->type = type;
	
		
	log.term(format("%s Ok", __PRETTY_FUNCTION__));
	state.numdev++;
	signal_device();
	return true;
}

bool FoamControl::rem_device(const string name) {
	log.term(format("%s (%s)", __PRETTY_FUNCTION__, name.c_str()));
	
	device_t *tmpdev = get_device(name);
	if (tmpdev == NULL) {
		log.term(format("%s Does not exist!", __PRETTY_FUNCTION__));
		return false;
	}
	
	//! @todo check that this works
	pthread::mutexholder h(&(gui_mutex));

	
	log.term(format("%s Ok", __PRETTY_FUNCTION__));
	state.numdev--;
	signal_device();
	return true;
}

FoamControl::device_t *FoamControl::get_device(const string name) {
	log.term(format("%s", __PRETTY_FUNCTION__));

	for (int i=0; i<state.numdev; i++) {
		if (state.devices[i].name == name) 
			return &(state.devices[i]);
	}

	return NULL;
}

FoamControl::device_t *FoamControl::get_device(const DevicePage *page) {
	log.term(format("%s", __PRETTY_FUNCTION__));
	
	for (int i=0; i<state.numdev; i++) {
		if (state.devices[i].page == page) 
			return &(state.devices[i]);
	}
	
	return NULL;	
}
