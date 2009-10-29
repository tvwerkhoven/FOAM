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
	@file foam_primemod-dummy.c
	@author @authortim
	@date November 30 2007

	@brief This is a dummy module to test the bare FOAM framework capabilities.
*/

// HEADERS //
/***********/

#include "libfoam.h"

extern pthread_mutex_t mode_mutex;
extern pthread_cond_t mode_cond;

int modInitModule(control_t *ptc, config_t *cs_config) {
	logInfo(0, "Running in dummy mode, don't expect great AO results :)");
	
	// populate ptc here
	ptc->mode = AO_MODE_LISTEN;			// start in listen mode (safe bet, you probably want this)
	ptc->calmode = CAL_INFL;			// this is not really relevant
	ptc->logfrac = 100;                 // log verbose messages only every 100 frames    
	ptc->wfs_count = 1;					// 1 FW, WFS and WFC
	ptc->wfc_count = 1;
	ptc->fw_count = 1;
	
	// allocate memory for filters, wfcs and wfss
	// use malloc to make the memory globally available
	ptc->filter = (filtwheel_t *) malloc(ptc->fw_count * sizeof(filtwheel_t));
	ptc->wfc = (wfc_t *) malloc(ptc->wfc_count * sizeof(wfc_t));
	ptc->wfs = (wfs_t *) malloc(ptc->wfs_count * sizeof(wfs_t));
	
	// configure WFS 0
	ptc->wfs[0].name = "SH WFS";
	ptc->wfs[0].res.x = 256;
	ptc->wfs[0].res.y = 256;
	ptc->wfs[0].darkfile = NULL;
	ptc->wfs[0].flatfile = NULL;
	ptc->wfs[0].skyfile = NULL;
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
	cs_config->listenport = 10000;		// listen on port 10000 by default
	cs_config->use_syslog = false;		// don't use the syslog
	cs_config->syslog_prepend = "foam";	// prepend logging with 'foam'
	cs_config->use_stdout = true;		// do use stdout
	cs_config->loglevel = LOGDEBUG;		// log error, info and debug
	cs_config->infofile = NULL;			// don't log anything to file
	cs_config->errfile = NULL;
	cs_config->debugfile = NULL;

	return EXIT_SUCCESS;
}

int modPostInitModule(control_t *ptc, config_t *cs_config) {
	return EXIT_SUCCESS;
}

int modOpenInit(control_t *ptc) {
	return EXIT_SUCCESS;
}
void modStopModule(control_t *ptc) {
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

int modMessage(control_t *ptc, const client_t *client, char *list[], const int count) {
	// spaces are important!!!	
 	if (strcmp(list[0],"help") == 0) {
		// give module specific help here
		if (count > 1) { 
			// we don't know. tell this to parseCmd by returning 0
			return 0;
		}
		else {
			tellClient(client->buf_ev, "This is the dummy module and does not provide any additional commands");
		}
	}
	else { // no valid command found? tell the user this is the dummy module, there is nothing useful here
		tellClient(client->buf_ev, "This is the dummy module and does not provide any additional commands");
	} // strcmp stops here
	
	// if we end up here, we didn't return 0, so we found a valid command
	return 1;
	
}
