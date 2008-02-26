#include "foam_primemod-sim.h"

SDL_Surface *screen;	// Global surface to draw on
SDL_Event event;		// Global SDL event struct to catch user IO

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
	
	// we need to unlock the screen or else something might go wrong
	Sulock(screen);
}

int modOpenInit(control_t *ptc) {

	if (drvReadSensor(ptc) != EXIT_SUCCESS) {		// read the sensor output into ptc.image
		logErr("Error, reading sensor failed.");
		ptc->mode = AO_MODE_LISTEN;
		return EXIT_FAILURE;
	}
	
	modSelSubapts(&(ptc->wfs[0]), 0, -1); 			// check samini (2nd param) and samxr (3d param)
	
	return EXIT_SUCCESS;
}

int modOpenLoop(control_t *ptc) {
	int i;
	// for (i=0; i<ptc->wfc[1].nact; i++)
	// 	ptc->wfc[1].ctrl[i] = ((ptc->frames % 50) / 25.0 - 1);
				
	if (drvReadSensor(ptc) != EXIT_SUCCESS)			// read the sensor output into ptc.image
		return EXIT_FAILURE;
		
	if (modParseSH((&ptc->wfs[0])) != EXIT_SUCCESS)			// process SH sensor output, get displacements
		return EXIT_FAILURE;
	
//	if (ptc->frames % 20 == 0) {
	drawStuff(ptc, 0, screen);

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
		logInfo("Checking if calibrations necessary for closed loop succeeded (WFS %d/%d).", i, ptc->wfs_count);
		if (modCalWFCChk(ptc, i) == EXIT_FAILURE) {
			logWarn("Calibration incomplete for WFS %d, please calibrate first", i);
			ptc->mode = AO_MODE_LISTEN;
			return EXIT_FAILURE;
		}
	}
	return EXIT_SUCCESS;
}

int modClosedLoop(control_t *ptc) {
	// set both actuators to something random
	int i,j;
	
	// for (i=0; i<ptc->wfc_count; i++) {
	// 	logDebug("Setting WFC %d with %d acts.", i, ptc->wfc[i].nact);
	// 	for (j=0; j<ptc->wfc[i].nact; j++)
	// 		ptc->wfc[i].ctrl[j] = drand48()*2-1;
	// }

	if (drvReadSensor(ptc) != EXIT_SUCCESS)				// read the sensor output into ptc.image
		return EXIT_FAILURE;
	
	if (modParseSH((&ptc->wfs[0])) != EXIT_SUCCESS)		// process SH sensor output, get displacements
		return EXIT_FAILURE;
		
	if (modCalcCtrl(ptc, 0, 10) != EXIT_SUCCESS)		// parse displacements, get ctrls for WFC's
		return EXIT_FAILURE;

	// for (i=0; i<ptc->wfc_count; i++) {
	// 	logDebug("Setting WFC %d with %d acts.", i, ptc->wfc[i].nact);
	// 	for (j=0; j<ptc->wfc[i].nact; j++)
	// 		logDirect("%f ",ptc->wfc[i].ctrl[j]);
	// }
	
//	if (ptc->frames % 20 == 0) {
 	drawStuff(ptc, 0, screen);
//	}
	// if (ptc->frames % 20 == 0) {	
	// 	logInfo("Dumping output...");
	// 	modWritePGM("../config/wfsdump.pgm", screen);
	// 	exit(0);
	// }
	

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
			return modCalPinhole(ptc, 0);
			break;
		case CAL_INFL: // influence matrix
			return modCalWFC(ptc, 0); // arguments: (control_t *ptc, int wfs)
			break;
		case CAL_LINTEST: // influence matrix
			return modLinTest(ptc, 0); // arguments: (control_t *ptc, int wfs)
			break;
		default:
			logErr("Unsupported calibrate mode encountered.");
			return EXIT_FAILURE;
			break;			
	}
}
