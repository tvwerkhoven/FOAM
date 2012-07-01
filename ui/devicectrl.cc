/*
 devicectrl.cc -- generic device controller
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

#include <stdio.h>
#include <string>
#include <pthread.h>

#include "protocol.h"
#include "devicectrl.h"

using namespace std;

DeviceCtrl::DeviceCtrl(Log &log, const string h, const string p, const string n):
	host(h), port(p), devname(n),
	protocol(host, port, devname), log(log),
	ok(false), calib(false), init(false), errormsg("Not connected")
{
	log.term(format("%s(name=%s)", __PRETTY_FUNCTION__, n.c_str()));
	
	// Open control connection, register basic callbacks
	protocol.slot_message = sigc::mem_fun(this, &DeviceCtrl::on_message_common);
	protocol.slot_connected = sigc::mem_fun(this, &DeviceCtrl::on_connected);
}

DeviceCtrl::~DeviceCtrl() {
	log.term(format("%s", __PRETTY_FUNCTION__));
}

void DeviceCtrl::connect() {
	log.term(format("%s (%s:%s, %s)", __PRETTY_FUNCTION__, host.c_str(), port.c_str(), devname.c_str()));
	protocol.connect();
}

void DeviceCtrl::send_cmd(const string &cmd) {
	lastcmd = cmd;
	protocol.write(cmd);
	set<string>::iterator it = cmd_ign_list.find(cmd);
	if (it == cmd_ign_list.end())
		log.add(Log::DEBUG, devname + ": -> " + cmd);
	log.term(format("%s (%s)", __PRETTY_FUNCTION__, cmd.c_str()));
}

void DeviceCtrl::on_message_common(string line) {
	log.term(format("%s (%s)", __PRETTY_FUNCTION__, line.c_str()));
	// Save line for passing to on_message()
	string orig = line;
	string stat = popword(line);
	string cmd = popword(line);
	
	if (stat != "ok") {
		ok = false;
		errormsg = line;
		log.add(Log::ERROR, devname + ": <- " + stat + " " + cmd + " " + line);
	} else {
		ok = true;
		
		set<string>::iterator it = cmd_ign_list.find(cmd);
		if (it == cmd_ign_list.end())
			log.add(Log::OK, devname + ": <- " + stat + " " + cmd + " " + line);
		// Common functions are complete, call on_message for further parsing. 
		// DeviceCtrl::on_message() is virtual so it will call the derived class first.
		
		// Only call in case of success
		on_message(orig);
	}
}

void DeviceCtrl::on_message(string line) {
	// Discard first 'ok' or 'err' (DeviceCtrl::on_message_common() already parsed this)
	string stat = popword(line);

	// Parse list of commands device can accept here
	string what = popword(line);
	if (what == "commands") {
		// The rest should be semicolon-delimited commands: "<cmd> [opts]; <cmd2> [opts];" etc.
		int ncmds = popint32(line);
		
		devcmds.clear();
		for (int i=0; i<ncmds; i++) {
			string cmd = popword(line, ";");
			if (cmd == "")
				break;
			devcmds.push_back(cmd);
		}
		// Sort alphabetically & notify GUI of update
		devcmds.sort();
		signal_commands();
		return;
	} else
		log.add(Log::WARNING, "Unknown response: " + devname + ": <- " + stat + " " + what);

	signal_message();
}

void DeviceCtrl::on_connected(bool conn) {
	log.term(format("%s (%d)", __PRETTY_FUNCTION__, conn));

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
