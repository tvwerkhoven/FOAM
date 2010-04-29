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
public:
	string name;
	
	Device(string name): name(name) { 
		printf("Device::Device(): Create new device, name=%s\n", name.c_str());
	}
	~Device() {
		printf("Device::~Device()\n");
	}
	string getname() { return name; }
};

class DeviceA : public Device {
public:
	DeviceA(string name): Device(name) {
		printf("DeviceA::DeviceA()\n");
	}
	~DeviceA() {
		printf("DeviceA::~DeviceA()\n");
	}
	int measure() { 
		printf("DeviceA::measure()\n");
		return 10; 
	}
};

class DeviceManager {
public:
	typedef map<string, Device*> device_t;
	device_t devices;
	int ndev;
	
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
			printf("ID '%s' does not exist!\n", id.c_str());
			return NULL;
		}
		return devices[id];
	}
	// Delete a device from the list. Return -1 if not found.
	int del(string id) {
		if (devices.find(id) == devices.end()) {
			printf("ID '%s' does not exist!\n", id.c_str());
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
	int getcount {
		return ndev;
	}
	
	DeviceManager(): ndev(0) {
		printf("DeviceManager::DeviceManager()\n");
	}
	~DeviceManager() {
		printf("DeviceManager::~DeviceManager()\n");
	}
};

DeviceManager *devices;

int FOAM_simstatic::load_modules() {
	io.msg(IO_DEB2, "FOAM_simstatic::load_modules()");
	io.msg(IO_INFO, "This is the simstatic prime module, enjoy.");
		
	devices = new DeviceManager;
	
	// Add camera device
	DeviceA *deva1 = new DeviceA("DEVICEA:1");
	devices->add((Device *) deva1);

	DeviceA *deva2 = new DeviceA("DEVICEA:2");
	devices->add((Device *) deva2);

	((DeviceA*) devices->get("DEVICEA:1"))->measure();
	
	printf("list: %s\n", devices->getlist().c_str());
	
	//return 0;
	exit(0);
}

// OPEN LOOP ROUTINES //
/*********************/

int FOAM_simstatic::open_init() {
	io.msg(IO_DEB2, "FOAM_simstatic::open_init()");
	
	ptc->wfs[0]->cam->set_mode(Camera::RUNNING);
	ptc->wfs[0]->cam->init_capture();
	
	return 0;
}

int FOAM_simstatic::open_loop() {
	io.msg(IO_DEB2, "FOAM_simstatic::open_loop()");
	
	//void *tmp;
	//ptc->wfs[0]->cam->get_image(&tmp);
	ptc->wfs[0]->measure(0);
	
	usleep(1000000);
	
	return 0;
}

int FOAM_simstatic::open_finish() {
	io.msg(IO_DEB2, "FOAM_simstatic::open_finish()");
	
	ptc->wfs[0]->cam->set_mode(Camera::OFF);

	return 0;
}

// CLOSED LOOP ROUTINES //
/************************/

int FOAM_simstatic::closed_init() {
	io.msg(IO_DEB2, "FOAM_simstatic::closed_init()");
	
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
	
	// Process everything in uppercase internally
	transform(line.begin(), line.end(), line.begin(), ::toupper);
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
