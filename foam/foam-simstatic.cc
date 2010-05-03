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

#include <iostream>

#include "foam.h"
#include "types.h"
#include "io.h"
#include "devices.h"
#include "imgcam.h"

#include "foam-simstatic.h"

// Global device list for easier access
ImgCamera *imgcam;

class DeviceA : public Device {
public:
	DeviceA(Io &io, string name, string port): Device(io, name, port) {
		io.msg(IO_DEB2, "DeviceA::DeviceA()");
	}
	virtual ~DeviceA() {
		io.msg(IO_DEB2, "DeviceA::~DeviceA()");
	}
	virtual void on_message(Connection *conn, std::string line) {
		io.msg(IO_DEB2, "DeviceA::on_message(conn, line=%s)", line.c_str());
	}
	virtual int verify() {
		io.msg(IO_DEB2, "DeviceA::verify()");
		return 0;
	}
	
	int measure() { 
		io.msg(IO_DEB2, "DeviceA::measure()");
		return 10; 
	}
};

int FOAM_simstatic::load_modules() {
	io.msg(IO_DEB2, "FOAM_simstatic::load_modules()");
	io.msg(IO_INFO, "This is the simstatic prime module, enjoy.");
		
	// Add ImgCam device
	imgcam = new ImgCamera(io, "imgcam", ptc->listenport, ptc->cfgfile);
	devices->add((Device *) imgcam);
	
//
//	// Add another device
//	DeviceA *deva2 = new DeviceA(io, "DEVICEA:2", ptc->listenport);
//	devices->add((Device *) deva2);
//
//	// Do something
//	((DeviceA*) devices->get("DEVICEA:1"))->measure();
	
	// io.msg(IO_XNFO, "list: %s", devices->getlist().c_str());
	
	return 0;
}

// OPEN LOOP ROUTINES //
/*********************/

int FOAM_simstatic::open_init() {
	io.msg(IO_DEB2, "FOAM_simstatic::open_init()");
	
	imgcam->set_mode(Camera::RUNNING);
	//ptc->devices->get("DEVICEA:1")->calibrate();
	
	return 0;
}

int FOAM_simstatic::open_loop() {
	io.msg(IO_DEB2, "FOAM_simstatic::open_loop()");
		
	usleep(1000000);
	imgcam->set_mode(Camera::RUNNING);
	
	return 0;
}

int FOAM_simstatic::open_finish() {
	io.msg(IO_DEB2, "FOAM_simstatic::open_finish()");
	
	imgcam->set_mode(Camera::OFF);

	return 0;
}

// CLOSED LOOP ROUTINES //
/************************/

int FOAM_simstatic::closed_init() {
	io.msg(IO_DEB2, "FOAM_simstatic::closed_init()");
	
	// Run open-loop init first
	open_init();
	
	return 0;
}

int FOAM_simstatic::closed_loop() {
	io.msg(IO_DEB2, "FOAM_simstatic::closed_loop()");

	usleep(1000000);
	return 0;
}

int FOAM_simstatic::closed_finish() {
	io.msg(IO_DEB2, "FOAM_simstatic::closed_finish()");
	
	// Run open-loop finish first
	open_finish();

	return 0;
}

// MISC ROUTINES //
/*****************/

int FOAM_simstatic::calib() {
	io.msg(IO_DEB2, "FOAM_simstatic::calib()=%s", ptc->calib.c_str());

	if (ptc->calib == "INFLUENCE") {
		io.msg(IO_DEB2, "FOAM_simstatic::calib INFLUENCE");
		usleep((useconds_t) 1.0 * 1000000);
		return 0;
	}
	else
		return -1;

	return 0;
}

void FOAM_simstatic::on_message(Connection *connection, std::string line) {
	io.msg(IO_DEB2, "FOAM_simstatic::on_message(Connection *connection, std::string line)");
	netio.ok = true;
	
	// First let the parent process this
	FOAM::on_message(connection, line);
	
	string cmd = popword(line);
	
	if (cmd == "help") {
		string topic = popword(line);
		if (topic.size() == 0) {
			connection->write(\
												":==== simstat help ==========================\n"
												":calib <mode>:           Calibrate AO system.");
		}
		else if (topic == "calib") {
			connection->write(\
												":calib <mode>:           Calibrate AO system.\n"
												":  mode=influence:       Measure wfs-wfc influence.\n"
												":  mode=subapsel:        Select subapertures.");
		}
		else if (!netio.ok) {
			connection->write("err cmd help :topic unkown");
		}
	}
	else if (cmd == "get") {
		string what = popword(line);
		if (what == "calib") {
			connection->write("ok var calib 1 influence");
		}
		else if (!netio.ok) {
			connection->write("err get var :var unkown");
		}
	}
	else if (cmd == "calib") {
		string calmode = popword(line);
		connection->write("ok cmd calib");
		ptc->calib = calmode;
		ptc->mode = AO_MODE_CAL;
		pthread_cond_signal(&mode_cond); // signal a change to the main thread
	}
	else if (!netio.ok) {
		connection->write("err cmd :cmd unkown");
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
