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
	
	modSelSubapts(&(ptc->wfs[0]), 0, 0); 			// check samini (2nd param) and samxr (3d param)
	
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
		logInfo("Calibration appears to be OK for all %d WFSs.", ptc->wfs_count);
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
	// todo: add dark/flat field calibrations
	// dark:
	//  add calibration enum
	//  add function to calib.c with defined FILENAME
	//  add switch to drvReadSensor which gives a black image (1)
	// flat:
	//  whats flat? how to do with SH sensor in place? --> take them out
	//  add calib enum
	//  add switch/ function / drvreadsensor
	// sky:
	//  same as flat
	
	logInfo("Switching calibration");
	switch (ptc->calmode) {
		case CAL_PINHOLE: // pinhole WFS calibration
			logInfo("Performing pinhole calibration for WFS %d", 0);
			return modCalPinhole(ptc, 0);
			break;
		case CAL_INFL: // influence matrix
			logInfo("Performing influence matrix calibration for WFS %d", 0);
			return modCalWFC(ptc, 0); // arguments: (control_t *ptc, int wfs)
			break;
		// case CAL_LINTEST: // influence matrix
		// 	logInfo("Performing linearity test using WFS %d", 0);
		// 	return modLinTest(ptc, 0); // arguments: (control_t *ptc, int wfs)
		// 	break;
		default:
			logWarn("Unsupported calibrate mode encountered.");
			return EXIT_FAILURE;
			break;			
	}
}
