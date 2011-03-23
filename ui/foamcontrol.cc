/*
 foamcontrol.cc -- FOAM control connection 
 Copyright (C) 2009--2011 Tim van Werkhoven <t.i.m.vanwerkhoven@xs4all.nl>
 
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

FoamControl::FoamControl(Log &log, int argc, char *argv[]): 
log(log),
execname(""), conffile(""),
ok(false), errormsg("Not connected") {
	printf("%x:FoamControl::FoamControl()\n", (int) pthread_self());
	if (parse_args(argc, argv))
		exit(-1);
}

void FoamControl::show_version() {
	printf("FOAM Control (%s version %s, built %s %s)\n", PACKAGE_NAME, PACKAGE_VERSION, __DATE__, __TIME__);
	printf("Copyright (c) 2007--2011 %s\n", PACKAGE_BUGREPORT);
	printf("\nFOAM Control comes with ABSOLUTELY NO WARRANTY. This is free software,\n"
				 "and you are welcome to redistribute it under certain conditions;\n"
				 "see the file COPYING for details.\n");
}

void FoamControl::show_clihelp(const bool error) {
	if(error)
		fprintf(stderr, "Try '%s --help' for more information.\n", execname.c_str());
	else {
		printf("Usage: %s [option]...\n\n", execname.c_str());
		printf("  -c, --config=FILE    Read configuration from FILE.\n"
					 "  -h, --help           Display this help message.\n"
					 "      --version        Display version information.\n\n");
		printf("Report bugs to %s.\n", PACKAGE_BUGREPORT);
	}
}

int FoamControl::parse_args(int argc, char *argv[]) {
	int r, option_index = 0;
	
	static struct option const long_options[] = {
		{"config", required_argument, NULL, 'c'},
		{"help", no_argument, NULL, 'h'},
		{"version", no_argument, NULL, 1},
		{NULL, 0, NULL, 0}
	};
	
	while((r = getopt_long(argc, argv, "c:h", long_options, &option_index)) != EOF) {
		switch(r) {
			case 0:
				break;
			case 'c':												// Configuration file
				conffile = string(optarg);
				break;
			case 1:													// Version info
				show_version();
				return -1;
			case '?':												// Help
			case 'h':												// Help
			default:
				show_clihelp();
				return -1;
		}
	}
	
	return 0;
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

//!< @bug This does not disable the GUI, after protocol.disconnect, there is no call to on_connected? Does protocol do this at all on disconnect? Solved with on_connected(protocol.is_connected());?
//!< @todo This should propagate through the whole GUI, also the device tabs
int FoamControl::disconnect() {
	printf("%x:FoamControl::disconnect(conn=%d)\n", (int) pthread_self(), protocol.is_connected());
	if (protocol.is_connected()) {
		protocol.disconnect();
		//!< @todo Is this necessary?
		on_connected(protocol.is_connected());
	}
	
	return 0;
}

void FoamControl::send_cmd(const string &cmd) {
	state.lastcmd = cmd;
	protocol.write(cmd);
	log.add(Log::DEBUG, "FOAM: -> " + cmd);
	printf("%x:FoamControl::sent cmd: %s\n", (int) pthread_self(), cmd.c_str());
}

void FoamControl::set_mode(aomode_t mode) {
	if (!protocol.is_connected()) return;
	
	printf("%x:FoamControl::set_mode(%s)\n", (int) pthread_self(), mode2str(mode).c_str());
	
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
	printf("%x:FoamControl::on_connected(bool=%d)\n", (int) pthread_self(), conn);

	if (!conn) {
		ok = false;
		errormsg = "Not connected";
		signal_connect();
		return;
	}
	
	ok = true;
	
	// Get basic system information
	send_cmd("get mode");
	send_cmd("get calib");
	send_cmd("get devices");

	signal_connect();
	return;
}

void FoamControl::on_message(string line) {
	printf("%x:FoamControl::on_message(string=%s)\n", (int) pthread_self(), line.c_str());

	state.lastreply = line;
	
	string stat = popword(line);

	if (stat != "ok") {
		ok = false;
		log.add(Log::ERROR, "FOAM: <- " + state.lastreply);
		signal_message();
		return;
	}
	
	log.add(Log::OK, "FOAM: <- " + state.lastreply);

	
	ok = true;
	string what = popword(line);
	
	if (what == "frames")
		state.numframes = popint32(line);
	else if (what == "mode")
		state.mode = str2mode(popword(line));
	else if (what == "calib") {
		state.numcal = popint32(line);
		for (int i=0; i<state.numcal; i++)
			state.calmodes[i] = popword(line);
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
	printf("%x:FoamControl::add_device(%s, %s)\n", (int) pthread_self(), name.c_str(), type.c_str());
	// Check if already exists
	if (get_device(name) != NULL) {
		printf("%x:FoamControl::add_device() Exists!\n", (int) pthread_self());
		log.add(Log::ERROR, "Device " + name + " already exists, cannot add!");
		return false;
	}

	if (type.substr(0,3) != "dev") {
		printf("%x:FoamControl::add_device() Type wrong!\n", (int) pthread_self());
		log.add(Log::ERROR, "Device type wrong, should start with 'dev' (was: " + type + ")");
		return false;
	}
	
	//! @todo check that this works
	pthread::mutexholder h(&(gui_mutex));

	// Does not exist, add and init
	printf("%x:FoamControl::add_device() @ index %d\n", (int) pthread_self(), state.numdev);
	device_t *newdev = &(state.devices[state.numdev]);
	newdev->name = name;
	newdev->type = type;
	
		
	printf("%x:FoamControl::add_device() Ok\n", (int) pthread_self());
	state.numdev++;
	signal_device();
	return true;
}

bool FoamControl::rem_device(const string name) {
	printf("%x:FoamControl::rem_device(%s)\n", (int) pthread_self(), name.c_str());
	
	device_t *tmpdev = get_device(name);
	if (tmpdev == NULL) {
		printf("%x:FoamControl::rem_device() Does not exist!\n", (int) pthread_self());
		return false;
	}
	
	//! @todo check that this works
	pthread::mutexholder h(&(gui_mutex));

	
	printf("%x:FoamControl::rem_device() Ok\n", (int) pthread_self());
	state.numdev--;
	signal_device();
	return true;
}

FoamControl::device_t *FoamControl::get_device(const string name) {
	printf("%x:FoamControl::get_device(%s)\n", (int) pthread_self(), name.c_str());

	for (int i=0; i<state.numdev; i++) {
		if (state.devices[i].name == name) 
			return &(state.devices[i]);
	}

	return NULL;
}

FoamControl::device_t *FoamControl::get_device(const DevicePage *page) {
	printf("%x:FoamControl::get_device(page)\n", (int) pthread_self());
	
	for (int i=0; i<state.numdev; i++) {
		if (state.devices[i].page == page) 
			return &(state.devices[i]);
	}
	
	return NULL;	
}
