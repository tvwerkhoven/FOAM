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
	@file foam-dummy.c
	@author Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)
	@date November 30 2007

	@brief This is a dummy module to test the bare FOAM framework capabilities.
*/

// HEADERS //
/***********/

#include "foam.h"
#include "io.h"

conntrack_t clientlist;

extern pthread_mutex_t mode_mutex;
extern pthread_cond_t mode_cond;
extern Io *io;

int modInitModule(control_t *ptc, config_t *cs_config) {
  io->msg(IO_INFO, "Running in dummy mode, don't expect great AO results :)");
	
	// populate ptc here
	ptc->mode = AO_MODE_LISTEN;		// start in listen mode
	ptc->calmode = CAL_INFL;			// this is not really relevant
	ptc->logfrac = 100;           // log verbose messages only every 100 frames    
	ptc->wfs_count = 1;						// 1 FW, WFS and WFC
	ptc->wfc_count = 1;
	ptc->fw_count = 1;
	
	// allocate memory for filters, wfcs and wfss
	// use malloc to make the memory globally available
	ptc->filter = new filtwheel_t[1];
	ptc->wfc = new wfc_t[1]; //(wfc_t *) malloc(ptc->wfc_count * sizeof(wfc_t));
	ptc->wfs = new wfs_t[1]; //(wfs_t *) malloc(ptc->wfs_count * sizeof(wfs_t));
	
	// configure WFS 0
	ptc->wfs[0].name = "SH WFS";
	ptc->wfs[0].res.x = 256;
	ptc->wfs[0].res.y = 256;
	ptc->wfs[0].bpp = 8;
	ptc->wfs[0].darkfile = "";
	ptc->wfs[0].flatfile = "";
	ptc->wfs[0].skyfile = "";
	ptc->wfs[0].scandir = AO_AXES_XY;
	
	// configure WFC 0
	ptc->wfc[0].name = "OkoDM";
	ptc->wfc[0].nact = 37;
	ptc->wfc[0].gain.p = 1.0;
	ptc->wfc[0].gain.i = 1.0;
	ptc->wfc[0].gain.d = 1.0;
	ptc->wfc[0].type = WFC_DM;
	
	// configure filter 0
	ptc->filter[0].name = "Telescope FW";
	ptc->filter[0].nfilts = 3;
	ptc->filter[0].filters[0] = FILT_PINHOLE;
	ptc->filter[0].filters[1] = FILT_OPEN;
	ptc->filter[0].filters[2] = FILT_CLOSED;
		
	// configure cs_config here
	cs_config->listenip = "0.0.0.0";	// listen on any IP by defaul
	cs_config->listenport = "1025";		// listen on port 1010 by default
	cs_config->use_syslog = false;		// don't use the syslog
	cs_config->syslog_prepend = "foam";	// prepend logging with 'foam'
	cs_config->logfile = "./dummylog";					// log to file

	return EXIT_SUCCESS;
}

int modPostInitModule(control_t *ptc, config_t *cs_config) {
	return EXIT_SUCCESS;
}

int modOpenInit(control_t *ptc) {
	return EXIT_SUCCESS;
}
void modStopModule(control_t *ptc) {
	delete[] ptc->filter;
	delete[] ptc->wfc;
	delete[] ptc->wfs;
	// placeholder ftw!
}

int modOpenLoop(control_t *ptc) {
	return EXIT_SUCCESS;
}

int modOpenFinish(control_t *ptc) {
	return EXIT_SUCCESS;
}


int modClosedInit(control_t *ptc) {
	return EXIT_SUCCESS;
}

int modClosedLoop(control_t *ptc) {
	return EXIT_SUCCESS;
}

int modClosedFinish(control_t *ptc) {
	return EXIT_SUCCESS;
}

int modCalibrate(control_t *ptc) {
	return EXIT_SUCCESS;
}

int modMessage(control_t *ptc, Connection *connection, string cmd, string rest) {
	return 0;
}
