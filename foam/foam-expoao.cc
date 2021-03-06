/*
 foam-expoao.cc -- FOAM expoao build
 Copyright (C) 2010-2012 Tim van Werkhoven <werkhoven@strw.leidenuniv.nl>
 
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
#include "telescope.h"
#include "wht.h"

#include "foam-expoao.h"

using namespace std;

// Global device list for easier access
AndorCam *ixoncam;
Shwfs *ixonwfs;
AlpaoDM *alpao_dm97;
WHT *wht_track;

FOAM_ExpoAO::FOAM_ExpoAO(int argc, char *argv[]): FOAM(argc, argv) {
	io.msg(IO_DEB2, "FOAM_ExpoAO::FOAM_ExpoAO()");
	// Register calibration modes
	calib_modes["zero"] = calib_mode("zero", "Set current WFS data as reference", "", false);
	calib_modes["influence"] = calib_mode("influence", "Measure wfs-wfc influence, cutoff at singv", "[singv]", false);
	calib_modes["offsetvec"] = calib_mode("offsetvec", "Add offset vector to correction", "[x] [y]", false);
	calib_modes["svd"] = calib_mode("svd", "Recalculate SVD wfs-wfc influence, cutoff at singv.", "[singv]", true);
}

int FOAM_ExpoAO::load_modules() {
	io.msg(IO_DEB2, "FOAM_ExpoAO::load_modules()");
	io.msg(IO_INFO, "This is the expoao build, enjoy.");
	
	//! @todo Try-catch clause does not function properly, errors don't propagate outward io.msg() problem?
	try {
		// Init WHT telescope interface
		wht_track = new WHT(io, ptc, "wht", ptc->listenport, ptc->conffile);
		devices->add((foam::Device *) wht_track);
		
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
		io.msg(IO_ERR | IO_FATAL, "FOAM_ExpoAO::load_modules: %s", e.what());
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

	ixoncam->set_proc_frames(false);
	ixoncam->set_mode(Camera::RUNNING);
	
	return 0;
}

int FOAM_ExpoAO::open_loop() {
	io.msg(IO_DEB2, "FOAM_ExpoAO::open_loop()");
	openperf_addlog("expoao loop");
	string vec_str;
	
	// Get next frame from ixoncam
	Camera::frame_t *frame = ixoncam->get_next_frame(true);
	openperf_addlog("cam->get_next_frame");
	
	// Analyze frame with shack-hartmann routines
	Shwfs::wf_info_t *wf_meas = ixonwfs->measure(frame);
	openperf_addlog("wfs->measure");
	
	// Print analysis
	vec_str = "";
	for (size_t i=0; i < wf_meas->wfamp->size; i++)
		vec_str += format("%.3g ", gsl_vector_float_get(wf_meas->wfamp, i));
	io.msg(IO_DEB1, "FOAM_ExpoAO::wfs_m: %s", vec_str.c_str());
	
	ixonwfs->comp_ctrlcmd(alpao_dm97->getname(), wf_meas->wfamp, alpao_dm97->ctrlparams.err);
	openperf_addlog("wfs->comp_ctrlcmd");
	
	vec_str = "";
	for (size_t i=0; i<alpao_dm97->ctrlparams.err->size; i++)
		vec_str += format("%.3g ", gsl_vector_float_get(alpao_dm97->ctrlparams.err, i));
	io.msg(IO_DEB1, "FOAM_ExpoAO::wfc_rec: %s", vec_str.c_str());
	
	ixonwfs->comp_shift(alpao_dm97->getname(), alpao_dm97->ctrlparams.err, wf_meas->wf_full);
	openperf_addlog("wfs->comp_shift");

	float ttx=0, tty=0;
	ixonwfs->comp_tt(wf_meas->wfamp, &ttx, &tty);
	wht_track->set_track_offset(ttx, tty);
	openperf_addlog("wfs->comp_tt");

	vec_str = "";
	for (size_t i=0; i<wf_meas->wfamp->size; i++)
		vec_str += format("%.3g ", gsl_vector_float_get(wf_meas->wfamp, i));
	io.msg(IO_DEB1, "FOAM_ExpoAO::wfs_r: %s", vec_str.c_str());

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
	closedperf_addlog("expoao loop");
	string vec_str;

	// Get next frame from ixoncam
	Camera::frame_t *frame = ixoncam->get_next_frame(true);
	closedperf_addlog("cam->get_next_frame()");

	// Analyze frame with shack-hartmann routines
	Shwfs::wf_info_t *wf_meas = ixonwfs->measure(frame);
	closedperf_addlog("wfs->measure()");
	
	// Calculate control command for DM
	ixonwfs->comp_ctrlcmd(alpao_dm97->getname(), wf_meas->wfamp, alpao_dm97->ctrlparams.err);
	closedperf_addlog("wfc->comp_ctrlcmd()");
	
	// Apply control to DM to correct shifts
	alpao_dm97->update_control(alpao_dm97->ctrlparams.err);
	alpao_dm97->actuate();
	closedperf_addlog("wfc->update_control()");
	
	// Use control vector to compute total shifts that we are correcting
	ixonwfs->comp_shift(alpao_dm97->getname(), alpao_dm97->ctrlparams.target, wf_meas->wf_full);
	openperf_addlog("wfs->comp_shift");
	
	// Compute tip-tilt signal from total shift vector, track telescope
	float ttx=0, tty=0;
	ixonwfs->comp_tt(wf_meas->wf_full, &ttx, &tty);
	wht_track->set_track_offset(ttx, tty);
	openperf_addlog("wfs->comp_tt");

	return 0;
}

int FOAM_ExpoAO::closed_finish() {
	io.msg(IO_DEB2, "FOAM_ExpoAO::closed_finish()");
	
	ixoncam->set_mode(Camera::WAITING);

	return 0;
}

// MISC ROUTINES //
/*****************/

int FOAM_ExpoAO::calib(const string &calib_mode, const string &calib_opts) {
	io.msg(IO_DEB2, "FOAM_ExpoAO::calib()=%s", calib_mode.c_str());
	int calret = 0;
	string this_opts = calib_opts;
	
	if (calib_mode == "zero") {							// Calibrate reference/'flat' wavefront
		io.msg(IO_INFO, "FOAM_ExpoAO::calib() Zero calibration");
		calret = ixonwfs->calib_zero(alpao_dm97, ixoncam);
		
		// If we failed, return.
		if (calret)
			return -1;

		protocol->broadcast(format("ok calib zero :%s", 
															 ixonwfs->get_refvec_str().c_str() ));
	} 
	else if (calib_mode == "influence") {		// Calibrate influence function
		// calib influence [actuation amplitude] [singval cutoff] -- 
		// actuation amplitude: how far should we move the actuators?
		// singval < 0: drop these modes
		// singval > 1: use this amount of modes
		// else: use this amount of singular value
		double act_amp = popdouble(this_opts);
		if (act_amp == 0.0) act_amp = 0.08;
		double sval_cutoff = popdouble(this_opts);
		if (sval_cutoff == 0.0) sval_cutoff = 0.7;
		io.msg(IO_INFO, "FOAM_ExpoAO::calib() influence calibration, amp=%g, sval=%g", act_amp, sval_cutoff);

		// Init actuation vector & positions, camera, 
		vector <float> actpos;
		actpos.push_back(-act_amp);
		actpos.push_back(act_amp);
		
		// Calibrate for influence function now
		calret = ixonwfs->calib_influence(alpao_dm97, ixoncam, actpos, sval_cutoff);

		// If we failed, return.
		if (calret)
			return -1;

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
	else if (calib_mode == "offsetvec") {	// Add offset vector to correction 
		double xoff = popdouble(this_opts);
		double yoff = popdouble(this_opts);
		
		if (ixonwfs->calib_offset(xoff, yoff)) {
			io.msg(IO_ERR, "FOAM_ExpoAO::calib() offset vector could not be applied!");
			return -1;
		} else {
			io.msg(IO_INFO, "FOAM_ExpoAO::calib() apply offset vector (%g, %g)", xoff, yoff);
		}
	}
	else if (calib_mode == "svd") {					// (Re-)calculate SVD given the influence matrix
		// calib svd [singval cutoff] -- 
		// singval < 0: drop these modes
		// singval > 1: use this amount of modes
		// else: use this amount of singular value
        //! @todo Document this
        //! @bug When singval == # actuators, this code crashes (i.e. 37 virtual actuators and asking singval=37 crashes)
		double sval_cutoff = popdouble(this_opts);
		if (sval_cutoff == 0.0) sval_cutoff = 0.7;
		io.msg(IO_INFO, "FOAM_ExpoAO::calib() re-calc SVD, sval=%g", sval_cutoff);
		
		calret = ixonwfs->update_actmat(alpao_dm97->getname(), sval_cutoff);
		
		// If we failed, return.
		if (calret)
			return -1;
		
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
	FOAM::on_message(conn, line);
}

int main(int argc, char *argv[]) {
	FOAM_ExpoAO foam(argc, argv);
	
	if (foam.init())
		exit(-1);
	
	foam.io.msg(IO_INFO, "Running expoao mode");
	foam.listen();
	
	return 0;
}
