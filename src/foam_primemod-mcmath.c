/*! 
	@file foam_primemod-mcmath.c
	@author @authortim
	@date isoD	2008-04-18

	@brief This is the McMath prime-module which can be used at that telescope.
*/

// HEADERS //
/***********/

// this is done via the command line using -include, see Makefile.am
//#include "foam_primemod-dummy.h"

// We need these for modMessage, see foam_cs.c
extern pthread_mutex_t mode_mutex;
extern pthread_cond_t mode_cond;

// some globals we use
mod_display_t disp;

int modInitModule(control_t *ptc, config_t *cs_config) {
	logInfo(0, "This is the McMath prime module, enjoy.");
	
	// populate ptc here
	ptc->mode = AO_MODE_LISTEN;			// start in listen mode (safe bet, you probably want this)
	ptc->calmode = CAL_INFL;			// this is not really relevant
	ptc->wfs_count = 1;					// 2 FW, 1 WFS and 2 WFC
	ptc->wfc_count = 2;
	ptc->fw_count = 2;
	
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
	ptc->wfc[0].name = "Okotech DM";
	ptc->wfc[0].nact = 37;
	ptc->wfc[0].gain = 1.0;
	ptc->wfc[0].type = WFC_DM;
	
	// configure WFC 1
	ptc->wfc[1].name = "TT";
	ptc->wfc[1].nact = 2;
	ptc->wfc[1].gain = 1.0;
	ptc->wfc[1].type = WFC_TT;
	
	// configure filter 0
	ptc->filter[0].name = "Telescope FW";
	ptc->filter[0].nfilts = 3;
	ptc->filter[0].filters[0] = FILT_PINHOLE;
	ptc->filter[0].filters[1] = FILT_OPEN;
	ptc->filter[0].filters[2] = FILT_CLOSED;

	ptc->filter[0].name = "WFS FW";
	ptc->filter[0].nfilts = 3;
	ptc->filter[0].filters[0] = FILT_PINHOLE;
	ptc->filter[0].filters[1] = FILT_OPEN;
	ptc->filter[0].filters[2] = FILT_CLOSED;
	
	// configure cs_config here
	cs_config->listenip = "0.0.0.0";	// listen on any IP by defaul
	cs_config->listenport = 10000;		// listen on port 10000 by default
	cs_config->use_syslog = false;		// don't use the syslog
	cs_config->syslog_prepend = "foam-mm";	// prepend logging with 'foam'
	cs_config->use_stdout = true;		// do use stdout
	cs_config->loglevel = LOGDEBUG;		// log error, info and debug
	cs_config->logfrac = 100;			// log verbose messages only every 100 frames
	cs_config->infofile = NULL;			// don't log anything to file
	cs_config->errfile = NULL;
	cs_config->debugfile = NULL;

	return EXIT_SUCCESS;
}

int modOpenInit(control_t *ptc) {
	// init display
	disp.caption = "MM - WFS";
	disp.res.x = ptc->wfs[0].res.x;
	disp.res.y = ptc->wfs[0].res.y;
	disp.flags = SDL_HWSURFACE | SDL_DOUBLEBUF | SDL_RESIZABLE;
	
	modInitDraw(&disp);
	return EXIT_SUCCESS;
}
void modStopModule(control_t *ptc) {
	// placeholder ftw!
	modFinishDraw
}

int modOpenLoop(control_t *ptc) {
	return EXIT_SUCCESS;
}

int modClosedInit(control_t *ptc) {
	return EXIT_SUCCESS;
}

int modClosedLoop(control_t *ptc) {
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
	else { // no valid command found? return 0 so that the main thread knows this
		return 0;
	} // strcmp stops here
	
	// if we end up here, we didn't return 0, so we found a valid command
	return 1;
	
}