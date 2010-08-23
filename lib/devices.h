/*
 devices.h -- base hardware handling classes
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

#ifndef HAVE_DEVICES_H
#define HAVE_DEVICES_H

#include <map>

#include "io.h"
#include "protocol.h"

typedef Protocol::Server::Connection Connection;

// Device class to handle devices in a generic fashion
class Device {
protected:
	Io &io;
	string name;												//!< Device name
	string type;												//!< Device type
	string port;												//!< Port to listen on
	Protocol::Server *protocol;
	
public:
	Device(Io &io, string name, string type, string port);
	virtual ~Device();
	
	// Should verify the integrity of the device.
	virtual int verify() { return 0; }
	// Network IO handling routines
	virtual void on_message(Connection *conn, std::string line) { io.msg(IO_DEB2, "Device::on_message(line='%s')", line.c_str()); }
	virtual void on_connect(Connection *conn, bool status) { io.msg(IO_DEB2, "Device::on_connect(stat=%d)", (int) status); }
	
	string getname() { return name; }
	string gettype() { return type; }
};

// Keep track of all devices in the system
class DeviceManager {
private:
	Io &io;
	
	typedef map<string, Device*> device_t;
	device_t devices;
	int ndev;
	
public:
	// Add a device to the list, get the name from the device to use as key.
	int add(Device *dev);
	// Get a device from the list. Return NULL if not found.
	Device* get(string id);
	// Delete a device from the list. Return -1 if not found.
	int del(string id);
	
	// Return a list of all devices in the list
	string getlist(bool showtype = true);
	
	// Return number of devices 
	int getcount() { return ndev; }
	
	DeviceManager(Io &io);
	~DeviceManager();
};

#endif // HAVE_DEVICES_H
