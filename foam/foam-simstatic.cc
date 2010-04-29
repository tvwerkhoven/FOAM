/*
 foam-simstatic.cc -- static simulation module
 Copyright (C) 2008--2010 Tim van Werkhoven <t.i.m.vanwerkhoven@xs4all.nl>
 
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
 @file foam-simstatic.c
 @author Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)
 @brief This is a static simulation mode, with just a simple image to work with.
 
 This setup can be used to benchmark performance of the AO system if no
 AO hardware (camera, TT, DM) is present.
 */

#include "types.h"
#include "io.h"
#include "shwfs.h"
#include "cam.h"
#include "foam.h"
#include "foam-simstatic.h"

#include <map>
#include <iostream>

class Device {
protected:
	Io &io;
	string name;												//!< Device name
	string port;												//!< Port to listen on
	Protocol::Server *protocol;
	
public:
	Device(Io &io, string name, string port): io(io), name(name), port(port) { 
		io.msg(IO_DEB2, "Device::Device(): Create new device, name=%s\n", name.c_str());
		
		protocol = new Protocol::Server(port, name);
		protocol->slot_message = sigc::mem_fun(this, &Device::on_message);
		protocol->slot_message = sigc::mem_fun(this, &Device::on_connect);
		protocol->listen();
	}
	virtual ~Device() {
		io.msg(IO_DEB2, "Device::~Device()\n");
		delete protocol;
	}
	
	// Should verify the integrity of the device.
	virtual int verify() { return 0; }
	// Network IO handling routines
	virtual void on_message(Connection *conn, std::string line) { ; }
	virtual	void on_connect(Connection *conn, bool status) { ; }

	string getname() { return name; }
};

class DeviceA : public Device {
public:
	DeviceA(Io &io, string name, string port): Device(io, name, port) {
		io.msg(IO_DEB2, "DeviceA::DeviceA()\n");
	}
	virtual ~DeviceA() {
		io.msg(IO_DEB2, "DeviceA::~DeviceA()\n");
	}
	virtual void on_message(Connection *conn, std::string line) {
		io.msg(IO_DEB2, "DeviceA::on_message(conn, line=%s)\n", line.c_str());
	}
	virtual int verify() {
		io.msg(IO_DEB2, "DeviceA::verify()\n");
		return 0;
	}
	
	int measure() { 
		io.msg(IO_DEB2, "DeviceA::measure()\n");
		return 10; 
	}
};

class DeviceManager {
private:
	Io &io;
	
	typedef map<string, Device*> device_t;
	device_t devices;
	int ndev;
	
public:
	// Add a device to the list, get the name from the device to use as key.
	int add(Device *dev) {
		string id = dev->getname();
		if (devices.find(id) != devices.end()) {
			printf("ID '%s' already exists!\n", id.c_str());
			return -1;
		}
		devices[id] = dev;
		ndev++;
		return 0;
	}
	// Get a device from the list. Return NULL if not found.
	Device* get(string id) {
		if (devices.find(id) == devices.end()) {
			io.msg(IO_DEB2, "ID '%s' does not exist!\n", id.c_str());
			return NULL;
		}
		return devices[id];
	}
	// Delete a device from the list. Return -1 if not found.
	int del(string id) {
		if (devices.find(id) == devices.end()) {
			io.msg(IO_DEB2, "ID '%s' does not exist!\n", id.c_str());
			return -1;
		}
		devices.erase(devices.find(id));
		ndev--;
		return 0;
	}
	
	// Return a list of all devices in the list
	string getlist() {
		device_t::iterator it;
		string devlist = "";
		for (it=devices.begin() ; it != devices.end(); it++ ) {
			cout << (*it).first << " => " << (*it).second << endl;
			devlist += (*it).first + " ";
		}
		
		return devlist;
	}
	
	// Return number of devices 
	int getcount() {
		return ndev;
	}
	
	DeviceManager(Io &io): io(io), ndev(0) {
		io.msg(IO_DEB2, "DeviceManager::DeviceManager()\n");
	}
	~DeviceManager() {
		io.msg(IO_DEB2, "DeviceManager::~DeviceManager()\n");
	}
};

DeviceManager *devices;

int FOAM_simstatic::load_modules() {
	io.msg(IO_DEB2, "FOAM_simstatic::load_modules()");
	io.msg(IO_INFO, "This is the simstatic prime module, enjoy.");
		
	devices = new DeviceManager(io);
	
	// Add camera device
	DeviceA *deva1 = new DeviceA(io, "DEVICEA:1", ptc->listenport);
	devices->add((Device *) deva1);

	// Add another device
	DeviceA *deva2 = new DeviceA(io, "DEVICEA:2", ptc->listenport);
	devices->add((Device *) deva2);

	// Do something
	((DeviceA*) devices->get("DEVICEA:1"))->measure();
	
	io.msg(IO_XNFO, "list: %s\n", devices->getlist().c_str());
	
	//return 0;
	exit(0);
}

// OPEN LOOP ROUTINES //
/*********************/

int FOAM_simstatic::open_init() {
	io.msg(IO_DEB2, "FOAM_simstatic::open_init()");
	
	//ptc->devices->get("DEVICEA:1")->calibrate();
	
	return 0;
}

int FOAM_simstatic::open_loop() {
	io.msg(IO_DEB2, "FOAM_simstatic::open_loop()");
		
	usleep(1000000);
	
	return 0;
}

int FOAM_simstatic::open_finish() {
	io.msg(IO_DEB2, "FOAM_simstatic::open_finish()");
	
//	ptc->wfs[0]->cam->set_mode(Camera::OFF);

	return 0;
}

// CLOSED LOOP ROUTINES //
/************************/

int FOAM_simstatic::closed_init() {
	io.msg(IO_DEB2, "FOAM_simstatic::closed_init()");
	
	// Run open-loop init first
	open_init();
	
	return EXIT_SUCCESS;
}

int FOAM_simstatic::closed_loop() {
	io.msg(IO_DEB2, "FOAM_simstatic::closed_loop()");

	usleep(1000000);
	return EXIT_SUCCESS;
}

int FOAM_simstatic::closed_finish() {
	io.msg(IO_DEB2, "FOAM_simstatic::closed_finish()");
	
	// Run open-loop finish first
	open_finish();

	return EXIT_SUCCESS;
}

// MISC ROUTINES //
/*****************/

int FOAM_simstatic::calib() {
	io.msg(IO_DEB2, "FOAM_simstatic::calib()=%s", ptc->calib.c_str());

	if (ptc->calib == "INFLUENCE") {
		io.msg(IO_DEB2, "FOAM_simstatic::calib INFLUENCE");
		usleep((useconds_t) 1.0 * 1000000);
	}

	return EXIT_SUCCESS;
}

void FOAM_simstatic::on_message(Connection *connection, std::string line) {
	io.msg(IO_DEB2, "FOAM_simstatic::on_message(Connection *connection, std::string line)");
	netio.ok = true;
	
	// First let the parent process this
	FOAM::on_message(connection, line);
	
	string cmd = popword(line);
	
	if (cmd == "HELP") {
		string topic = popword(line);
		if (topic.size() == 0) {
			connection->write(\
												":==== simstat help ==========================\n"
												":calib <mode>:           Calibrate AO system.");
		}
		else if (topic == "CALIB") {
			connection->write(\
												":calib <mode>:           Calibrate AO system.\n"
												":  mode=influence:       Measure wfs-wfc influence.\n"
												":  mode=subapsel:        Select subapertures.");
		}
		else if (!netio.ok) {
			connection->write("ERR CMD HELP :TOPIC UNKOWN");
		}
	}
	else if (cmd == "GET") {
		string what = popword(line);
		if (what == "CALIB") {
			connection->write("OK VAR CALIB 1 INFLUENCE");
		}
		else if (what == "DEVICES") {
			connection->write("OK VAR DEVICES %d %s", devices->getcount(), devices->getlist());
		}
		else if (!netio.ok) {
			connection->write("ERR GET VAR :VAR UNKOWN");
		}
	}
	else if (cmd == "CALIB") {
		string calmode = popword(line);
		connection->write("OK CMD CALIB");
		ptc->calib = calmode;
		ptc->mode = AO_MODE_CAL;
		pthread_cond_signal(&mode_cond); // signal a change to the main thread
	}
	else if (!netio.ok) {
		connection->write("ERR CMD :CMD UNKOWN");
	}
}

int main(int argc, char *argv[]) {
	FOAM_simstatic foam(argc, argv);
	
	if (foam.has_error())
		return foam.io.msg(IO_INFO, "Initialisation error.");
	
	if (foam.init())
		return foam.io.msg(IO_ERR, "Configuration error.");
		
	foam.io.msg(IO_INFO, "Running simstatic mode");
	
	foam.listen();
	
	return 0;
}
