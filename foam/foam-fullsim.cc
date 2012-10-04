/*
 foam-fullsim.cc -- full simulation module
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
#include "simulcam.h"
#include "wfs.h"
#include "shwfs.h"
#include "wfc.h"
#include "simulwfc.h"
#include "telescope.h"

#include "foam-fullsim.h"

using namespace std;

// Global device list for easier access
SimulWfc *simwfc;
SimulWfc *simwfcerr;
SimulCam *simcam;
Shwfs *simwfs;
Telescope *simtel;

FOAM_FullSim::FOAM_FullSim(int argc, char *argv[]): FOAM(argc, argv) {
	io.msg(IO_DEB2, "FOAM_FullSim::FOAM_FullSim()");

	// Register calibration modes
	calib_modes["zero"] = calib_mode("zero", "Set current WFS data as reference", "", false);
	calib_modes["influence"] = calib_mode("influence", "Measure wfs-wfc influence, cutoff at singv", "[singv]", false);
	calib_modes["offsetvec"] = calib_mode("offsetvec", "Add offset vector to correction", "[x] [y]", false);
	calib_modes["svd"] = calib_mode("svd", "Recalculate SVD wfs-wfc influence, cutoff at singv.", "[singv]", true);
}

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

	// Init Telescope simulation
	simtel = new Telescope(io, ptc, "simtel", "simtel_t", ptc->listenport, ptc->conffile);
	devices->add((foam::Device *) simtel);

	return 0;
}

// OPEN LOOP ROUTINES //
/*********************/

int FOAM_FullSim::open_init() {
	io.msg(IO_DEB2, "FOAM_FullSim::open_init()");
	
	// Check subimage bounds
	if (simwfs->check_subimgs(simcam->get_res()))
		return -1;
	
	// Start camera
	simcam->set_mode(Camera::RUNNING);
	
	return 0;
}

int FOAM_FullSim::open_loop() {
	io.msg(IO_DEB2, "FOAM_FullSim::open_loop()");
	openperf_addlog("init fullsim");
	string vec_str;
	
	// Get next frame, simulcam takes care of all simulation
	//!< @bug This call blocks and if the camera is stopped before it returns, it will hang
	Camera::frame_t *frame = simcam->get_next_frame(true);
	openperf_addlog("cam->get_next_frame");
	
	// Propagate simulated frame through system (WFS, algorithms, WFC)
	Shwfs::wf_info_t *wf_meas = simwfs->measure(frame);
	openperf_addlog("wfs->measure");
	
	vec_str = "";
	for (size_t i=0; i<wf_meas->wfamp->size; i++)
		vec_str += format("%.3g ", gsl_vector_float_get(wf_meas->wfamp, i));
	io.msg(IO_XNFO, "FOAM_FullSim::wfs_m: %s", vec_str.c_str());

	simwfs->comp_ctrlcmd(simwfc->getname(), wf_meas->wfamp, simwfc->ctrlparams.err);
	closedperf_addlog("wfs->comp_ctrlcmd");

	vec_str = "";
	for (size_t i=0; i<simwfc->ctrlparams.err->size; i++)
		vec_str += format("%.3g ", gsl_vector_float_get(simwfc->ctrlparams.err, i));
	io.msg(IO_XNFO, "FOAM_FullSim::wfc_rec: %s", vec_str.c_str());

	simwfs->comp_shift(simwfc->getname(), simwfc->ctrlparams.err, wf_meas->wfamp);
	closedperf_addlog("wfs->comp_shift");
	
	vec_str = "";
	for (size_t i=0; i<wf_meas->wfamp->size; i++)
		vec_str += format("%.3g ", gsl_vector_float_get(wf_meas->wfamp, i));
	io.msg(IO_XNFO, "FOAM_FullSim::wfs_r: %s", vec_str.c_str());
	
	// Compute tip-tilt signal from total shift vector, track telescope
	float ttx=0, tty=0;
	simwfs->comp_tt(wf_meas->wfamp, &ttx, &tty);
	simtel->set_track_offset(ttx, tty);
	openperf_addlog("wfs->comp_tt");

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
	
	// Check subimage bounds
	if (simwfs->check_subimgs(simcam->get_res()))
		return -1;

	simcam->set_mode(Camera::RUNNING);
	
	return 0;
}

int FOAM_FullSim::closed_loop() {
	io.msg(IO_DEB2, "FOAM_FullSim::closed_loop()");
	closedperf_addlog("init fullsim");
	string vec_str;
	
	// Get new frame from SimulCamera
	Camera::frame_t *frame = simcam->get_next_frame(true);
	closedperf_addlog("cam->get_next_frame");

	// Measure wavefront error with SHWFS
	Shwfs::wf_info_t *wf_meas = simwfs->measure(frame);
	closedperf_addlog("wfs->measure");
	
	vec_str = "";
	for (size_t i=0; i<wf_meas->wfamp->size; i++)
		vec_str += format("%.3g ", gsl_vector_float_get(wf_meas->wfamp, i));
	io.msg(IO_INFO, "FOAM_FullSim::wfs_m: %s", vec_str.c_str());
	
	simwfs->comp_ctrlcmd(simwfc->getname(), wf_meas->wfamp, simwfc->ctrlparams.err);
	closedperf_addlog("wfs->comp_ctrlcmd");

	vec_str = "";
	for (size_t i=0; i<simwfc->ctrlparams.err->size; i++)
		vec_str += format("%.3g ", gsl_vector_float_get(simwfc->ctrlparams.err, i));
	io.msg(IO_INFO, "FOAM_FullSim::wfc_rec: %s", vec_str.c_str());
	
	simwfs->comp_shift(simwfc->getname(), simwfc->ctrlparams.err, wf_meas->wf_full);
	closedperf_addlog("wfs->comp_shift");
	
	vec_str = "";
	for (size_t i=0; i<wf_meas->wf_full->size; i++)
		vec_str += format("%.3g ", gsl_vector_float_get(wf_meas->wf_full, i));
	io.msg(IO_INFO, "FOAM_FullSim::wfs_r: %s", vec_str.c_str());
	
	simwfc->update_control(simwfc->ctrlparams.err);
	simwfc->actuate(true);
	closedperf_addlog("wfc->update_control");
	
	// Compute tip-tilt signal from total shift vector, track telescope
	float ttx=0, tty=0;
	simwfs->comp_tt(wf_meas->wf_full, &ttx, &tty);
	simtel->set_track_offset(ttx, tty);
	openperf_addlog("wfs->comp_tt");

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

int FOAM_FullSim::calib(const string &calib_mode, const string &calib_opts) {
	io.msg(IO_DEB2, "FOAM_FullSim::calib()=%s", calib_mode.c_str());
	int calret = 0;
	string this_opts = calib_opts;
	
	if (calib_mode == "influence") {		// Calibrate influence function
		// calib influence [actuation amplitude] [singval cutoff] -- 
		// actuation amplitude: how far should we move the actuators?
		// singval < 0: drop these modes
		// singval > 1: use this amount of modes
		// else: use this amount of singular value
		double act_amp = popdouble(ptc->calib_opt);
		if (act_amp == 0.0) act_amp = 1.0;
		double sval_cutoff = popdouble(ptc->calib_opt);
		if (sval_cutoff == 0.0) sval_cutoff = 0.7;
		io.msg(IO_INFO, "FOAM_FullSim::calib() influence calibration, amp=%g, sval=%g", act_amp, sval_cutoff);
		
		// Init actuation vector & positions, camera, 
		vector <float> actpos;
		actpos.push_back(-act_amp);
		actpos.push_back(act_amp);

		// Disable seeing during calibration
		double old_seeingfac = simcam->get_seeingfac(); simcam->set_seeingfac(0.0);
		bool old_do_wfcerr = simcam->do_simwfcerr; simcam->do_simwfcerr = false;
		
		// Calibrate for influence function now
		calret = simwfs->calib_influence(simwfc, simcam, actpos, sval_cutoff);
		
		// Reset seeing settings
		simcam->set_seeingfac(old_seeingfac);
		simcam->do_simwfcerr = old_do_wfcerr;
		
		// If we failed, return.
		if (calret)
			return -1;
		
		// Broadcast results to clients
		protocol->broadcast(format("ok calib svd singvals :%s", 
															 simwfs->get_singval_str(simwfc->getname()).c_str() ));
		protocol->broadcast(format("ok calib svd condition :%g", simwfs->get_svd_cond(simwfc->getname())));
		protocol->broadcast(format("ok calib svd usage :%g %d", 
															 simwfs->get_svd_singuse(simwfc->getname()),
															 simwfs->get_svd_modeuse(simwfc->getname())
															 ));
	} 
	else if (ptc->calib == "zero") {	// Calibrate reference/'flat' wavefront
		io.msg(IO_INFO, "FOAM_FullSim::calib() Zero calibration");
		// Disable seeing & wfc during calibration
		double old_seeingfac = simcam->get_seeingfac(); simcam->set_seeingfac(0.0);
		bool old_do_wfcerr = simcam->do_simwfcerr; simcam->do_simwfcerr = false;
		bool old_do_simwfc = simcam->do_simwfc; simcam->do_simwfc = false;

		calret = simwfs->calib_zero(simwfc, simcam);
		
		// If we failed, return.
		if (calret)
			return -1;

		// Restore seeing & wfc
		simcam->set_seeingfac(old_seeingfac);
		simcam->do_simwfc = old_do_simwfc;
		simcam->do_simwfcerr = old_do_wfcerr;
		
		protocol->broadcast(format("ok calib zero :%s", 
															 simwfs->get_refvec_str().c_str() ));

	} 
	else if (ptc->calib == "offsetvec") {	// Add offset vector to correction 
		double xoff = popdouble(ptc->calib_opt);
		double yoff = popdouble(ptc->calib_opt);
		//<! @bug xoff, yoff don't both work?
		
		if (simwfs->calib_offset(xoff, yoff)) {
			io.msg(IO_ERR, "FOAM_FullSim::calib() offset vector could not be applied!");
			return -1;
		} else {
			io.msg(IO_INFO, "FOAM_FullSim::calib() apply offset vector (%g, %g)", xoff, yoff);
			protocol->broadcast(format("ok calib offsetvec %g %g", xoff, yoff));
		}
		
	}
	else if (ptc->calib == "svd") {	// (Re-)calculate SVD given the influence matrix
		// calib svd [singval cutoff] -- 
		// singval < 0: drop these modes
		// singval > 1: use this amount of modes
		// else: use this amount of singular value
		double sval_cutoff = popdouble(ptc->calib_opt);
		if (sval_cutoff == 0.0) sval_cutoff = 0.7;
		io.msg(IO_INFO, "FOAM_FullSim::calib() re-calc SVD, sval=%g", sval_cutoff);
		
		calret = simwfs->update_actmat(simwfc->getname(), sval_cutoff);
		
		// If we failed, return.
		if (calret)
			return -1;
		
		// Broadcast results to clients
		protocol->broadcast(format("ok calib svd singvals :%s", 
															 simwfs->get_singval_str(simwfc->getname()).c_str() ));
		protocol->broadcast(format("ok calib svd condition :%g", simwfs->get_svd_cond(simwfc->getname())));
		protocol->broadcast(format("ok calib svd usage :%g %d", 
															 simwfs->get_svd_singuse(simwfc->getname()),
															 simwfs->get_svd_modeuse(simwfc->getname())
															 ));
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
		string calopts = line;
		conn->write("ok cmd calib");
		ptc->calib = calmode;
		ptc->calib_opt = calopts;
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
