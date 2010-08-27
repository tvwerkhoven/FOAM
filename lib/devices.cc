/*
 devices.cc -- base hardware handling classes
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

/*! 
 @file devices.cc
 @author Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)
 @brief Generic device class, specific hardware controls are derived from this class.
 */

#include "io.h"
#include "devices.h"
#include "foamctrl.h"

// Device class

Device::Device(Io &io, foamctrl *ptc, string n, string t, string p, string cf): 
io(io), name(n), type("dev." + t), port(p), conffile(cf), cfg(cf, n), netio(p, n)
{ 
	io.msg(IO_XNFO, "Device::Device(): Create new device, name=%s, type=%s", 
				 name.c_str(), type.c_str());
	
	string _type = cfg.getstring("type");
	if (_type != type) 
		throw exception("Type should be " + type + " for this Device!");
	
	io.msg(IO_XNFO, "Device %s listening on port %s.", name.c_str(), port.c_str());
	netio.slot_message = sigc::mem_fun(this, &Device::on_message);
	netio.slot_connected = sigc::mem_fun(this, &Device::on_connect);
	netio.listen();
}

Device::~Device() {
	io.msg(IO_DEB2, "Device::~Device()");
}

int DeviceManager::add(Device *dev) {
	string id = dev->getname();
	if (devices.find(id) != devices.end()) {
		io.msg(IO_ERR, "Device ID '%s' already exists!", id.c_str());
		return -1;
	}
	devices[id] = dev;
	ndev++;
	return 0;
}

Device* DeviceManager::get(string id) {
	if (devices.find(id) == devices.end()) {
		io.msg(IO_ERR, "Device ID '%s' does not exist!", id.c_str());
		return NULL;
	}
	return devices[id];
}

int DeviceManager::del(string id) {
	if (devices.find(id) == devices.end()) {
		io.msg(IO_ERR, "Device ID '%s' does not exist!", id.c_str());
		return -1;
	}
	devices.erase(devices.find(id));
	ndev--;
	return 0;
}

string DeviceManager::getlist(bool showtype) {
	device_t::iterator it;
	string devlist = "";
	for (it=devices.begin() ; it != devices.end(); it++) {
		devlist += (*it).first + " ";
		if (showtype) devlist += (*it).second->gettype() + " ";
	}
	
	return devlist;
}

DeviceManager::DeviceManager(Io &io): io(io), ndev(0) {
	io.msg(IO_DEB2, "DeviceManager::DeviceManager()");
}

DeviceManager::~DeviceManager() {
	io.msg(IO_DEB2, "DeviceManager::~DeviceManager()");
	device_t::iterator it;
	for (it=devices.begin() ; it != devices.end(); it++)
		delete (*it).second;
	
}
