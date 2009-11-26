/*
 Copyright (C) 2008 Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)
 
 This file is part of FOAM.
 
 FOAM is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 FOAM is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with FOAM.  If not, see <http://www.gnu.org/licenses/>.

 $Id$
 */
/*! 
 @file foam-simstatic.c
 @author Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)
 @date 2008-04-18
 
 @brief This is a static simulation mode, with just a simple image to work with.
 
 This primemodule can be used to benchmark performance of the AO system if no
 AO hardware (camera, TT, DM) is present. This is branched off of the mcmath
 prime module.
 */

// HEADERS //
/***********/

#include "foam-simstatic.h"
#include "types.h"
#include "io.h"
#include "wfs.h"
#include "cam.h"

// We need these for modMessage, see foam.cc
extern pthread_mutex_t mode_mutex;
extern pthread_cond_t mode_cond;

extern Io *io;

// GLOBALS //
/***********/

int modInitModule(foamctrl *ptc, foamcfg *cs_config) {
	io->msg(IO_DEB2, __FILE__ "::modInitModule(foamctrl *ptc, foamcfg *cs_config)");

	io->msg(IO_INFO, "This is the simstatic prime module, enjoy.");
	
	// Set up WFS #1 with dummy camera
	ptc->wfs[0] = Wfs::create(ptc->wfscfgs[0]);
	
	return EXIT_SUCCESS;
}

int modPostInitModule(foamctrl *ptc, foamcfg *cs_config) {
	io->msg(IO_DEB2, __FILE__ "::modPostInitModule(foamctrl *ptc, foamcfg *cs_config)");
	return EXIT_SUCCESS;
}

void modStopModule(foamctrl *ptc) {
	io->msg(IO_DEB2, __FILE__ "::modStopModule(foamctrl *ptc)");
}

// OPEN LOOP ROUTINES //
/*********************/

int modOpenInit(foamctrl *ptc) {
	io->msg(IO_DEB2, __FILE__ "::modOpenInit(foamctrl *ptc)");
	
	ptc->wfs[0]->cam->set_mode(Camera::RUNNING);
	ptc->wfs[0]->cam->init_capture();
	
	return EXIT_SUCCESS;
}

int modOpenLoop(foamctrl *ptc) {
	io->msg(IO_DEB2, __FILE__ "::modOpenLoop(foamctrl *ptc)");
	
	usleep(1000000);
	return EXIT_SUCCESS;
}

int modOpenFinish(foamctrl *ptc) {
	io->msg(IO_DEB2, __FILE__ "::modOpenFinish(foamctrl *ptc)");
	
	ptc->wfs[0]->cam->set_mode(Camera::OFF);

	return EXIT_SUCCESS;
}

// CLOSED LOOP ROUTINES //
/************************/

int modClosedInit(foamctrl *ptc) {
	io->msg(IO_DEB2, __FILE__ "::modClosedInit(foamctrl *ptc)");
	
	modOpenInit(ptc);
	
	return EXIT_SUCCESS;
}

int modClosedLoop(foamctrl *ptc) {
	io->msg(IO_DEB2, __FILE__ "::modClosedLoop(foamctrl *ptc)");

	usleep(1000);
	return EXIT_SUCCESS;
}

int modClosedFinish(foamctrl *ptc) {
	io->msg(IO_DEB2, __FILE__ "::modClosedFinish(foamctrl *ptc)");
	
	modOpenFinish(ptc);

	return EXIT_SUCCESS;
}

// MISC ROUTINES //
/*****************/

int modCalibrate(foamctrl *ptc) {
	io->msg(IO_DEB2, __FILE__ "::modCalibrate(foamctrl *ptc)");

	if (ptc->calmode == CAL_SUBAPSEL) {
		io->msg(IO_DEB2, __FILE__ "::modCalibrate CAL_SUBAPSEL");
		ptc->wfs[0]->calibrate();
	}
		
	return EXIT_SUCCESS;
}

int modMessage(foamctrl *ptc, Connection *connection, string cmd, string line) {
	// Quick recap of messaging codes:
	// 400 UNKNOWN
	// 401 UNKNOWN MODE
	// 402 MODE REQUIRES ARG
	// 403 MODE FORBIDDEN
	// 300 ERROR
	// 200 OK
	// 201 STATE CHANGE
	if (cmd == "help") {
		string topic = popword(line);
		if (topic.size() == 0) {
			connection->write("This is the simstat module of FOAM.");
		}
		else if (topic == "calib") {
			connection->write(\
												"calib <mode>:           Calibrate AO system.\n"
												"  mode=sasel:			     Select subapertures.");
		}
		else {
			return -1;
		}
	}
	else if (cmd == "calib") {
		string calmode = popword(line);
		if (calmode == "sasel") {
			connection->server->broadcast("201 :CALIB OK SUBAPSEL");
			ptc->calmode = CAL_SUBAPSEL;
			ptc->mode = AO_MODE_CAL;
			pthread_cond_signal(&mode_cond); // signal a change to the main thread
		}
		else {
			connection->write("401 :CALIB MODE UNKNOWN");
		}	
	}
	else {
		return -1;
	}
	
	// if we end up here, we didn't return 0, so we found a valid command
	return 0;
}
