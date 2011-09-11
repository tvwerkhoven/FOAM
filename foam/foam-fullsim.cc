/*
 foam-fullsim.cc -- full simulation module
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
SimulWfc *simwfcerr;
SimulCam *simcam;
Shwfs *simwfs;

int FOAM_FullSim::load_modules() {
	io.msg(IO_DEB2, "FOAM_FullSim::load_modules()");
	io.msg(IO_INFO, "This is the full simulation mode, enjoy.");
	
	// Init WFC simulation
	simwfc = new SimulWfc(io, ptc, "simwfc", ptc->listenport, ptc->conffile);
	devices->add((foam::Device *) simwfc);

	// Init WFC error simulation (we use one WFC for generating errors and another for correcting them. This should go perfectly)
	simwfcerr = new SimulWfc(io, ptc, "simwfcerr", ptc->listenport, ptc->conffile);
	devices->add((foam::Device *) simwfcerr);

	// Init camera simulation (using simwfcerr and simwfc)
	simcam = new SimulCam(io, ptc, "simcam", ptc->listenport, ptc->conffile, *simwfc, *simwfcerr);
	devices->add((foam::Device *) simcam);
	
	// Init WFS simulation (using camera)
	simwfs = new Shwfs(io, ptc, "simshwfs", ptc->listenport, ptc->conffile, *simcam);
	devices->add((foam::Device *) simwfs);

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
	open_perf.addlog(1);
	string vec_str;
	
	// Get next frame, simulcam takes care of all simulation
	//!< @bug This call blocks and if the camera is stopped before it returns, it will hang
	Camera::frame_t *frame = simcam->get_next_frame(true);
	open_perf.addlog(2);
	
	// Propagate simulated frame through system (WFS, algorithms, WFC)
	Shwfs::wf_info_t *wf_meas = simwfs->measure(frame);
	open_perf.addlog(3);
	
	vec_str = "";
	for (size_t i=0; i<wf_meas->wfamp->size; i++)
		vec_str += format("%.3g ", gsl_vector_float_get(wf_meas->wfamp, i));
	io.msg(IO_INFO, "FOAM_FullSim::wfs_m: %s", vec_str.c_str());

	simwfs->comp_ctrlcmd(simwfc->getname(), wf_meas->wfamp, simwfc->ctrlparams.err);
	open_perf.addlog(4);
	
	vec_str = "";
	for (size_t i=0; i<simwfc->ctrlparams.err->size; i++)
		vec_str += format("%.3g ", gsl_vector_float_get(simwfc->ctrlparams.err, i));
	io.msg(IO_INFO, "FOAM_FullSim::wfc_rec: %s", vec_str.c_str());

	simwfs->comp_shift(simwfc->getname(), simwfc->ctrlparams.err, wf_meas->wfamp);
	open_perf.addlog(5);

	vec_str = "";
	for (size_t i=0; i<wf_meas->wfamp->size; i++)
		vec_str += format("%.3g ", gsl_vector_float_get(wf_meas->wfamp, i));
	io.msg(IO_INFO, "FOAM_FullSim::wfs_r: %s", vec_str.c_str());

	usleep(0.1 * 1000000);
	return 0;
}

int FOAM_FullSim::open_finish() {
	io.msg(IO_DEB2, "FOAM_FullSim::open_finish()");
	
	simcam->set_mode(Camera::WAITING);
	
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
	closed_perf.addlog(1);
	string vec_str;

	// Get new frame from SimulCamera
	Camera::frame_t *frame = simcam->get_next_frame(true);
	closed_perf.addlog(2);

	// Measure wavefront error with SHWFS
	Shwfs::wf_info_t *wf_meas = simwfs->measure(frame);
	closed_perf.addlog(3);
	
	vec_str = "";
	for (size_t i=0; i<wf_meas->wfamp->size; i++)
		vec_str += format("%.3g ", gsl_vector_float_get(wf_meas->wfamp, i));
	io.msg(IO_INFO, "FOAM_FullSim::wfs_m: %s", vec_str.c_str());
	
	simwfs->comp_ctrlcmd(simwfc->getname(), wf_meas->wfamp, simwfc->ctrlparams.err);
	closed_perf.addlog(4);

	vec_str = "";
	for (size_t i=0; i<simwfc->ctrlparams.err->size; i++)
		vec_str += format("%.3g ", gsl_vector_float_get(simwfc->ctrlparams.err, i));
	io.msg(IO_INFO, "FOAM_FullSim::wfc_rec: %s", vec_str.c_str());
	
	simwfs->comp_shift(simwfc->getname(), simwfc->ctrlparams.err, wf_meas->wfamp);
	closed_perf.addlog(5);
	
	vec_str = "";
	for (size_t i=0; i<wf_meas->wfamp->size; i++)
		vec_str += format("%.3g ", gsl_vector_float_get(wf_meas->wfamp, i));
	io.msg(IO_INFO, "FOAM_FullSim::wfs_r: %s", vec_str.c_str());
	
	simwfc->update_control(simwfc->ctrlparams.err);
	simwfc->actuate(true);
	closed_perf.addlog(6);

	usleep(0.01 * 1000000);
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
		gsl_vector_float *tmpact = gsl_vector_float_calloc(simwfc->get_nact());
		vector <float> actpos;
		actpos.push_back(-1.0);
		actpos.push_back(1.0);

		simwfs->init_infmat(simwfc->getname(), simwfc->get_nact(), actpos);
		
		io.msg(IO_XNFO, "FOAM_FullSim::calib() Start camera...");
		// Disable seeing during calibration
		double old_seeingfac = simcam->get_seeingfac(); simcam->set_seeingfac(0.0);
		bool old_do_wfcerr = simcam->do_simwfcerr; simcam->do_simwfcerr = false;
		simcam->set_mode(Camera::RUNNING);
		
		// Loop over all actuators, actuate according to actpos
		io.msg(IO_XNFO, "FOAM_FullSim::calib() Start calibration loop...");
		for (int i = 0; i < simwfc->get_nact(); i++) {	// Loop over actuators
			for (size_t p = 0; p < actpos.size(); p++) {	// Loop over actuator positions
				if (ptc->mode != AO_MODE_CAL)
					goto influence_break;
				
				// Set actuator to actpos[p], measure, store
				gsl_vector_float_set(tmpact, i, actpos[p]);
				simwfc->update_control(tmpact, gain_t(1,0,0), 0.0);
				simwfc->actuate(true);
				Camera::frame_t *frame = simcam->get_next_frame(true);
				simwfs->build_infmat(simwfc->getname(), frame, i, p);
			}
			
			// Set actuator back to 0
			gsl_vector_float_set_zero(tmpact);
		}
		
		io.msg(IO_XNFO, "FOAM_FullSim::calib() Process data...");
		// Calculate the final influence function
		simwfs->calc_infmat(simwfc->getname());
		
		// Calculate forward matrix
		simwfs->calc_actmat(simwfc->getname());
		
		influence_break:
		// Restore seeing
		simcam->set_mode(Camera::OFF);
		simcam->set_seeingfac(old_seeingfac);
		simcam->do_simwfcerr = old_do_wfcerr;
		gsl_vector_float_free(tmpact);
	} 
	else if (ptc->calib == "zero") {	// Calibrate reference/'flat' wavefront
		gsl_vector_float *tmpact = gsl_vector_float_calloc(simwfc->get_nact());
		// Set wavefront corrector to flat position, start camera
		simwfc->update_control(tmpact, gain_t(0.0, 0.0, 0.0), 0.0);
		simwfc->actuate(true);

		io.msg(IO_XNFO, "FOAM_FullSim::calib() Start camera...");
		// Disable seeing & wfc during calibration
		double old_seeingfac = simcam->get_seeingfac(); simcam->set_seeingfac(0.0);
		bool old_do_wfcerr = simcam->do_simwfcerr; simcam->do_simwfcerr = false;
		bool old_do_simwfc = simcam->do_simwfc; simcam->do_simwfc = false;

		simcam->set_mode(Camera::RUNNING);
		
		io.msg(IO_XNFO, "FOAM_FullSim::calib() Measure reference...");
		// Get next frame (wait for it)
		Camera::frame_t *frame = simcam->get_next_frame(true);
		
		io.msg(IO_XNFO, "FOAM_FullSim::calib() Process data...");
		// Set this frame as reference
		simwfs->set_reference(frame);
		simwfs->store_reference();
		
		// Restore seeing & wfc
		simcam->set_mode(Camera::OFF);
		simcam->set_seeingfac(old_seeingfac);
		simcam->do_simwfc = old_do_simwfc;
		simcam->do_simwfcerr = old_do_wfcerr;
		gsl_vector_float_free(tmpact);
	} 
	else {
		io.msg(IO_WARN, "FOAM_FullSim::calib unknown!");
		return -1;
	}

	io.msg(IO_XNFO, "FOAM_FullSim::calib() Complete");

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
	FOAM_FullSim foam(argc, argv);
	
	if (foam.init())
		exit(-1);
	
	foam.io.msg(IO_INFO, "Running full simulation mode");
	foam.listen();
	
	return 0;
}
