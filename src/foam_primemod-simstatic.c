/*! 
	@file foam_primemod-simstatic.c
	@author @authortim
	@date 2008-03-12

	@brief This is the prime module to run @name in static simulation.
	
	\section Info
	
	Static simulation in this sense means that only 1 image is loaded as simulated sensor output.
	This can be useful to test actual system performance as this image is usually provided by 
	the wavefront sensors themselves and thus costs no time. In simulation mode however, this 
	image must be generated each time, and since this costs time, real benchmarking is not possible.
	This static simulation tries to solve that.
	
	\section Dependencies
	
	This prime module needs the following modules for a complete package:
	\li foam_modules-display.c - to display WFS output
	\li foam_modules-sh.c - to track targets
	\li foam_modules-pgm.c - to read PGM files
	
*/

// HEADERS //
/***********/

#include "foam_primemod-simstatic.h"

#define FOAM_MODSIM_STATIC "../config/simstatic.pgm"

// GLOBALS //
/***********/

float *simimgsurf=NULL;		// Global surface to draw on
SDL_Surface *screen;		// Global surface to draw on
SDL_Event event;			// Global SDL event struct to catch user IO

// ROUTINES //
/************/

int modInitModule(control_t *ptc) {
    // initialize video
    if (SDL_Init(SDL_INIT_VIDEO) == -1) 
		logErr("Could not initialize SDL: %s", SDL_GetError());
		
	atexit(SDL_Quit);

	SDL_WM_SetCaption("WFS output", "WFS output");

	screen = SDL_SetVideoMode(ptc->wfs[0].res.x, ptc->wfs[0].res.y, 0, SDL_HWSURFACE|SDL_DOUBLEBUF);
	if (screen == NULL) 
		logErr("Unable to set video: %s", SDL_GetError());

	return EXIT_SUCCESS;
}

void modStopModule(control_t *ptc) {
	// Unlock the screen, we might have stopped the module during drawing
	modFinishDraw(screen);
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
				
	if (drvReadSensor(ptc) != EXIT_SUCCESS) {		// read the sensor output into ptc.image
		logWarn("Error, reading sensor failed.");
		return EXIT_FAILURE;
	}
		
	if (modParseSH((&ptc->wfs[0])) != EXIT_SUCCESS)			// process SH sensor output, get displacements
		return EXIT_FAILURE;
	
	logDebug("Frame: %ld", ptc->frames);
	if (ptc->frames % 500 == 0) 
		modDrawStuff(ptc, 0, screen);

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
	if (drvReadSensor(ptc) != EXIT_SUCCESS) {		// read the sensor output into ptc.image
		logWarn("Error, reading sensor failed.");
		return EXIT_FAILURE;
	}
	
	if (modParseSH((&ptc->wfs[0])) != EXIT_SUCCESS)	// process SH sensor output, get displacements
		return EXIT_FAILURE;
	
	if (modCalcCtrlFake(ptc, 0, 0) != EXIT_SUCCESS)	// process SH sensor output, get displacements
		return EXIT_FAILURE;	
		
	logDebug("frame: %ld", ptc->frames);
	if (ptc->frames % 500 == 0) 
		modDrawStuff(ptc, 0, screen);
	
	if (ptc->frames > 20000)
		stopFOAM();

	
	if (SDL_PollEvent(&event))
		if (event.type == SDL_QUIT)
			stopFOAM();
		
	return EXIT_SUCCESS;
}

int modCalibrate(control_t *ptc) {

	logInfo("No calibration in static simulation mode");

	return EXIT_SUCCESS;
}

int drvReadSensor() {
	int i, x, y;
	int simres[2];
	// logDebug("Now reading %d sensors", ptc.wfs_count);
	
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
			if (modReadPGMArr(FOAM_MODSIM_STATIC, &simimgsurf, simres) != EXIT_SUCCESS)
				logErr("Cannot read static image.");

			if (simres[0] != res.x || simres[1] != res.y) {
				logWarn("Simulation resolution incorrect for %s! (%dx%d vs %dx%d)", \
					FOAM_MODSIM_STATIC, res.x, res.y, simres[0], simres[1]);
				return EXIT_FAILURE;			
			}
			
			for (y=0; y<res.y*res.x; y++)
				ptc.wfs[0].image[y] = simimgsurf[y];
				
		}
	}

	return EXIT_SUCCESS;
}

int drvSetActuator(control_t *ptc, int wfc) {
	// this is a placeholder, since we simulate the DM, we don't need to do 
	// anything here. 
	return EXIT_SUCCESS;
}

gsl_matrix_float *inflf=NULL, *vf=NULL;
gsl_vector_float *singf, *workf, *dispf, *actf;

int modCalcCtrlFake(control_t *ptc, const int wfs, int nmodes) {
	int i, j;
	int nsubap = ptc->wfs[0].nsubap;
	int nacttot = 39;
	
	// function assumes presence of dmmodes, singular and wfsmodes...
	if (inflf == NULL || vf == NULL) {
		logInfo("First modCalcCtrlFake, need to initialize stuff.");
		// fake the influence matrix:
		gsl_matrix *infl, *v;
		gsl_vector *sing, *work;
		
		infl = gsl_matrix_calloc(nsubap*2, nacttot);
		v = gsl_matrix_calloc(nacttot, nacttot);
		sing = gsl_vector_calloc(nacttot);
		work = gsl_vector_calloc(nacttot);
		
		inflf = gsl_matrix_float_calloc(nsubap*2, nacttot);
		vf = gsl_matrix_float_calloc(nacttot, nacttot);
		singf = gsl_vector_float_calloc(nacttot);
		workf = gsl_vector_float_calloc(nacttot);
		
		dispf = gsl_vector_float_calloc(nsubap*2);
		actf = gsl_vector_float_calloc(nacttot);
		
		for (i=0; i<nsubap*2; i++) 
			for (j=0; j<nacttot; j++)
				gsl_matrix_set(infl, i, j, drand48()*2-1);
		
		// SVD the influence matrix:		
		gsl_linalg_SV_decomp(infl, v, sing, work);
		
		// copy to float versions
		for (i=0; i<nsubap*2; i++) 
			for (j=0; j<nacttot; j++)
				gsl_matrix_float_set(inflf, i, j, gsl_matrix_get(infl, i, j));
				
		for (i=0; i<nacttot; i++) {
			for (j=0; j<nacttot; j++)
				gsl_matrix_float_set(vf, i, j, gsl_matrix_get(v, i, j));
					
			gsl_vector_float_set(singf, i, gsl_vector_get(sing, i));
			gsl_vector_float_set(workf, i, gsl_vector_get(work, i));
		}
		
		// init test displacement vector
		for (i=0; i<nsubap*2; i++)
			gsl_vector_float_set(dispf, i, drand48()*2-1);
		
		logInfo("Init done, starting SVD reconstruction stuff.");
		
		gsl_matrix_free(infl);
		gsl_matrix_free(v);
		gsl_vector_free(work);
		gsl_vector_free(sing);
	}
	
	
    gsl_blas_sgemv (CblasTrans, 1.0, inflf, dispf, 0.0, workf);

	int singvals=nacttot;
	float wi, alpha;
	
	for (i = 0; i < nacttot; i++) {
		wi = gsl_vector_float_get (workf, i);
		alpha = gsl_vector_float_get (singf, i);
		if (alpha != 0) {
			alpha = 1.0 / alpha;
			singvals--;
		}
		gsl_vector_float_set (workf, i, alpha * wi);
	}

    gsl_blas_sgemv (CblasNoTrans, 1.0, vf, workf, 0.0, actf);
	
	return EXIT_SUCCESS;
}


// TODO: document this
int drvFilterWheel(control_t *ptc, fwheel_t mode) {
	// in simulation this is easy, just set mode in ptc
	ptc->filter = mode;
	return EXIT_SUCCESS;
}
