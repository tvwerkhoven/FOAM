/*! 
	@file foam_modules-sim.c
	@author @authortim
	@date November 30 2007

	@brief This file contains the functions to run @name in simulation mode, and 
	links to other files like foam_modules-dm.c which simulates the DM.
*/

// HEADERS //
/***********/

#include "foam_primemod-simstatic.h"

#define FOAM_MODSIM_STATIC "../config/simstaticrect.pgm"

SDL_Surface *simimgsurf=NULL;	// Global surface to draw on

SDL_Surface *screen;		// Global surface to draw on
SDL_Event event;			// Global SDL event struct to catch user IO

int modInitModule(control_t *ptc) {

    /* Initialize defaults, Video and Audio */
    if((SDL_Init(SDL_INIT_VIDEO) == -1)) { 
        logErr("Could not initialize SDL: %s.\n", SDL_GetError());
		return EXIT_FAILURE;
    }
	atexit(SDL_Quit);

	SDL_WM_SetCaption("WFS output", "WFS output");

	screen = SDL_SetVideoMode(ptc->wfs[0].res.x, ptc->wfs[0].res.y, 0, SDL_HWSURFACE|SDL_DOUBLEBUF);
	if (screen == NULL) {
		logErr("Unable to set video: %s", SDL_GetError());
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

void modStopModule(control_t *ptc) {
	// let's just do nothing here because we're done anyway :P
}

int modOpenInit(control_t *ptc) {

	if (drvReadSensor(ptc) != EXIT_SUCCESS) {		// read the sensor output into ptc.image
		logWarn("Error, reading sensor failed.");
		return EXIT_FAILURE;
	}
	
	modSelSubapts(&(ptc->wfs[0]), 0, -1); 			// check samini (2nd param) and samxr (3d param)
	
	return EXIT_SUCCESS;
}

int modOpenLoop(control_t *ptc) {
	int i;
	// for (i=0; i<ptc->wfc[1].nact; i++)
	// 	ptc->wfc[1].ctrl[i] = ((ptc->frames % 50) / 25.0 - 1);
				
	if (drvReadSensor(ptc) != EXIT_SUCCESS) {		// read the sensor output into ptc.image
		logWarn("Error, reading sensor failed.");
		return EXIT_FAILURE;
	}
		
	if (modParseSH((&ptc->wfs[0])) != EXIT_SUCCESS)			// process SH sensor output, get displacements
		return EXIT_FAILURE;
	
	logInfo("frame: %ld", ptc->frames);
	if (ptc->frames % 500 == 0) {
		Slock(screen);
		displayImg(ptc->wfs[0].image, ptc->wfs[0].res, screen);
		modDrawGrid(&(ptc->wfs[0]), screen);
		modDrawSubapts(&(ptc->wfs[0]), screen);
		modDrawVecs(&(ptc->wfs[0]), screen);
		Sulock(screen);
		SDL_Flip(screen);
	}

	if (ptc->frames > 20000)
		stopFOAM();
		
	if (SDL_PollEvent(&event))
		if (event.type == SDL_QUIT)
			stopFOAM();
			
	return EXIT_SUCCESS;
}

int modClosedInit(control_t *ptc) {
	// this is the same for open and closed modes, don't rewrite stuff
	if (modOpenInit(ptc) != EXIT_SUCCESS)
		return EXIT_FAILURE;
		
	return EXIT_SUCCESS;
}

int modClosedLoop(control_t *ptc) {
	// set both actuators to something random
	int i,j;
	
	for (i=0; i<ptc->wfc_count; i++) {
		logDebug("Setting WFC %d with %d acts.", i, ptc->wfc[i].nact);
		for (j=0; j<ptc->wfc[i].nact; j++)
			ptc->wfc[i].ctrl[j] = drand48()*2-1;
	}

	if (drvReadSensor(ptc) != EXIT_SUCCESS) {		// read the sensor output into ptc.image
		logWarn("Error, reading sensor failed.");
		return EXIT_FAILURE;
	}

	//modSelSubapts(&ptc.wfs[0], 0, 0); 			// check samini (2nd param) and samxr (3d param)				
	
	if (modParseSH((&ptc->wfs[0])) != EXIT_SUCCESS)			// process SH sensor output, get displacements
		return EXIT_SUCCESS;
	
	logInfo("frame: %ld", ptc->frames);
	if (ptc->frames % 500 == 0) {
		Slock(screen);
		displayImg(ptc->wfs[0].image, ptc->wfs[0].res, screen);
		modDrawGrid(&(ptc->wfs[0]), screen);
		modDrawSubapts(&(ptc->wfs[0]), screen);
		modDrawVecs(&(ptc->wfs[0]), screen);
		Sulock(screen);
		SDL_Flip(screen);
		
	}
	
	if (ptc->frames > 20000)
		stopFOAM();

	
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
	logInfo("No calibration in static simulation mode");	
	// switch (ptc->calmode) {
	// 	case CAL_PINHOLE: // pinhole WFS calibration
	// 		return modCalPinhole(ptc, 0);
	// 		break;
	// 	case CAL_INFL: // influence matrix
	// 		return modCalWFC(ptc, 0); // arguments: (control_t *ptc, int wfs)
	// 		break;
	// 	default:
	// 		logErr("Unsupported calibrate mode encountered.");
	// 		return EXIT_FAILURE;
	// 		break;			
	// }
}

int drvReadSensor() {
	int i, x, y;
	logDebug("Now reading %d sensors", ptc.wfs_count);
	
	if (ptc.wfs_count < 1) {
		logWarn("Nothing to process, no WFSs defined.");
		return EXIT_FAILURE;
	}
	
	coord_t res = ptc.wfs[0].res;
	
	// if filterwheel is set to pinhole, simulate a coherent image
	if (ptc.mode == AO_MODE_CAL && ptc.filter == FILT_PINHOLE) {
		logWarn("Calibration not supported in static simulation mode.");
		return EXIT_FAILURE;
	}
	else {
		if (simimgsurf == NULL) {
			if (modReadPGM(FOAM_MODSIM_STATIC, &simimgsurf) != EXIT_SUCCESS) {
				logErr("Cannot read static image.");
				return EXIT_FAILURE;
			}
			if (simimgsurf->w != res.x || simimgsurf->h != res.y) {
				logWarn("Simulation resolution incorrect for %s! (%dx%d vs %dx%d)", \
					FOAM_MODSIM_STATIC, res.x, res.y, simimgsurf->w, simimgsurf->h);
				return EXIT_FAILURE;			
			}
			// copy from SDL_Surface to array so we can work with it
			//simimg = calloc( res.x*res.y, sizeof(*simimg));
			// if (simimg == NULL) {
			// 	logErr("Error allocating memory for simulation image");
			// 	return EXIT_FAILURE;
			// }
			// else {
			for (y=0; y<res.y; y++)
				for (x=0; x<res.x; x++)
					ptc.wfs[0].image[y*res.x + x] = (float) getpixel(simimgsurf, x, y);
						
			// }
		}
	}

	return EXIT_SUCCESS;
}

int drvSetActuator(control_t *ptc, int wfc) {
	// this is a placeholder, since we simulate the DM, we don't need to do 
	// anything here. 
	return EXIT_SUCCESS;
}

int modCalcDMVolt() {
	logDebug("Calculating DM voltages");
	return EXIT_SUCCESS;
}

// TODO: document this
int drvFilterWheel(control_t *ptc, fwheel_t mode) {
	// in simulation this is easy, just set mode in ptc
	ptc->filter = mode;
	return EXIT_SUCCESS;
}
