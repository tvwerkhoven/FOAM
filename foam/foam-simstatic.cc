/*
 foam-simstatic.cc -- static simulation module
 Copyright (C) 2008--2011 Tim van Werkhoven <t.i.m.vanwerkhoven@xs4all.nl>
 
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

#include "types.h"
#include "io.h"
#include "path++.h"

#include "foam.h"
#include "devices.h"
#include "imgcam.h"
#include "camera.h"
#include "shwfs.h"
#include "wfs.h"

#include "foam-simstatic.h"

using namespace std;

// Global device list for easier access
ImgCamera *imgcama;
Shwfs *simwfs;

int FOAM_simstatic::load_modules() {
	io.msg(IO_DEB2, "FOAM_simstatic::load_modules()");
	io.msg(IO_INFO, "This is the simstatic prime module, enjoy.");
		
	// Add ImgCam device
	imgcama = new ImgCamera(io, ptc, "imgcamA", ptc->listenport, ptc->conffile);
	devices->add((foam::Device *) imgcama);
	
	// Init WFS simulation (using camera)
	simwfs = new Shwfs(io, ptc, "simshwfs", ptc->listenport, ptc->conffile, *imgcama);
	devices->add((foam::Device *) simwfs);
	
	return 0;
}

// OPEN LOOP ROUTINES //
/*********************/

int FOAM_simstatic::open_init() {
	io.msg(IO_DEB2, "FOAM_simstatic::open_init()");
	
	//((DummyCamera*) devices->get("dummycam"))->set_mode(Camera::RUNNING);
	imgcama->set_mode(Camera::RUNNING);
	return 0;
}

int FOAM_simstatic::open_loop() {
	io.msg(IO_DEB2, "FOAM_simstatic::open_loop()");
	Camera::frame_t *frame = imgcama->get_last_frame();
	
	// Propagate simulated frame through system (WFS, algorithms, WFC)
	Shwfs::wf_info_t *wf_meas = simwfs->measure(frame);
	
	// TODO
	simwfs->comp_ctrlcmd("fakewfc", wf_meas->wfamp, NULL);

	usleep(1.0 * 1000000);
	return 0;
}

int FOAM_simstatic::open_finish() {
	io.msg(IO_DEB2, "FOAM_simstatic::open_finish()");
	
	imgcama->set_mode(Camera::WAITING);
	return 0;
}

// CLOSED LOOP ROUTINES //
/************************/

int FOAM_simstatic::closed_init() {
	io.msg(IO_DEB2, "FOAM_simstatic::closed_init()");
	
	// Run open-loop init first
	open_init();
	
	return 0;
}

int FOAM_simstatic::closed_loop() {
	Camera::frame_t *frame = imgcama->get_last_frame();
	
	// Propagate simulated frame through system (WFS, algorithms, WFC)
	Shwfs::wf_info_t *wf_meas = simwfs->measure(frame);
	
	simwfs->comp_ctrlcmd("fakewfc", wf_meas->wfamp, NULL);
	return 0;
}

int FOAM_simstatic::closed_finish() {
	io.msg(IO_DEB2, "FOAM_simstatic::closed_finish()");
	
	// Run open-loop finish first
	open_finish();

	return 0;
}

// MISC ROUTINES //
/*****************/

int FOAM_simstatic::calib() {
	io.msg(IO_DEB2, "FOAM_simstatic::calib()=%s", ptc->calib.c_str());

	if (ptc->calib == "influence") {
		io.msg(IO_DEB2, "FOAM_simstatic::calib INFLUENCE");
		usleep((useconds_t) 1.0 * 1000000);
		return 0;
	}
	else
		return -1;

	return 0;
}

void FOAM_simstatic::on_message(Connection *connection, string line) {
	io.msg(IO_DEB2, "FOAM_simstatic::on_message(line=%s)", line.c_str());
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
			connection->write("ok calib 1 influence");
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
	FOAM_simstatic foam(argc, argv);
	
	if (foam.init())
		exit(-1);

	foam.io.msg(IO_INFO, "Running simstatic mode");
	foam.listen();
	
	return 0;
}
