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
	\li foam_modules-img.c - to read image files
	
*/

// HEADERS //
/***********/

#include "foam_primemod-simstatic.h"

// GLOBALS //
/***********/

float *simimgsurf=NULL;		// Global surface to draw on
SDL_Surface *screen;		// Global surface to draw on
SDL_Event event;			// Global SDL event struct to catch user IO
extern pthread_mutex_t mode_mutex;
extern pthread_cond_t mode_cond;


// ROUTINES //
/************/

int modInitModule(control_t *ptc) {
    // initialize video
    if (SDL_Init(SDL_INIT_VIDEO) == -1) 
		logErr("Could not initialize SDL: %s", SDL_GetError());
		
	atexit(SDL_Quit);

	SDL_WM_SetCaption("WFS output", "WFS output");

	screen = SDL_SetVideoMode(ptc->wfs[0].res.x, ptc->wfs[0].res.y, 0, SDL_HWSURFACE | SDL_DOUBLEBUF | SDL_RESIZABLE);
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
	
	modSelSubapts(ptc->wfs[0].image, ptc->wfs[0].res, ptc->wfs[0].cells, ptc->wfs[0].subc, ptc->wfs[0].gridc, &(ptc->wfs[0].nsubap), 0, -1);
	
	logDebug(0, "Res: (%d,%d), nsubap: %d, cells: (%d,%d), subc 0 and 1: (%d,%d) (%d,%d), gridc 0 and 1: (%d,%d) (%d,%d)", \
		ptc->wfs[0].res.x, ptc->wfs[0].res.y, ptc->wfs[0].nsubap, ptc->wfs[0].cells[0], ptc->wfs[0].cells[1], \
		ptc->wfs[0].subc[0][0], ptc->wfs[0].subc[0][1], \
		ptc->wfs[0].subc[1][0], ptc->wfs[0].subc[1][1], ptc->wfs[0].gridc[0][0], ptc->wfs[0].gridc[0][1], \
		ptc->wfs[0].gridc[1][0], ptc->wfs[0].gridc[1][1]);
	
	return EXIT_SUCCESS;
}

int modOpenLoop(control_t *ptc) {
	int i;
				
	if (drvReadSensor(ptc) != EXIT_SUCCESS) {		// read the sensor output into ptc.image
		logWarn("Error, reading sensor failed.");
		return EXIT_FAILURE;
	}

	if (modCalDarkFlat(ptc->wfs[0].image, ptc->wfs[0].darkim, ptc->wfs[0].flatim, ptc->wfs[0].corrim) != EXIT_SUCCESS)
		return EXIT_FAILURE;
	
	if (modParseSH(ptc->wfs[0].corrim, ptc->wfs[0].subc, ptc->wfs[0].gridc, ptc->wfs[0].nsubap, \
		ptc->wfs[0].track, ptc->wfs[0].disp, ptc->wfs[0].refc) != EXIT_SUCCESS)
		return EXIT_FAILURE;
		
	logDebug(LOG_SOMETIMES, "Frame: %ld", ptc->frames);
	if (ptc->frames % cs_config.logfrac == 0) 
		modDrawStuff(ptc, 0, screen);

	if (ptc->frames > FOAM_SIMSTATIC_MAXFRAMES) {
		ptc->frames = 0;
		ptc->mode = AO_MODE_LISTEN;
	}
		
		
	if (SDL_PollEvent(&event))
		switch (event.type) {
			case SDL_QUIT:
				stopFOAM();
				break;
			case SDL_VIDEORESIZE:
				screen = SDL_SetVideoMode(event.resize.w, event.resize.h, 0, SDL_HWSURFACE | SDL_DOUBLEBUF | SDL_RESIZABLE);
				break;
		}
			
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
	
	if (modCalDarkFlat(ptc->wfs[0].image, ptc->wfs[0].darkim, ptc->wfs[0].flatim, ptc->wfs[0].corrim) != EXIT_SUCCESS)
		return EXIT_FAILURE;
	
	if (modParseSH(ptc->wfs[0].corrim, ptc->wfs[0].subc, ptc->wfs[0].gridc, ptc->wfs[0].nsubap, \
		ptc->wfs[0].track, ptc->wfs[0].disp, ptc->wfs[0].refc) != EXIT_SUCCESS)
		return EXIT_FAILURE;
	
	if (modCalcCtrlFake(ptc, 0, 0) != EXIT_SUCCESS)	// process SH sensor output, get displacements
		return EXIT_FAILURE;	
		
	logDebug(LOG_SOMETIMES, "frame: %ld", ptc->frames);
	if (ptc->frames % cs_config.logfrac == 0) 
		modDrawStuff(ptc, 0, screen);
	
	if (ptc->frames > FOAM_SIMSTATIC_MAXFRAMES) {
		ptc->mode = AO_MODE_LISTEN;
	}
	

	
	if (SDL_PollEvent(&event))
		switch (event.type) {
			case SDL_QUIT:
				stopFOAM();
				break;
			case SDL_VIDEORESIZE:
				screen = SDL_SetVideoMode(event.resize.w, event.resize.h, 0, SDL_HWSURFACE | SDL_DOUBLEBUF | SDL_RESIZABLE);
				break;
		}

		
	return EXIT_SUCCESS;
}

int modCalibrate(control_t *ptc) {

	logInfo(0, "No calibration in static simulation mode");

	return EXIT_SUCCESS;
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
				pthread_cond_signal(&mode_cond);
				tellClients("200 OK CALIBRATE PINHOLE");
			}
			else if (strcmp(list[1],"lintest") == 0) {
				ptc->mode = AO_MODE_CAL;
				ptc->calmode = CAL_LINTEST;
				pthread_cond_signal(&mode_cond);
				tellClients("200 OK CALIBRATE LINTEST");
			}
			else if (strcmp(list[1],"influence") == 0) {
				ptc->mode = AO_MODE_CAL;
				ptc->calmode = CAL_INFL;
				pthread_cond_signal(&mode_cond);
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

int drvReadSensor() {
	int i, x, y;
	int simres[2];
	// logDebug(0, "Now reading %d sensors", ptc.wfs_count);
	
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
			if (modReadIMGArr(FOAM_SIMSTATIC_IMG, &simimgsurf, simres) != EXIT_SUCCESS)
				logErr("Cannot read static image.");

			if (simres[0] != res.x || simres[1] != res.y) {
				logWarn("Simulation resolution incorrect for %s! (%dx%d vs %dx%d)", \
					FOAM_SIMSTATIC_IMG, res.x, res.y, simres[0], simres[1]);
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
	int nacttot = FOAM_SIMSTATIC_NACT;
	
	// function assumes presence of dmmodes, singular and wfsmodes...
	if (inflf == NULL || vf == NULL) {
		logInfo(0, "First modCalcCtrlFake, need to initialize stuff.");
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
		
		logInfo(0, "Init done, starting SVD reconstruction stuff.");
		
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
