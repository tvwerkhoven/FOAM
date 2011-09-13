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
	openperf_addlog(1);
	string vec_str;
	
	// Get next frame, simulcam takes care of all simulation
	//!< @bug This call blocks and if the camera is stopped before it returns, it will hang
	Camera::frame_t *frame = simcam->get_next_frame(true);
	openperf_addlog(2);
	
	// Propagate simulated frame through system (WFS, algorithms, WFC)
	Shwfs::wf_info_t *wf_meas = simwfs->measure(frame);
	openperf_addlog(3);
	
	vec_str = "";
	for (size_t i=0; i<wf_meas->wfamp->size; i++)
		vec_str += format("%.3g ", gsl_vector_float_get(wf_meas->wfamp, i));
	io.msg(IO_INFO, "FOAM_FullSim::wfs_m: %s", vec_str.c_str());

	simwfs->comp_ctrlcmd(simwfc->getname(), wf_meas->wfamp, simwfc->ctrlparams.err);
	closedperf_addlog(4);

	vec_str = "";
	for (size_t i=0; i<simwfc->ctrlparams.err->size; i++)
		vec_str += format("%.3g ", gsl_vector_float_get(simwfc->ctrlparams.err, i));
	io.msg(IO_INFO, "FOAM_FullSim::wfc_rec: %s", vec_str.c_str());

	simwfs->comp_shift(simwfc->getname(), simwfc->ctrlparams.err, wf_meas->wfamp);
	closedperf_addlog(5);

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
	closedperf_addlog(1);
	string vec_str;
	
	static gsl_vector_float *tmp_wfmeas=NULL;
	if (tmp_wfmeas == NULL)
		tmp_wfmeas = gsl_vector_float_calloc(simwfs->wf.nmodes);

	// Get new frame from SimulCamera
	Camera::frame_t *frame = simcam->get_next_frame(true);
	closedperf_addlog(2);

	// Measure wavefront error with SHWFS
	Shwfs::wf_info_t *wf_meas = simwfs->measure(frame);
	closedperf_addlog(3);
	
	vec_str = "";
	for (size_t i=0; i<wf_meas->wfamp->size; i++)
		vec_str += format("%.3g ", gsl_vector_float_get(wf_meas->wfamp, i));
	io.msg(IO_INFO, "FOAM_FullSim::wfs_m: %s", vec_str.c_str());
	
	simwfs->comp_ctrlcmd(simwfc->getname(), wf_meas->wfamp, simwfc->ctrlparams.err);
	closedperf_addlog(4);

	vec_str = "";
	for (size_t i=0; i<simwfc->ctrlparams.err->size; i++)
		vec_str += format("%.3g ", gsl_vector_float_get(simwfc->ctrlparams.err, i));
	io.msg(IO_INFO, "FOAM_FullSim::wfc_rec: %s", vec_str.c_str());
	
	simwfs->comp_shift(simwfc->getname(), simwfc->ctrlparams.err, tmp_wfmeas);
	closedperf_addlog(5);
	
	vec_str = "";
	for (size_t i=0; i<tmp_wfmeas->size; i++)
		vec_str += format("%.3g ", gsl_vector_float_get(tmp_wfmeas, i));
	io.msg(IO_INFO, "FOAM_FullSim::wfs_r: %s", vec_str.c_str());
	
	simwfc->update_control(simwfc->ctrlparams.err);
	simwfc->actuate(true);
	closedperf_addlog(6);

	usleep(0.01 * 1000000);
	return 0;
}

int FOAM_FullSim::closed_finish() {
	io.msg(IO_DEB2, "FOAM_FullSim::closed_finish()");
	
	simcam->set_mode(Camera::WAITING);

	return 0;
}

// MISC ROUTINES //
/*****************/

int FOAM_FullSim::calib() {
	io.msg(IO_DEB2, "FOAM_FullSim::calib()=%s", ptc->calib.c_str());

	if (ptc->calib == "influence") {		// Calibrate influence function
		double sval_cutoff = popdouble(ptc->calib_opt);
		if (sval_cutoff <= 0.0 || sval_cutoff > 1.0)
			sval_cutoff = 0.7;
		io.msg(IO_INFO, "FOAM_FullSim::calib() influence calibration, sval=%g", sval_cutoff);

		// Init actuation vector & positions, camera, 
		vector <float> actpos;
		actpos.push_back(-1.0);
		actpos.push_back(1.0);
		
		// Disable seeing during calibration
		double old_seeingfac = simcam->get_seeingfac(); simcam->set_seeingfac(0.0);
		bool old_do_wfcerr = simcam->do_simwfcerr; simcam->do_simwfcerr = false;
		
		// Calibrate for influence function now
		simwfs->calib_influence(simwfc, simcam, actpos, sval_cutoff);
		
		// Reset seeing settings
		simcam->set_seeingfac(old_seeingfac);
		simcam->do_simwfcerr = old_do_wfcerr;
	} 
	else if (ptc->calib == "zero") {	// Calibrate reference/'flat' wavefront
		io.msg(IO_INFO, "FOAM_FullSim::calib() Zero calibration");
		// Disable seeing & wfc during calibration
		double old_seeingfac = simcam->get_seeingfac(); simcam->set_seeingfac(0.0);
		bool old_do_wfcerr = simcam->do_simwfcerr; simcam->do_simwfcerr = false;
		bool old_do_simwfc = simcam->do_simwfc; simcam->do_simwfc = false;

		simwfs->calib_zero(simwfc, simcam);

		// Restore seeing & wfc
		simcam->set_seeingfac(old_seeingfac);
		simcam->do_simwfc = old_do_simwfc;
		simcam->do_simwfcerr = old_do_wfcerr;
	} 
	else if (ptc->calib == "offsetvec") {	// Add offset vector to correction 
		double xoff = popdouble(ptc->calib_opt);
		double yoff = popdouble(ptc->calib_opt);
		io.msg(IO_INFO, "FOAM_FullSim::calib() apply offset vector (%g, %g)", xoff, yoff);
		
		if (simwfs->calib_offset(xoff, yoff))
			io.msg(IO_ERR, "FOAM_FullSim::calib() offset vector could not be applied!");
	}
	else if (ptc->calib == "svd") {	// (Re-)calculate SVD given the influence matrix
		double sval_cutoff = popdouble(ptc->calib_opt);
		if (sval_cutoff <= 0.0 || sval_cutoff > 1.0)
			sval_cutoff = 0.7;
		io.msg(IO_INFO, "FOAM_FullSim::calib() re-calc SVD, sval=%g", sval_cutoff);

		simwfs->calc_actmat(simwfc->getname(), sval_cutoff);
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
												":calib <mode> [opt]:     Calibrate AO system.");
		}
		else if (topic == "calib") {			// help calib
			conn->write(\
												":calib <mode> [opt]:     Calibrate AO system.\n"
												":  mode=zero:            Set current WFS data as reference.\n"
												":  mode=influence [singv]:\n"
												":                        Measure wfs-wfc influence, cutoff at singv\n"
												":  mode=offsetvec [x] [y]:\n"
												":                        Add offset vector to correction.\n"
												":  mode=svd [singv]:     Recalculate SVD wfs-wfc influence, cutoff at singv.");
		}
	}
	else if (cmd == "get") {						// get ...
		string what = popword(line);
		if (what == "calibmodes")					// get calibmodes
			conn->write("ok calibmodes 4 zero influence offsetvec svd");
		else
			parsed = false;
	}
	else if (cmd == "calib") {					// calib <mode> [opt]
		string calmode = popword(line);
		string calopt = popword(line);
		conn->write("ok cmd calib");
		ptc->calib = calmode;
		ptc->calib_opt = calopt;
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
