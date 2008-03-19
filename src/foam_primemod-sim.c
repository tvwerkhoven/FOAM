/*! 
	@file foam_primemod-simstatic.c
	@author @authortim
	@date 2008-03-12

	@brief This is the prime module to run @name in simulation mode.
	
	\section Info
	
	Simulation mode can be used to test the qualitative performance of certain modules. This simulation
	starts with a statically generated wavefront and then simulates the telescope aperture, followed
	by a simulation of the various wavefront correctors (TT and DM).
	Finally, a Shack-Hartmann wavefront sensor (lenslet array) is simulated where the wavefront is converted
	to an image. After that, the sensor output is available and from there on, simulation is not necessary anymore.
	The simulated sensor output can be used to do correlation or center-of-gravity tracking on.
	
	\info Dependencies
	
	This prime module needs the following modules for a complete package:
	\li foam_modules-sim.c - to simulate the AO system (wavefront, telescope, DM/TT)
	\li foam_modules-sh.c - to track targets
	\li foam_modules-pgm.c - to read PGM files
	
*/

// HEADERS //
/***********/

#include "foam_primemod-sim.h"

// GLOBALS //
/***********/

SDL_Surface *screen;	// Global surface to draw on
SDL_Event event;		// Global SDL event struct to catch user IO
extern FILE *ttfd;

// ROUTINES //
/************/

int modInitModule(control_t *ptc) {

    /* Initialize defaults, Video and Audio */
    if((SDL_Init(SDL_INIT_VIDEO) == -1)) 
		logErr("Could not initialize SDL: %s.\n", SDL_GetError());

	atexit(SDL_Quit);
	
	SDL_WM_SetCaption("WFS 0 output", "WFS 0 output");

	screen = SDL_SetVideoMode(ptc->wfs[0].res.x, ptc->wfs[0].res.y, 0, SDL_HWSURFACE|SDL_DOUBLEBUF);
	if (screen == NULL) 
		logErr("Unable to set video: %s", SDL_GetError());
		
	return EXIT_SUCCESS;
}

void modStopModule(control_t *ptc) {
	// let's just do nothing here because we're done anyway :P
	
	fclose(ttfd);
	// we need to unlock the screen or else something might go wrong
	modFinishDraw(screen);
}

int modOpenInit(control_t *ptc) {

	if (drvReadSensor(ptc) != EXIT_SUCCESS) {		// read the sensor output into ptc.image
		logErr("Error, reading sensor failed.");
		ptc->mode = AO_MODE_LISTEN;
		return EXIT_FAILURE;
	}
	
	modSelSubapts(ptc->wfs[0].image, ptc->wfs[0].res, ptc->wfs[0].cells, ptc->wfs[0].subc, ptc->wfs[0].gridc, &(ptc->wfs[0].nsubap), 0, 0);
	
	return EXIT_SUCCESS;
}

int modOpenLoop(control_t *ptc) {
	if (drvReadSensor(ptc) != EXIT_SUCCESS)			// read the sensor output into ptc.image
		return EXIT_FAILURE;
		
	if (modParseSH((&ptc->wfs[0])) != EXIT_SUCCESS)		// process SH sensor output, get displacements
		return EXIT_FAILURE;
	
//	if (ptc->frames % 20 == 0) {
	// modDrawSens(ptc, 0, screen);
	modDrawStuff(ptc, 0, screen);

//	}

	if (SDL_PollEvent(&event))
		if (event.type == SDL_QUIT)
			stopFOAM();
			
	return EXIT_SUCCESS;
}

int modClosedInit(control_t *ptc) {
	int i;
	// this is the same for open and closed modes, don't rewrite stuff
	if (modOpenInit(ptc) != EXIT_SUCCESS)
		return EXIT_FAILURE;

	
	// // check if calibration is done
	for (i=0; i < ptc->wfs_count; i++) {
		
		if (modCalWFCChk(ptc, i) == EXIT_FAILURE) {
			logWarn("Calibration incomplete for WFS %d, please calibrate first", i);
			ptc->mode = AO_MODE_LISTEN;
			return EXIT_FAILURE;
		}
		logInfo(0, "Calibration appears to be OK for all %d WFSs.", ptc->wfs_count);
	}
	return EXIT_SUCCESS;
}

int modClosedLoop(control_t *ptc) {
	// set both actuators to something random
	int i,j;
	
	
	if (drvReadSensor(ptc) != EXIT_SUCCESS)				// read the sensor output into ptc.image
		return EXIT_FAILURE;
	
	if (modParseSH((&ptc->wfs[0])) != EXIT_SUCCESS)		// process SH sensor output, get displacements
		return EXIT_FAILURE;
		
	if (modCalcCtrl(ptc, 0, 0) != EXIT_SUCCESS)		// parse displacements, get ctrls for WFC's
		return EXIT_FAILURE;

//	if (ptc->frames % 20 == 0) {
 	modDrawStuff(ptc, 0, screen);
//	}	

	if (SDL_PollEvent(&event))
		if (event.type == SDL_QUIT)
			stopFOAM();
		
	return EXIT_SUCCESS;
}

int modCalibrate(control_t *ptc) {	
	logInfo(0, "Switching calibration");
	switch (ptc->calmode) {
		case CAL_PINHOLE: // pinhole WFS calibration
			logInfo(0, "Performing pinhole calibration for WFS %d", 0);
			return modCalPinhole(ptc, 0);
			break;
		case CAL_INFL: // influence matrix
			logInfo(0, "Performing influence matrix calibration for WFS %d", 0);
			return modCalWFC(ptc, 0); // arguments: (control_t *ptc, int wfs)
			break;
		// case CAL_LINTEST: // influence matrix
		// 	logInfo(0, "Performing linearity test using WFS %d", 0);
		// 	return modLinTest(ptc, 0); // arguments: (control_t *ptc, int wfs)
		// 	break;
		default:
			logWarn("Unsupported calibrate mode encountered.");
			return EXIT_FAILURE;
			break;			
	}
}

int modMessage(control_t *ptc, const client_t *client, char *list[], const int count) {
	// spaces are important!!!
	float tmpfl;
	int tmpint;
	
 	if (strcmp(list[0],"help") == 0) {
		// give module specific help here
		if (count > 1) { 
			// give help on a specific command
			if (strcmp(list[1], "calibrate") == 0) {
				tellClient(client->buf_ev, "\
200 OK HELP CALIBRATE\n\
calibrate <mode>\n\
   mode=pinhole: do a pinhole calibration.\n\
   mode=influence: do a WFC influence matrix calibration.");
			}
			else if (strcmp(list[1], "step") == 0) {
				tellClient(client->buf_ev, "\
200 OK HELP STEP\n\
step <x|y> [d]\n\
    step the AO system d pixels in the x or y direction.\n\
    if d is omitted, +1 is assumed.");
			}
			else if (strcmp(list[1], "gain") == 0) {
				tellClient(client->buf_ev, "\
200 OK HELP STEP\n\
gain <wfc> <gain>\n\
    set the gain for a certain wfc to <gain>.");
			}
			else { // oops, the user requested help on a topic (list[1])
				// we don't know. tell this to parseCmd by returning 0
				return 0;
			}
		}
		else {
			tellClient(client->buf_ev, "\
step <x|y> [d]:         step a wfs in the x or y direction\n\
gain <wfc> <gain>:      set the gain for a wfc");
		}
	}
 	else if (strcmp(list[0],"step") == 0) {
		if (ptc->mode == AO_MODE_CAL) 
			tellClient(client->buf_ev,"403 STEP NOT ALLOWED DURING CALIBRATION");

		else if (count > 1) {
			if (strcmp(list[1],"x") == 0) {
				if (count > 2) {
					tmpfl = strtof(list[3], NULL);
					if (tmpfl > -10 && tmpfl < 10) {
						ptc->wfs[0].stepc.x = tmpfl;
						tellClients("200 OK STEP X %+.2f", tmpfl);
					}
					else tellClient(client->buf_ev,"401 INVALID STEPSIZE");
				}
				else {
					ptc->wfs[0].stepc.x += 1;
					tellClients("200 OK STEP X +1");
				}
			}
			else if (strcmp(list[1],"y") == 0) {
				if (count > 2) {
					tmpfl = strtof(list[3], NULL);
					if (tmpfl > -10 && tmpfl < 10) {
						ptc->wfs[0].stepc.y = tmpfl;
						tellClients("200 OK STEP Y %+.2f", tmpfl);
					}
					else tellClient(client->buf_ev,"401 INVALID STEPSIZE");
				}
				else {
					ptc->wfs[0].stepc.y += 1;
					tellClients("200 OK STEP Y +1");
				}
			}
			else tellClient(client->buf_ev,"401 UNKNOWN STEP");
		}
		else tellClient(client->buf_ev,"402 STEP REQUIRES ARG");
	}
	else if (strcmp(list[0],"gain") == 0) {
		if (count > 2) {
			tmpint = strtol(list[1], NULL, 10);
			tmpfl = strtof(list[2], NULL);
			if (tmpint >= 0 && tmpint < ptc->wfc_count) {
				if (tmpfl > -5 && tmpfl < 5) {
					tellClients("200 OK GAIN %+.4f", tmpfl);
					ptc->wfc[tmpint].gain = tmpfl;
				}
				else tellClient(client->buf_ev,"401 INVALID GAIN %d", tmpfl);
			}
			else tellClient(client->buf_ev,"401 UNKNOWN WFC %d", tmpint);
		}
		else tellClient(client->buf_ev,"402 GAIN REQUIRES ARG");
	}
	else if (strcmp(list[0],"calibrate") == 0) {
		if (count > 1) {

			if (strcmp(list[1],"pinhole") == 0) {
				ptc->mode = AO_MODE_CAL;
				ptc->calmode = CAL_PINHOLE;
				pthread_mutex_lock(&mode_mutex);
				pthread_cond_signal(&mode_cond);
				pthread_mutex_unlock(&mode_mutex);
				tellClients("200 OK CALIBRATE PINHOLE");
			}
			else if (strcmp(list[1],"lintest") == 0) {
				ptc->mode = AO_MODE_CAL;
				ptc->calmode = CAL_LINTEST;
				pthread_mutex_lock(&mode_mutex);
				pthread_cond_signal(&mode_cond);
				pthread_mutex_unlock(&mode_mutex);
				tellClients("200 OK CALIBRATE LINTEST");
			}
			else if (strcmp(list[1],"influence") == 0) {
				ptc->mode = AO_MODE_CAL;
				ptc->calmode = CAL_INFL;
				pthread_mutex_lock(&mode_mutex);
				pthread_cond_signal(&mode_cond);
				pthread_mutex_unlock(&mode_mutex);	
				tellClients("200 OK CALIBRATE INFLUENCE");
			}
			else tellClient(client->buf_ev,"401 UNKNOWN CALIBRATION");
		}
		else tellClient(client->buf_ev,"402 CALIBRATE REQUIRES ARG");
	}
	else { // no valid command found? return 0 so that the main thread knows this
		return 0;
	} // strcmp stops here
	
	// if we end up here, we didn't return 0, so we found a valid command
	return 1;
	
}

