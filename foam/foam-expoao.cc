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
#include "andorcam.h"
#include "wfs.h"
#include "shwfs.h"
#include "wfc.h"

#include "foam-expoao.h"

using namespace std;

// Global device list for easier access
AndorCam *ixoncam;
Shwfs *ixonwfs;

int FOAM_ExpoAO::load_modules() {
	io.msg(IO_DEB2, "FOAM_ExpoAO::load_modules()");
	io.msg(IO_INFO, "This is the expoao build, enjoy.");
	
	// Init Andor camera
	io.msg(IO_INFO, "Init Andor Ixon Camera...");
	ixoncam = new AndorCam(io, ptc, "ixoncam", ptc->listenport, ptc->conffile);
	devices->add((foam::Device *) ixoncam);
	
	io.msg(IO_INFO, "Andor camera initialized, printing capabilities");
	ixoncam->print_andor_caps();
	
	// Init WFS simulation (using ixoncam)
	ixonwfs = new Shwfs(io, ptc, "ixonwfs", ptc->listenport, ptc->conffile, *ixoncam);
	devices->add((foam::Device *) ixonwfs);

	return 0;
}

// OPEN LOOP ROUTINES //
/*********************/

int FOAM_ExpoAO::open_init() {
	io.msg(IO_DEB2, "FOAM_ExpoAO::open_init()");
	
	simcam->set_mode(Camera::RUNNING);
	
	return 0;
}

int FOAM_ExpoAO::open_loop() {
	io.msg(IO_DEB2, "FOAM_ExpoAO::open_loop()");
	open_perf.addlog(1);
	string vec_str;
	
	// Get next frame from ixoncam
	Camera::frame_t *frame = ixoncam->get_next_frame(true);
	open_perf.addlog(2);
	
	// Analyze frame with shack-hartmann routines
	Shwfs::wf_info_t *wf_meas = ixonwfs->measure(frame);
	open_perf.addlog(3);
	
	// Print analysis
	vec_str = "";
	for (size_t i=0; i < wf_meas->wfamp->size; i++)
		vec_str += format("%.3g ", gsl_vector_float_get(wf_meas->wfamp, i));
	io.msg(IO_INFO, "FOAM_ExpoAO::wfs_m: %s", vec_str.c_str());

	// Wait a little so we can follow the program
	usleep(1.0 * 1000000);
	return 0;
}

int FOAM_ExpoAO::open_finish() {
	io.msg(IO_DEB2, "FOAM_ExpoAO::open_finish()");
	
	FOAM_ExpoAO::open_init();
	
	return 0;
}

// CLOSED LOOP ROUTINES //
/************************/

int FOAM_ExpoAO::closed_init() {
	io.msg(IO_DEB2, "FOAM_ExpoAO::closed_init()");
	
	simcam->set_mode(Camera::RUNNING);
	
	return 0;
}

int FOAM_ExpoAO::closed_loop() {
	io.msg(IO_DEB2, "FOAM_ExpoAO::closed_loop()");
	closed_perf.addlog(1);
	string vec_str;

	// Get next frame from ixoncam
	Camera::frame_t *frame = ixoncam->get_next_frame(true);
	open_perf.addlog(2);

	// Analyze frame with shack-hartmann routines
	Shwfs::wf_info_t *wf_meas = ixonwfs->measure(frame);
	open_perf.addlog(3);
	
	// Print analysis
	vec_str = "";
	for (size_t i=0; i < wf_meas->wfamp->size; i++)
		vec_str += format("%.3g ", gsl_vector_float_get(wf_meas->wfamp, i));
	io.msg(IO_INFO, "FOAM_ExpoAO::wfs_m: %s", vec_str.c_str());

	// Don't wait here, we want to test performance
	return 0;
}

int FOAM_ExpoAO::closed_finish() {
	io.msg(IO_DEB2, "FOAM_ExpoAO::closed_finish()");
	
	FOAM_ExpoAO::open_finish();

	return 0;
}

// MISC ROUTINES //
/*****************/

int FOAM_ExpoAO::calib() {
	io.msg(IO_DEB2, "FOAM_ExpoAO::calib()=%s", ptc->calib.c_str());

	if (ptc->calib == "zero") {	// Calibrate reference/'flat' wavefront
		//gsl_vector_float *tmpact = gsl_vector_float_calloc(simwfc->get_nact());
		// Set wavefront corrector to flat position, start camera
//		simwfc->update_control(tmpact, gain_t(0.0, 0.0, 0.0), 0.0);
//		simwfc->actuate(true);

		io.msg(IO_XNFO, "FOAM_ExpoAO::calib() Start camera...");
		ixoncam->set_mode(Camera::RUNNING);
		
		io.msg(IO_XNFO, "FOAM_ExpoAO::calib() Measure reference...");
		// Get next frame (wait for it)
		Camera::frame_t *frame = ixoncam->get_next_frame(true);
		
		io.msg(IO_XNFO, "FOAM_ExpoAO::calib() Process data...");
		// Set this frame as reference
		ixonwfs->set_reference(frame);
		ixonwfs->store_reference();
		
		// Restore seeing & wfc
		simcam->set_mode(Camera::WAITING);
		gsl_vector_float_free(tmpact);
	} 
	else {
		io.msg(IO_WARN, "FOAM_ExpoAO::calib unknown!");
		return -1;
	}

	io.msg(IO_XNFO, "FOAM_ExpoAO::calib() Complete");

	return 0;
}

void FOAM_ExpoAO::on_message(Connection *const conn, string line) {
	io.msg(IO_DEB2, "FOAM_ExpoAO::on_message(line=%s)", line.c_str());
	
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
												":  mode=zero:            Set current WFS data as reference.");
		}
	}
	else if (cmd == "get") {						// get ...
		string what = popword(line);
		if (what == "calibmodes")					// get calibmodes
			conn->write("ok calibmodes 1 zero");
		else
			parsed = false;
	}
	else if (cmd == "calib") {					// calib <mode>
		string calmode = popword(line);
		conn->write("ok cmd calib");
		ptc->calib = calmode;
		ptc->mode = AO_MODE_CAL;
		{
			pthread::mutexholder h(&mode_mutex);
			mode_cond.signal();						// signal a change to the threads
		}
	}
	else
		parsed = false;
	
	if (!parsed)
		FOAM::on_message(conn, orig);
}

int main(int argc, char *argv[]) {
	FOAM_ExpoAO foam(argc, argv);
	
	if (foam.init())
		exit(-1);
	
	foam.io.msg(IO_INFO, "Running expoao mode");
	foam.listen();
	
	return 0;
}
