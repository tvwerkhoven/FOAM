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
	string act_patt = "";
	for (size_t i=0; i<simwfc->wfc_amp->size; i++)
		act_patt += format("%.3g ", gsl_vector_float_get(simwfc->wfc_amp, i));
	io.msg(IO_INFO, "FOAM_FullSim::wfc_act: %s", act_patt.c_str());

	for (size_t i=0; i<simwfc->wfc_amp->size; i++)
		gsl_vector_float_set(simwfc->wfc_amp, i, 0.0);

	Camera::frame_t *frame = simcam->get_next_frame(true);

	Shwfs::wf_info_t *wf_meas = simwfs->measure(frame);
	act_patt = "";
	for (size_t i=0; i<wf_meas->wfamp->size; i++)
		act_patt += format("%.3g ", gsl_vector_float_get(wf_meas->wfamp, i));
	io.msg(IO_INFO, "FOAM_FullSim::wfs_m: %s", act_patt.c_str());

	simwfs->comp_ctrlcmd(simwfc->getname(), wf_meas->wfamp, simwfc->wfc_amp);
	
	act_patt = "";
	for (size_t i=0; i<simwfc->wfc_amp->size; i++)
		act_patt += format("%.3g ", gsl_vector_float_get(simwfc->wfc_amp, i));
	io.msg(IO_INFO, "FOAM_FullSim::wfc_rec: %s", act_patt.c_str());

	simwfs->comp_shift(simwfc->getname(), simwfc->wfc_amp, wf_meas->wfamp);

	act_patt = "";
	for (size_t i=0; i<wf_meas->wfamp->size; i++)
		act_patt += format("%.3g ", gsl_vector_float_get(wf_meas->wfamp, i));
	io.msg(IO_INFO, "FOAM_FullSim::wfs_r: %s", act_patt.c_str());
	
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
	
	// Measure wavefront error with SHWFS
	Camera::frame_t *frame = simcam->get_next_frame(true);
	Shwfs::wf_info_t *wf_meas = simwfs->measure(frame);
	
	// Calculte mirror 
	simwfs->comp_ctrlcmd(simwfc->getname(), wf_meas->wfamp, simwfc->wfc_amp);
	
	simwfc->actuate(simwfc->wfc_amp, gain_t(1,0,0), true);

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
		vector <float> actpos;
		actpos.push_back(-1.0);
		//actpos.push_back(0.3);
		actpos.push_back(1.0);

		simwfs->init_infmat(simwfc->getname(), simwfc->nact, actpos);
		
		io.msg(IO_XNFO, "FOAM_FullSim::calib() Start camera...");
		// Disable seeing during calibration
		double old_seeingfac = simcam->seeingfac; simcam->seeingfac = 0.0;
		simcam->set_mode(Camera::RUNNING);
		
		// Loop over all actuators, actuate according to actpos
		io.msg(IO_XNFO, "FOAM_FullSim::calib() Start calibration loop...");
		for (int i = 0; i < simwfc->nact; i++) {
			for (size_t p = 0; p < actpos.size(); p++) {
				if (ptc->mode != AO_MODE_CAL)
					goto influence_break;
				// Set actuator to actpos[p], measure, store
				gsl_vector_float_set(tmpact, i, actpos[p]);
				simwfc->actuate(tmpact, gain_t(1.0, 0.0, 0.0), true);
				Camera::frame_t *frame = simcam->get_next_frame(true);
				simwfs->build_infmat(simwfc->getname(), frame, i, p);
			}
			
			// Set actuator back to 0
			gsl_vector_float_set(tmpact, i, 0.0);
		}
		
		io.msg(IO_XNFO, "FOAM_FullSim::calib() Process data...");
		// Calculate the final influence function
		simwfs->calc_infmat(simwfc->getname());
		
		// Calculate forward matrix
		simwfs->calc_actmat(simwfc->getname());
		
		influence_break:
		// Restore seeing
		simcam->seeingfac = old_seeingfac;
		//simcam->do_simmla = old_do_simmla;
		simcam->set_mode(Camera::OFF);
		gsl_vector_float_free(tmpact);
	} 
	else if (ptc->calib == "zero") {	// Calibrate reference/'flat' wavefront
		// Set wavefront corrector to flat position, start camera
		simwfc->actuate(NULL, true);

		io.msg(IO_XNFO, "FOAM_FullSim::calib() Start camera...");
		// Disable seeing & wfc during calibration
		double old_seeingfac = simcam->seeingfac; simcam->seeingfac = 0.0;
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
		simcam->seeingfac = old_seeingfac;
		simcam->do_simwfc = old_do_simwfc;
		simcam->set_mode(Camera::OFF);
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
