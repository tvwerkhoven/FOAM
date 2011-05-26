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

#include <iostream>
#include <string>

#include "foam.h"
#include "types.h"
#include "io.h"

#include "camera.h"
#include "simulcam.h"
#include "wfs.h"
#include "shwfs.h"
#include "wfc.h"
#include "simulwfc.h"

#include "foam-fullsim.h"

using namespace std;

// Global device list for easier access
SimulWfc *simwfc;
SimulCam *simcam;
Shwfs *simwfs;

int FOAM_FullSim::load_modules() {
	io.msg(IO_DEB2, "FOAM_FullSim::load_modules()");
	io.msg(IO_INFO, "This is the full simulation mode, enjoy.");
	
	// Init WFC simulation
	simwfc = new SimulWfc(io, ptc, "simwfc", ptc->listenport, ptc->conffile);
	devices->add((Device *) simwfc);
	
	// Init camera simulation (using simwfc)
	simcam = new SimulCam(io, ptc, "simcam", ptc->listenport, ptc->conffile, *simwfc);
	devices->add((Device *) simcam);
	
	// Init WFS simulation (using camera)
	simwfs = new Shwfs(io, ptc, "simshwfs", ptc->listenport, ptc->conffile, *simcam);
	devices->add((Device *) simwfs);

	return 0;
}

// OPEN LOOP ROUTINES //
/*********************/

int FOAM_FullSim::open_init() {
	io.msg(IO_DEB2, "FOAM_FullSim::open_init()");
	
	simcam->set_mode(Camera::RUNNING);
	
	return 0;
}

int FOAM_FullSim::open_loop() {
	io.msg(IO_DEB2, "FOAM_FullSim::open_loop()");
	
	// Set random actuation on simulated wavefront corrector
	simwfc->actuate_random();

	Camera::frame_t *frame = simcam->get_next_frame(true);

	Shwfs::wf_info_t *wf_meas = simwfs->measure(frame);
	gsl_vector_float *ctrlcmd = simwfs->comp_ctrlcmd(wf_meas);

	usleep(0.1 * 1000000);
	return 0;
}

int FOAM_FullSim::open_finish() {
	io.msg(IO_DEB2, "FOAM_FullSim::open_finish()");
	
	simcam->set_mode(Camera::OFF);
	
	return 0;
}

// CLOSED LOOP ROUTINES //
/************************/

int FOAM_FullSim::closed_init() {
	io.msg(IO_DEB2, "FOAM_FullSim::closed_init()");
	
	simcam->set_mode(Camera::RUNNING);
	
	return 0;
}

int FOAM_FullSim::closed_loop() {
	io.msg(IO_DEB2, "FOAM_FullSim::closed_loop()");

	Camera::frame_t *frame = simcam->get_next_frame(true);
	
	Shwfs::wf_info_t *wf_meas = simwfs->measure(frame);
	gsl_vector_float *ctrlcmd = simwfs->comp_ctrlcmd(wf_meas);
	
	simwfc->actuate(ctrlcmd, gain_t(1,0,0), true);

	usleep(0.1 * 1000000);
	return 0;
}

int FOAM_FullSim::closed_finish() {
	io.msg(IO_DEB2, "FOAM_FullSim::closed_finish()");
	
	simcam->set_mode(Camera::OFF);

	return 0;
}

// MISC ROUTINES //
/*****************/

int FOAM_FullSim::calib() {
	io.msg(IO_DEB2, "FOAM_FullSim::calib()=%s", ptc->calib.c_str());

	if (ptc->calib == "influence") {		// Calibrate influence function
		// Init actuation vector & positions, camera, 
		gsl_vector_float *tmpact = gsl_vector_float_calloc(simwfc->nact);
		float actpos[3] = {-1.0, 0.0, 1.0};
		simcam->set_mode(Camera::RUNNING);
		
		// Loop over all actuators, actuate according to actpos
		for (int i = 0; i < simwfc->nact; i++) {
			for (int p = 0; p < 3; p++) {
				gsl_vector_float_set(tmpact, i, p);
				simwfc->actuate(tmpact, gain_t(1.0, 0.0, 0.0), true);
				Camera::frame_t *frame = simcam->get_next_frame(true);
				simwfs->build_infmat(frame, i, p);
			}
			
			// Set actuator back to 0
			gsl_vector_float_set(tmpact, i, 0.0);
		}
		
		// Calculate the final influence function
		simwfs->calc_infmat();
		simwfs->calc_infmat();
	} 
	else if (ptc->calib == "zero") {	// Calibrate reference/'flat' wavefront
		// Start camera, set wavefront corrector to flat position
		simcam->set_mode(Camera::RUNNING);
		simwfc->actuate(NULL, true);
		
		// Get next frame (wait for it)
		Camera::frame_t *frame = simcam->get_next_frame(true);
		
		// Set this frame as reference
		simwfs->set_reference(frame);
	} 
	else {
		io.msg(IO_WARN, "FOAM_FullSim::calib unknown!");
		return -1;
	}

	return 0;
}

void FOAM_FullSim::on_message(Connection *const conn, string line) {
	io.msg(IO_DEB2, "FOAM_FullSim::on_message(line=%s)", line.c_str());
	
	bool parsed = true;
	string orig = line;
	string cmd = popword(line);
	
	if (cmd == "help") {								// help
		string topic = popword(line);
		parsed = false;
		if (topic.size() == 0) {
			conn->write(\
												":==== full sim help =========================\n"
												":get calibmodes:         List calibration modes\n"
												":calib <mode>:           Calibrate AO system.");
		}
		else if (topic == "calib") {			// help calib
			conn->write(\
												":calib <mode>:           Calibrate AO system.\n"
												":  mode=zero:            Set current WFS data as reference.\n"
												":  mode=influence:       Measure wfs-wfc influence.");
		}
	}
	else if (cmd == "get") {						// get ...
		string what = popword(line);
		if (what == "calibmodes")					// get calibmodes
			conn->write("ok calibmodes 2 zero influence");
		else
			parsed = false;
	}
	else if (cmd == "calib") {					// calib <mode>
		string calmode = popword(line);
		conn->write("ok cmd calib");
		ptc->calib = calmode;
		ptc->mode = AO_MODE_CAL;
		mode_cond.signal();								// signal a change to the main thread
	}
	else
		parsed = false;
	
	if (!parsed)
		FOAM::on_message(conn, orig);
}

int main(int argc, char *argv[]) {
	FOAM_FullSim foam(argc, argv);
	
	if (foam.init())
		exit(-1);
	
	foam.io.msg(IO_INFO, "Running full simulation mode");
	foam.listen();
	
	return 0;
}
