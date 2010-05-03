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

#include "io.h"
#include "devices.h"

Device::Device(Io &io, string name, string port): io(io), name(name), port(port) { 
	io.msg(IO_XNFO, "Device::Device(): Create new device, name=%s", name.c_str());
	
	protocol = new Protocol::Server(port, name);
	io.msg(IO_XNFO, "Device %s listening on port %s.", name.c_str(), port.c_str());
	protocol->slot_message = sigc::mem_fun(this, &Device::on_message);
	protocol->slot_connected = sigc::mem_fun(this, &Device::on_connect);
	protocol->listen();
}

Device::~Device() {
	io.msg(IO_DEB2, "Device::~Device()");
	
	delete protocol;
}

int DeviceManager::add(Device *dev) {
	string id = dev->getname();
	if (devices.find(id) != devices.end()) {
		io.msg(IO_ERR, "ID '%s' already exists!", id.c_str());
		return -1;
	}
	devices[id] = dev;
	ndev++;
	return 0;
}

Device* DeviceManager::get(string id) {
	if (devices.find(id) == devices.end()) {
		io.msg(IO_ERR, "ID '%s' does not exist!", id.c_str());
		return NULL;
	}
	return devices[id];
}

int DeviceManager::del(string id) {
	if (devices.find(id) == devices.end()) {
		io.msg(IO_ERR, "ID '%s' does not exist!", id.c_str());
		return -1;
	}
	devices.erase(devices.find(id));
	ndev--;
	return 0;
}

string DeviceManager::getlist() {
	device_t::iterator it;
	string devlist = "";
	for (it=devices.begin() ; it != devices.end(); it++)
		devlist += (*it).first + " ";
	
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
