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

DeviceCtrl::DeviceCtrl(const string h, const string p, const string n):
	host(h), port(p), devname(n), devinfo("null")
{
	printf("%x:DeviceCtrl::DeviceCtrl(name=%s)\n", (int) pthread_self(), n.c_str());	
	
	// Open control connection
	protocol.slot_message = sigc::mem_fun(this, &DeviceCtrl::on_message);
	protocol.slot_connected = sigc::mem_fun(this, &DeviceCtrl::on_connected);
	printf("%x:DeviceCtrl::DeviceCtrl(): connecting to %s:%s@%s\n", (int) pthread_self(), host.c_str(), port.c_str(), devname.c_str());
	protocol.connect(host, port, devname);	
	
}

void DeviceCtrl::on_message(string line) {
	printf("%x:DeviceCtrl::on_message(line=%s)\n", (int) pthread_self(), line.c_str());	
	
	if (popword(line) != "ok") {
		ok = false;
		errormsg = popword(line);
		signal_update();
		return;
	}
	
	ok = true;
	
	string what = popword(line);
	
	if (what == "info") {
		devinfo = line;
	}
	
	signal_update();
}

void DeviceCtrl::on_connected(bool connected) {
	printf("%x:DeviceCtrl::on_connected(status=%d)\n", (int) pthread_self(), connected);	
	if (connected)
		protocol.write("info");
	
}