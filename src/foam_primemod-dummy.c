/*! 
	@file foam_primemod-dummy.c
	@author @authortim
	@date November 30 2007

	@brief This is a dummy module to test the bare FOAM framework capabilities.
*/

// HEADERS //
/***********/

//#include "foam_primemod-dummy.h"

extern pthread_mutex_t mode_mutex;
extern pthread_cond_t mode_cond;

int modInitModule(control_t *ptc, config_t *cs_config) {
	logInfo(0, "Running in dummy mode, don't expect great AO results :)");
	// populate ptc here
	ptc->mode = AO_MODE_LISTEN;			// start in listen mode (safe bet, you probably want this)
	ptc->calmode = CAL_INFL;			// this is not really relevant
	ptc->wfs_count = 0;					// we have a boring AO system with no filters, WFSs or WFCs
	ptc->wfc_count = 0;
	ptc->fw_count = 0;
	
	// configure cs_config here
	cs_config->listenip = "0.0.0.0";	// listen on any IP by default, can be overridden by config file
	cs_config->listenport = 10000;		// listen on port 10000 by default
	cs_config->use_syslog = false;		// don't use the syslog
	cs_config->syslog_prepend = "foam";	// prepend logging with 'foam'
	cs_config->use_stdout = true;		// do use stdout
	cs_config->loglevel = LOGDEBUG;		// log error, info and debug
	cs_config->logfrac = 100;			// log verbose messages only every 100 frames
	cs_config->infofile = NULL;			// don't log anything to file
	cs_config->errfile = NULL;
	cs_config->debugfile = NULL;

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