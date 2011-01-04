/*
 devices.cc -- base hardware handling classes
 Copyright (C) 2010--2011 Tim van Werkhoven <t.i.m.vanwerkhoven@xs4all.nl>
 
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

#ifdef HAVE_CONFIG_H
#include "autoconfig.h"
#endif

#include "io.h"
#include "devices.h"
#include "foamctrl.h"

using namespace std;

// Device class

//Device::Device(Io &io, foamctrl *ptc, string n, string t, string p, Path &conf): 
//io(io), ptc(ptc), name(n), type("dev." + t), port(p), conffile(conf), netio(p, n)
//{ 
//	init();
//}

Device::Device(Io &io, foamctrl *ptc, const string n, const string t, const string p, const Path conf, const bool online): 
io(io), ptc(ptc), name(n), type("dev." + t), port(p), conffile(conf), netio(p, n), online(online)
{ 
	init();
}

bool Device::init() {
	io.msg(IO_XNFO, "Device::Device(): Create new device, name=%s, type=%s", 
				 name.c_str(), type.c_str());
	
	// Only parse config file if we have one
	if (conffile.isset()) {
		cfg.parse(conffile, name);
	
		string _type = cfg.getstring("type");
		if (_type != type) 
			throw exception("Device::Device(): Type should be " + type + " for this Device (" + _type + ")!");
	}

	io.msg(IO_XNFO, "Device %s listening on port %s.", name.c_str(), port.c_str());
	if (online) {
		netio.slot_message = sigc::mem_fun(this, &Device::on_message);
		netio.slot_connected = sigc::mem_fun(this, &Device::on_connect);
		netio.listen();
	}
	
	return true;
}

Device::~Device() {
	io.msg(IO_DEB2, "Device::~Device()");
}

void Device::on_message(Connection * const conn, std::string line) { 
	io.msg(IO_DEB2, "Device::on_message('%s') %s::%s", 
				 line.c_str(), type.c_str(), name.c_str());
	string orig = line;
	
	string command = popword(line);
	if (command == "get") {
		string what = popword(line);
		if (what == "commands") {
			string devlist = "";
			
			list<string>::iterator it;
			int ndev=0;
			for (it=cmd_list.begin(); it!=cmd_list.end(); ++it) {
				devlist += *it + ";";
				ndev++;
			}
			
			conn->write(format("ok commands %d %s", ndev, devlist.c_str()));
			return;
		}
	}
	
	conn->write("error :Unknown command: " + orig);
}

void Device::get_var(Connection * const conn, const string varname, const double value, const string comment) const {
	if (!conn)
		return;
	
	conn->addtag(varname);
	if (comment == "")
		conn->write(format("ok %s %lf :%s", varname.c_str(), (double) value, comment.c_str()));
	else 
		conn->write(format("ok %s %lf", varname.c_str(), (double) value));
}


// DeviceManager class

DeviceManager::DeviceManager(Io &io): io(io), ndev(0) {
	io.msg(IO_DEB2, "DeviceManager::DeviceManager()");
}

DeviceManager::~DeviceManager() {
	io.msg(IO_DEB2, "DeviceManager::~DeviceManager()");
	device_t::iterator it;
	for (it=devices.begin() ; it != devices.end(); it++)
		delete (*it).second;
	
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
		throw exception("Device " + id + " does not exist!");
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

string DeviceManager::getlist(bool showtype, bool showonline) {
	device_t::iterator it;
	int num=0;
	string devlist = "";
	for (it=devices.begin() ; it != devices.end(); it++) {
		// Skip devices that are not online if requested
		if (!it->second->isonline() && showonline)
			continue;
		devlist += it->first + " ";
		if (showtype) devlist += it->second->gettype() + " ";
		num++;
	}
	
	devlist = format("%d ", num) + devlist;
	return devlist;
}


