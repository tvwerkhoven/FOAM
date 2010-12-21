/*
 devicectrl.cc -- generic device controller
 Copyright (C) 2010 Tim van Werkhoven <t.i.m.vanwerkhoven@xs4all.nl>
 
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

#include <stdio.h>
#include <string>
#include <pthread.h>

#include "protocol.h"
#include "devicectrl.h"

using namespace std;

DeviceCtrl::DeviceCtrl(Log &log, const string h, const string p, const string n):
	log(log), host(h), port(p), devname(n)
{
	printf("%x:DeviceCtrl::DeviceCtrl(name=%s)\n", (int) pthread_self(), n.c_str());	
	
	// Open control connection, register basic callbacks
	protocol.slot_message = sigc::mem_fun(this, &DeviceCtrl::on_message);
	protocol.slot_connected = sigc::mem_fun(this, &DeviceCtrl::on_connected);
	printf("%x:DeviceCtrl::DeviceCtrl(): connecting to %s:%s@%s\n", (int) pthread_self(), host.c_str(), port.c_str(), devname.c_str());
	protocol.connect(host, port, devname);
}

DeviceCtrl::~DeviceCtrl() {
	fprintf(stderr, "DeviceCtrl::~DeviceCtrl()\n");
}

void DeviceCtrl::send_cmd(const string &cmd) {
	lastcmd = cmd;
	protocol.write(cmd);
	log.add(Log::DEBUG, devname + ": -> " + cmd);
	printf("%x:DeviceCtrl::sent cmd: %s\n", (int) pthread_self(), cmd.c_str());
}

void DeviceCtrl::on_message(string line) {
	printf("%x:DeviceCtrl::on_message(line=%s)\n", (int) pthread_self(), line.c_str());	
	string stat = popword(line);

	if (stat != "ok") {
		ok = false;
		errormsg = line;
		log.add(Log::ERROR, devname + ": <- " + stat + " " + line);
	}
	else {
		ok = true;
		log.add(Log::OK, devname + ": <- " + stat + " " + line);
	}
	
	// Parse list of commands device can accept here
	string what = popword(line);
	if (what == "commands") {
		// The rest should be semicolon-delimited commands: "<cmd> [opts]; <cmd2> [opts];" etc.
		int ncmds = popint32(line);
		
		for (int i=0; i<ncmds; i++) {
			string cmd = popword(line, ";");
			if (cmd == "")
				break;
			devcmds.push_back(cmd);
		}
		// Sort alphabetically
		devcmds.sort();
	}
	
	signal_message();
}

void DeviceCtrl::on_connected(bool conn) {
	printf("%x:DeviceCtrl::on_connected(status=%d)\n", (int) pthread_self(), conn);	
	if (conn)
		send_cmd("get commands");
	else {
		ok = false;
		errormsg = "Not connected";
		devcmds.clear();
		//! @todo should we explictly disconnect the socket here too? Otherwise protocol will reconnect immediately.
	}		
	
	signal_connect();
}
