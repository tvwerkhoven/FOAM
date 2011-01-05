/*
 foam-full.cc -- full simulation module
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
 @file foam-fullsim.c
 @author Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)
 @brief This is a full simulation mode
 
 This is a full simulation mode, where the complete system from atmosphere to 
 CCD is taken into account.
 */

#include <iostream>
#include <string>

#include "foam.h"
#include "types.h"
#include "io.h"
#include "simulcam.h"
#include "shwfs.h"

#include "foam-fullsim.h"

// Global device list for easier access
SimulCam *simcam;
Shwfs *simwfs;
Wfs *wfs;

int FOAM_FullSim::load_modules() {
	io.msg(IO_DEB2, "FOAM_FullSim::load_modules()");
	io.msg(IO_INFO, "This is the full simulation mode, enjoy.");
		
	// Add Simulcam device
	simcam = new SimulCam(io, ptc, "simcam", ptc->listenport, ptc->conffile);
	devices->add((Device *) simcam);

	// Add new Wfs based on simulcam
	wfs = new Wfs(io, ptc, "simwfs", ptc->listenport, ptc->conffile, *simcam);
	devices->add((Device *) wfs);
	
	// Add new Shwfs based on simulcam
	simwfs = new Shwfs(io, ptc, "simshwfs", ptc->listenport, ptc->conffile, *simcam);
	devices->add((Device *) simwfs);

	return 0;
}

// OPEN LOOP ROUTINES //
/*********************/

int FOAM_FullSim::open_init() {
	io.msg(IO_DEB2, "FOAM_FullSim::open_init()");
	
	((SimulCam*) devices->get("simcam"))->set_mode(Camera::RUNNING);
	
	return 0;
}

int FOAM_FullSim::open_loop() {
	io.msg(IO_DEB2, "FOAM_FullSim::open_loop()");
	static SimulCam *tmpcam = ((SimulCam*) devices->get("simcam"));
	
	usleep(1000000);
	Camera::frame_t *frame = tmpcam->get_last_frame();
	
	return 0;
}

int FOAM_FullSim::open_finish() {
	io.msg(IO_DEB2, "FOAM_FullSim::open_finish()");
	
	((SimulCam*) devices->get("simcam"))->set_mode(Camera::OFF);

	return 0;
}

// CLOSED LOOP ROUTINES //
/************************/

int FOAM_FullSim::closed_init() {
	io.msg(IO_DEB2, "FOAM_FullSim::closed_init()");
	
	// Run open-loop init first
	open_init();
	
	return 0;
}

int FOAM_FullSim::closed_loop() {
	io.msg(IO_DEB2, "FOAM_FullSim::closed_loop()");

	usleep(10);
	return 0;
}

int FOAM_FullSim::closed_finish() {
	io.msg(IO_DEB2, "FOAM_FullSim::closed_finish()");
	
	// Run open-loop finish first
	open_finish();

	return 0;
}

// MISC ROUTINES //
/*****************/

int FOAM_FullSim::calib() {
	io.msg(IO_DEB2, "FOAM_FullSim::calib()=%s", ptc->calib.c_str());

	if (ptc->calib == "INFLUENCE") {
		io.msg(IO_DEB2, "FOAM_FullSim::calib INFLUENCE");
		usleep((useconds_t) 1.0 * 1000000);
		return 0;
	}
	else
		return -1;

	return 0;
}

void FOAM_FullSim::on_message(Connection *const conn, std::string line) {
	io.msg(IO_DEB2, "FOAM_FullSim::on_message(line=%s)", line.c_str());
	netio.ok = true;
	
	// First let the parent process this
	FOAM::on_message(conn, line);
	
	string cmd = popword(line);
	
	if (cmd == "help") {
		string topic = popword(line);
		if (topic.size() == 0) {
			conn->write(\
												":==== full sim help =========================\n"
												":calib <mode>:           Calibrate AO system.");
		}
		else if (topic == "calib") {
			conn->write(\
												":calib <mode>:           Calibrate AO system.\n"
												":  mode=influence:       Measure wfs-wfc influence.");
		}
		else if (!netio.ok) {
			conn->write("err cmd help :topic unkown");
		}
	}
	else if (cmd == "get") {
		string what = popword(line);
		if (what == "calib") {
			conn->write("ok var calib 1 influence");
		}
		else if (!netio.ok) {
			conn->write("err get var :var unkown");
		}
	}
	else if (cmd == "calib") {
		string calmode = popword(line);
		conn->write("ok cmd calib");
		ptc->calib = calmode;
		ptc->mode = AO_MODE_CAL;
		mode_cond.signal();						// signal a change to the main thread
	}
	else if (!netio.ok) {
		conn->write("err cmd :cmd unkown");
	}
}

int main(int argc, char *argv[]) {
	FOAM_FullSim foam(argc, argv);
		
	foam.io.msg(IO_INFO, "Running full simulation mode");
	foam.listen();
	
	return 0;
}
