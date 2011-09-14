/*
 foam-expoao.cc -- FOAM expoao build
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
#include "andor.h"
#include "wfs.h"
#include "shwfs.h"
#include "wfc.h"
#include "alpaodm.h"

#include "foam-expoao.h"

using namespace std;

// Global device list for easier access
AndorCam *ixoncam;
Shwfs *ixonwfs;
AlpaoDM *alpao_dm97;

int FOAM_ExpoAO::load_modules() {
	io.msg(IO_DEB2, "FOAM_ExpoAO::load_modules()");
	io.msg(IO_INFO, "This is the expoao build, enjoy.");
	
	try {
		// Init Alpao DM
		io.msg(IO_INFO, "Init Alpao DM97-15...");
		alpao_dm97 = new AlpaoDM(io, ptc, "alpao_dm97", ptc->listenport, ptc->conffile);
		devices->add((foam::Device *) alpao_dm97);

		// Init Andor camera
		io.msg(IO_INFO, "Init Andor Ixon Camera...");
		ixoncam = new AndorCam(io, ptc, "ixoncam", ptc->listenport, ptc->conffile);
		devices->add((foam::Device *) ixoncam);
		
		io.msg(IO_INFO, "Andor camera initialized, printing capabilities");
		ixoncam->print_andor_caps();
		
		// Init WFS (using ixoncam)
		ixonwfs = new Shwfs(io, ptc, "ixonwfs", ptc->listenport, ptc->conffile, *ixoncam);
		devices->add((foam::Device *) ixonwfs);
	} catch (std::runtime_error &e) {
		io.msg(IO_ERR, "FOAM_ExpoAO::load_modules: %s", e.what());
		return -1;
	}	catch (...) {
		io.msg(IO_ERR, "FOAM_ExpoAO::load_modules: unknown error!");
		throw;
	}
	
	return 0;
}

// OPEN LOOP ROUTINES //
/*********************/

int FOAM_ExpoAO::open_init() {
	io.msg(IO_DEB2, "FOAM_ExpoAO::open_init()");
	
	// Check subimage bounds
	if (ixonwfs->check_subimgs(ixoncam->get_res()))
		return -1;

	ixoncam->set_proc_frames(true);
	ixoncam->set_mode(Camera::RUNNING);
	
	return 0;
}

int FOAM_ExpoAO::open_loop() {
	io.msg(IO_DEB2, "FOAM_ExpoAO::open_loop()");
	openperf_addlog(1);
	string vec_str;
	
	// Get next frame from ixoncam
	Camera::frame_t *frame = ixoncam->get_next_frame(true);
	openperf_addlog(2);
	
	// Analyze frame with shack-hartmann routines
	Shwfs::wf_info_t *wf_meas = ixonwfs->measure(frame);
	openperf_addlog(3);
	
	// Print analysis
	vec_str = "";
	for (size_t i=0; i < wf_meas->wfamp->size; i++)
		vec_str += format("%.3g ", gsl_vector_float_get(wf_meas->wfamp, i));
	io.msg(IO_INFO, "FOAM_ExpoAO::wfs_m: %s", vec_str.c_str());
	
	ixonwfs->comp_ctrlcmd(alpao_dm97->getname(), wf_meas->wfamp, alpao_dm97->ctrlparams.err);
	openperf_addlog(4);
	
	vec_str = "";
	for (size_t i=0; i<alpao_dm97->ctrlparams.err->size; i++)
		vec_str += format("%.3g ", gsl_vector_float_get(alpao_dm97->ctrlparams.err, i));
	io.msg(IO_INFO, "FOAM_ExpoAO::wfc_rec: %s", vec_str.c_str());
	
	ixonwfs->comp_shift(alpao_dm97->getname(), alpao_dm97->ctrlparams.err, wf_meas->wfamp);
	openperf_addlog(5);

	vec_str = "";
	for (size_t i=0; i<wf_meas->wfamp->size; i++)
		vec_str += format("%.3g ", gsl_vector_float_get(wf_meas->wfamp, i));
	io.msg(IO_INFO, "FOAM_ExpoAO::wfs_r: %s", vec_str.c_str());

	// Wait a little so we can follow the program
	usleep(1.0 * 1000000);
	return 0;
}

int FOAM_ExpoAO::open_finish() {
	io.msg(IO_DEB2, "FOAM_ExpoAO::open_finish()");
	
	ixoncam->set_mode(Camera::WAITING);
	
	return 0;
}

// CLOSED LOOP ROUTINES //
/************************/

int FOAM_ExpoAO::closed_init() {
	io.msg(IO_DEB2, "FOAM_ExpoAO::closed_init()");
	
	// Check subimage bounds
	if (ixonwfs->check_subimgs(ixoncam->get_res()))
		return -1;

	ixoncam->set_proc_frames(false);
	ixoncam->set_mode(Camera::RUNNING);
	
	return 0;
}

int FOAM_ExpoAO::closed_loop() {
	io.msg(IO_DEB2, "FOAM_ExpoAO::closed_loop()");
	closedperf_addlog(1);
	string vec_str;

	// Get next frame from ixoncam
	Camera::frame_t *frame = ixoncam->get_next_frame(true);
	closedperf_addlog(2);

	// Analyze frame with shack-hartmann routines
	Shwfs::wf_info_t *wf_meas = ixonwfs->measure(frame);
	closedperf_addlog(3);
	
	// Calculate control command for DM
	ixonwfs->comp_ctrlcmd(alpao_dm97->getname(), wf_meas->wfamp, alpao_dm97->ctrlparams.err);
	closedperf_addlog(4);
	
	// Apply control to DM to correct shifts
	alpao_dm97->update_control(alpao_dm97->ctrlparams.err);
	alpao_dm97->actuate();
	closedperf_addlog(5);

	// Don't wait here, closed loop should be fast
	return 0;
}

int FOAM_ExpoAO::closed_finish() {
	io.msg(IO_DEB2, "FOAM_ExpoAO::closed_finish()");
	
	ixoncam->set_mode(Camera::WAITING);

	return 0;
}

// MISC ROUTINES //
/*****************/

int FOAM_ExpoAO::calib() {
	io.msg(IO_DEB2, "FOAM_ExpoAO::calib()=%s", ptc->calib.c_str());

	if (ptc->calib == "zero") {							// Calibrate reference/'flat' wavefront
		io.msg(IO_INFO, "FOAM_ExpoAO::calib() Zero calibration");
		ixonwfs->calib_zero(alpao_dm97, ixoncam);
		
		protocol->broadcast(format("ok calib zero :%s", 
															 ixonwfs->get_refvec_str().c_str() ));
	} 
	else if (ptc->calib == "influence") {		// Calibrate influence function
		// calib influence [singval cutoff] -- 
		// singval < 0: drop these modes
		// singval > 1: use this amount of modes
		// else: use this amount of singular value
		double sval_cutoff = popdouble(ptc->calib_opt);
		if (sval_cutoff == 0.0) sval_cutoff = 0.7;
		io.msg(IO_INFO, "FOAM_ExpoAO::calib() influence calibration, sval=%g", sval_cutoff);

		// Init actuation vector & positions, camera, 
		vector <float> actpos;
		actpos.push_back(-0.08);
		actpos.push_back(0.08);
		
		// Calibrate for influence function now
		ixonwfs->calib_influence(alpao_dm97, ixoncam, actpos, sval_cutoff);

		// Broadcast results to clients
		protocol->broadcast(format("ok calib svd singvals :%s", 
															 ixonwfs->get_singval_str(alpao_dm97->getname()).c_str() ));
		protocol->broadcast(format("ok calib svd condition :%g", 
															 ixonwfs->get_svd_cond(alpao_dm97->getname()) ));
		protocol->broadcast(format("ok calib svd usage :%g %d", 
															 ixonwfs->get_svd_singuse(alpao_dm97->getname()),
															 ixonwfs->get_svd_modeuse(alpao_dm97->getname())
															 ));
	} 
	else if (ptc->calib == "offsetvec") {	// Add offset vector to correction 
		double xoff = popdouble(ptc->calib_opt);
		double yoff = popdouble(ptc->calib_opt);
		io.msg(IO_INFO, "FOAM_ExpoAO::calib() apply offset vector (%g, %g)", xoff, yoff);
		
		if (ixonwfs->calib_offset(xoff, yoff))
			io.msg(IO_ERR, "FOAM_ExpoAO::calib() offset vector could not be applied!");
	}
	else if (ptc->calib == "svd") {					// (Re-)calculate SVD given the influence matrix
		// calib svd [singval cutoff] -- 
		// singval < 0: drop these modes
		// singval > 1: use this amount of modes
		// else: use this amount of singular value
		double sval_cutoff = popdouble(ptc->calib_opt);
		if (sval_cutoff == 0.0) sval_cutoff = 0.7;
		io.msg(IO_INFO, "FOAM_ExpoAO::calib() re-calc SVD, sval=%g", sval_cutoff);
		
		ixonwfs->calc_actmat(alpao_dm97->getname(), sval_cutoff);
		
		// Broadcast results to clients
		protocol->broadcast(format("ok calib svd singvals :%s", 
															 ixonwfs->get_singval_str(alpao_dm97->getname()).c_str() ));
		protocol->broadcast(format("ok calib svd condition :%g", 
															 ixonwfs->get_svd_cond(alpao_dm97->getname()) ));
		protocol->broadcast(format("ok calib svd usage :%g %d", 
															 ixonwfs->get_svd_singuse(alpao_dm97->getname()),
															 ixonwfs->get_svd_modeuse(alpao_dm97->getname())
															 ));
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
												":==== expoao help =========================\n"
												":get calibmodes:         List calibration modes\n"
												":calib <mode> [opts]:    Calibrate AO system.");
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
			conn->write("ok calibmodes 3 zero influence offsetvec  svd");
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
