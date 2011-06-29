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

#ifdef HAVE_CONFIG_H
#include "autoconfig.h"
#endif

#include "io.h"
#include "config.h"
#include "devices.h"
#include "foamctrl.h"

using namespace std;

// Device class

Device::Device(Io &io, foamctrl *const ptc, const string n, const string t, const string p, const Path conf, const bool online): 
is_calib(false), is_ok(false), io(io), ptc(ptc), name(n), type("dev." + t), port(p), conffile(conf), netio(p, n), online(online), outputdir(ptc->datadir)
{ 
	//! @todo This init() can just be placed here?
	init();
}

bool Device::init() {
	io.msg(IO_XNFO, "Device::Device(): Create new device, name=%s, type=%s", 
				 name.c_str(), type.c_str());
	
	add_cmd("get commands");

	// Only parse config file if we have one
	if (conffile.isset()) {
		cfg.parse(conffile, name);
	
		string _type = cfg.getstring("type");
		if (_type != type) 
			throw exception("Device::Device(): Type should be " + type + " for this Device (" + _type + ")!");
	}
	
	if (online) {
		netio.slot_message = sigc::mem_fun(this, &Device::on_message_common);
		netio.slot_connected = sigc::mem_fun(this, &Device::on_connect);
		io.msg(IO_XNFO, "Device %s listening on port %s.", name.c_str(), port.c_str());
	}
	
	// Always listen, also for offline devices. In that latter case, simply don't parse any data.
	netio.listen();

	set_outputdir("");

	return true;
}

Device::~Device() {
	io.msg(IO_DEB2, "Device::~Device()");
	
	// Update master configuration with our (potentially changed) settings
	ptc->cfg->update(&cfg);
}

void Device::on_message_common(Connection * const conn, string line) {
	io.msg(IO_DEB2, "Device::on_message_common('%s') %s", 
				 line.c_str(), name.c_str());
	on_message(conn, line);
}

void Device::on_message(Connection * const conn, string line) { 
	string orig = line;
	bool parsed = true;
	
	string command = popword(line);
	if (command == "get") {							// get ...
		string what = popword(line);
		if (what == "commands") {					// get commands
			string devlist = "";
			
			for (size_t i=0; i < cmd_list.size(); i++)
				devlist += cmd_list[i] + ";";
			
			conn->write(format("ok commands %d %s", (int) cmd_list.size(), devlist.c_str()));
//		} else if(what == "outputdir") {
//			conn->addtag("outputdir");
//			conn->write("ok outputdir :" + outputdir.str());
		} else {
			parsed = false;
		}
	} else if (command == "set") {
		string what = popword(line);
//		if (what == "outputdir") {
//			conn->addtag("outputdir");
//			string dir = popword(line);
//			if (set_outputdir(dir))
//				conn->write("error :directory "+dir+" not usable");
//		} else {
		parsed = false;
//		}
	} else {
		parsed = false;
	}
	
	// Command is not known, give an error message back
	if (!parsed)
		conn->write("error :Unknown command: " + orig);
}

void Device::get_var(Connection * const conn, const string varname, const string response) const {
	if (!conn)
		return;
	
	conn->addtag(varname);
	conn->write(response);
}

void Device::get_var(Connection * const conn, const string varname, const double value, const string comment) const {
	if (comment == "")
		get_var(conn, varname, format("ok %s %lf :%s", varname.c_str(), (double) value, comment.c_str()));
	else 
		get_var(conn, varname, format("ok %s %lf", varname.c_str(), (double) value));
}

void Device::set_calib(bool newcalib) {
	is_calib = newcalib;
	netio.broadcast(format("ok calib %d", is_calib));
}

void Device::set_status(bool newstat) {
	is_ok = newstat;
	netio.broadcast(format("ok status %d", newstat));
}

int Device::set_outputdir(const string identifier) {
	// This automatically prefixes ptc->datadir if 'identifier' is not absolute
	Path tmp = ptc->datadir + string(type + "." + name + identifier);

	//! @todo Make recursive mkdir() here
	if (!tmp.exists()) {// Does not exist, create
		if (mkdir(tmp.c_str(), 0755)) // mkdir() returned !0, error
			return 1;
	}
	else {	// Directory exists, check if readable
		if (tmp.access(R_OK | W_OK | X_OK))
			return 2;
	}

	outputdir = tmp;
	netio.broadcast("ok outputdir :" + outputdir.str(), "outputdir");
	return 0;
}



// DeviceManager class

DeviceManager::DeviceManager(Io &io): io(io) {
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
	return 0;
}

string DeviceManager::getlist(bool showtype, bool showonline) {
	string devlist = "";
	int num=0;
	for (device_t::iterator it=devices.begin(); it != devices.end(); it++) {
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


