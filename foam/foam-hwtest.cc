/*
 foam-hwtest.cc -- hardware test module
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

#include <iostream>
#include <string>

#include "foam.h"
#include "types.h"
#include "io.h"
#include "devices.h"
#include "camera.h"
#include "fw1394cam.h"

#include "foam-hwtest.h"

using namespace std;

// Global device list for easier access
FW1394Camera *testcam;

int FOAM_hwtest::load_modules() {
	io.msg(IO_DEB2, "FOAM_hwtest::load_modules()");
	io.msg(IO_INFO, "This is FOAM::hwtest.");
		
	// Add ImgCam device
	try {
		testcam = new FW1394Camera(io, ptc, "1394testcam", ptc->listenport, ptc->conffile);
	}
	catch (foam::Device::exception &e) {
		io.msg(IO_ERR | IO_FATAL, "Could not init FW1394Camera(): %s", e.what());
	}
	devices->add((foam::Device *) testcam);
//	imgcamb = new ImgCamera(io, "imgcamB", ptc->listenport, ptc->cfg);
//	devices->add((Device *) imgcamb);
//	imgcamc = new ImgCamera(io, "imgcamC", ptc->listenport, ptc->cfg);
//	devices->add((Device *) imgcamc);
	
	return 0;
}

// OPEN LOOP ROUTINES //
/*********************/

int FOAM_hwtest::open_init() {
	io.msg(IO_DEB2, "FOAM_hwtest::open_init()");
	
	((FW1394Camera*) devices->get("1394testcam"))->set_mode(Camera::RUNNING);
	
	return 0;
}

int FOAM_hwtest::open_loop() {
	io.msg(IO_DEB2, "FOAM_hwtest::open_loop()");
	static FW1394Camera *tmpcam = ((FW1394Camera*) devices->get("1394testcam"));
	
	usleep(1000000);
	//Camera::frame_t *frame = 
	tmpcam->get_last_frame();
	
	return 0;
}

int FOAM_hwtest::open_finish() {
	io.msg(IO_DEB2, "FOAM_hwtest::open_finish()");
	
	((FW1394Camera*) devices->get("1394testcam"))->set_mode(Camera::WAITING);

	return 0;
}

// CLOSED LOOP ROUTINES //
/************************/

int FOAM_hwtest::closed_init() {
	io.msg(IO_DEB2, "FOAM_hwtest::closed_init()");
	
	// Run open-loop init first
	open_init();
	
	return 0;
}

int FOAM_hwtest::closed_loop() {
	io.msg(IO_DEB2, "FOAM_hwtest::closed_loop()");

	usleep(10);
	return 0;
}

int FOAM_hwtest::closed_finish() {
	io.msg(IO_DEB2, "FOAM_hwtest::closed_finish()");
	
	// Run open-loop finish first
	open_finish();

	return 0;
}

// MISC ROUTINES //
/*****************/

int FOAM_hwtest::calib() {
	io.msg(IO_DEB2, "FOAM_hwtest::calib()=%s", ptc->calib.c_str());

	if (ptc->calib == "INFLUENCE") {
		io.msg(IO_DEB2, "FOAM_hwtest::calib INFLUENCE");
		usleep((useconds_t) 1.0 * 1000000);
		return 0;
	}
	else
		return -1;

	return 0;
}

void FOAM_hwtest::on_message(Connection *connection, string line) {
	io.msg(IO_DEB2, "FOAM_hwtest::on_message(line=%s)", line.c_str());
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
												":  mode=influence:       Measure wfs-wfc influence.");
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
		{
			pthread::mutexholder h(&mode_mutex);
			mode_cond.signal();						// signal a change to the threads
		}
	}
	else if (!netio.ok) {
		connection->write("err cmd :cmd unkown");
	}
}

int main(int argc, char *argv[]) {
	FOAM_hwtest foam(argc, argv);
	
	if (foam.init())
		exit(-1);

	foam.io.msg(IO_INFO, "Running hwtest mode");
	foam.listen();
	
	return 0;
}
