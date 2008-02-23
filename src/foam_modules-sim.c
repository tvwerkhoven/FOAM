/*! 
	@file foam_modules-sim.c
	@author @authortim
	@date November 30 2007

	@brief This file contains the functions to run @name in simulation mode, and 
	links to other files like foam_modules-dm.c which simulates the DM.
*/

// HEADERS //
/***********/

#include "foam_modules-sim.h"

#define FOAM_MODSIM_WAVEFRONT "../config/wavefront.fits"
#define FOAM_MODSIM_APERTURE "../config/apert15-256.pgm"
#define FOAM_MODSIM_APTMASK "../config/apert15-256.pgm"
#define FOAM_MODSIM_ACTPAT "../config/dm37-256.pgm"


struct simul {
	int wind[2]; 			// 'windspeed' in pixels/cycle
	int curorig[2]; 		// current origin
	float *simimg; 			// pointer to the image we use to simulate stuff
	int simimgres[2];		// resolution of the simulation image
	float seeingfac;		// factor to worsen seeing (2--20)
	fftw_complex *shin;		// input for fft algorithm
	fftw_complex *shout;	// output for fft (but shin can be used if fft is inplace)
	fftw_plan plan_forward; // plan, one time calculation on how to calculate ffts fastest
	char wisdomfile[32];
};

struct simul simparams = {
	.wind[0] = 10,
	.wind[1] = 5,
	.curorig[0] = 0,
	.curorig[1] = 0,
	.seeingfac = 3.,
	.simimg = NULL,
	.plan_forward = NULL,
	.wisdomfile = "fftw_wisdom.dat",
};

extern SDL_Surface *screen;	// Global surface to draw on
extern SDL_Event event;		// Global SDL event struct to catch user IO


// We need these every time, declare them globally
char errmsg[FLEN_STATUS]; 		// FLEN_STATUS is from fitsio.h
int status = 0;  				// MUST initialize status
float nulval = 0.0;
int anynul = 0;

int modInitModule(control_t *ptc) {

    /* Initialize defaults, Video and Audio */
    if((SDL_Init(SDL_INIT_VIDEO) == -1)) { 
        logErr("Could not initialize SDL: %s.\n", SDL_GetError());
		return EXIT_FAILURE;
    }
	atexit(SDL_Quit);
	
	SDL_WM_SetCaption("WFS output", "WFS output");

	screen = SDL_SetVideoMode(ptc->wfs[0].res[0], ptc->wfs[0].res[1], 0, SDL_HWSURFACE|SDL_DOUBLEBUF);
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
		Slock(screen);
		displayImg(ptc->wfs[0].image, ptc->wfs[0].res, screen);
		modDrawGrid(&(ptc->wfs[0]), screen);
		modDrawSubapts(&(ptc->wfs[0]), screen);
		modDrawVecs(&(ptc->wfs[0]), screen);
		Sulock(screen);
		SDL_Flip(screen);
		
//	}

	
	if (SDL_PollEvent(&event))
		if (event.type == SDL_QUIT)
			stopFOAM();
	return EXIT_SUCCESS;
}

int modClosedInit(control_t *ptc) {
	// this is the same for open and closed modes, don't rewrite stuff
	modOpenInit(ptc);
	return EXIT_SUCCESS;
}

int modClosedLoop(control_t *ptc) {
	// set both actuators to something random
	int i,j;
	
	for (i=0; i<ptc->wfc_count; i++) {
		logDebug("Setting WFC %d with %d acts.", i, ptc->wfc[i].nact);
		for (j=0; j<ptc->wfc[i].nact; j++)
			ptc->wfc[i].ctrl[j] = -1; drand48()*2-1;
	}
	ptc->wfc[1].ctrl[ptc->frames % 37] = 1;

	if (drvReadSensor(ptc) != EXIT_SUCCESS)			// read the sensor output into ptc.image
		return EXIT_SUCCESS;

	//modSelSubapts(&ptc.wfs[0], 0, 0); 			// check samini (2nd param) and samxr (3d param)				
	
	if (modParseSH((&ptc->wfs[0])) != EXIT_SUCCESS)			// process SH sensor output, get displacements
		return EXIT_SUCCESS;
	
//	if (ptc->frames % 20 == 0) {
	Slock(screen);
	displayImg(ptc->wfs[0].image, ptc->wfs[0].res, screen);
	modDrawGrid(&(ptc->wfs[0]), screen);
	modDrawSubapts(&(ptc->wfs[0]), screen);
	modDrawVecs(&(ptc->wfs[0]), screen);
	Sulock(screen);
	SDL_Flip(screen);
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
			return modCalPinhole(ptc, 0);
			break;
		case CAL_INFL: // influence matrix
			return modCalWFC(ptc, 0); // arguments: (control_t *ptc, int wfs)
			break;
		default:
			logErr("Unsupported calibrate mode encountered.");
			return EXIT_FAILURE;
			break;			
	}
}

int drvReadSensor() {
	int i;
	logDebug("Now reading %d sensors, origin is at (%d,%d).", ptc.wfs_count, simparams.curorig[0], simparams.curorig[1]);
	
	if (ptc.wfs_count < 1) {
		logErr("Nothing to process, no WFSs defined.");
		return EXIT_FAILURE;
	}
	
	// if filterwheel is set to pinhole, simulate a coherent image
	if (ptc.mode == AO_MODE_CAL && ptc.filter == FILT_PINHOLE) {
		for (i=0; i<ptc.wfs[0].res[0]*ptc.wfs[0].res[1]; i++)
			ptc.wfs[0].image[i] = 1;
			
	}
	else {
		// This reads in wavefront.fits to memory at the first call, and each consecutive call
		// selects a subimage of that big image, defined by simparams.curorig
		if (simAtm(FOAM_MODSIM_WAVEFRONT, ptc.wfs[0].res, simparams.curorig, ptc.wfs[0].image) != EXIT_SUCCESS) { 	
			if (status > 0) {
				fits_get_errstatus(status, errmsg);
				logErr("fitsio error in simAtm(): (%d) %s", status, errmsg);
				status = 0;
				return EXIT_FAILURE;
			}
			else logErr("error in simAtm().");
		}
		// This function simulates wind by changing the origin we want read at simAtm().
		// TODO: incorporate in simAtm?
		modSimWind();
		
		logDebug("simAtm() done");
	} // end for (ptc.filter == FILT_PINHOLE)


	// we simulate WFCs before the telescope to make sure they outer region is zero (Which is done by simTel())
	logDebug("Now simulating %d WFC(s).", ptc.wfc_count);
	for (i=0; i < ptc.wfc_count; i++)
		simWFC(&ptc, i, ptc.wfc[i].nact, ptc.wfc[i].ctrl, ptc.wfs[0].image); // Simulate every WFC in series
	
	Slock(screen);
	displayImg(ptc.wfs[0].image, ptc.wfs[0].res, screen);
	modDrawGrid(&(ptc.wfs[0]), screen);
	Sulock(screen);
	SDL_Flip(screen);
	sleep(1);
	if (simTel(FOAM_MODSIM_APERTURE, ptc.wfs[0].image, ptc.wfs[0].res) != EXIT_SUCCESS) { // Simulate telescope (from aperture.fits)
			logErr("error in simTel().");
	}
	
	// displayImg(ptc.wfs[0].image, ptc.wfs[0].res, screen);
	
	// Simulate the WFS here.
	if (modSimSH() != EXIT_SUCCESS) {
		logDebug("Simulating SH WFSs failed.");
		return EXIT_FAILURE;
	}	

	// Slock(screen);
	// displayImg(ptc.wfs[0].image, ptc.wfs[0].res, screen);
	// modDrawGrid(&(ptc.wfs[0]), screen);
	// Sulock(screen);
	// SDL_Flip(screen);
	// sleep(1);	
	
	return EXIT_SUCCESS;
}

int modSimWind() {
	if (simparams.simimgres[0] < ptc.wfs[0].res[0] + 2*simparams.wind[0]) {
		logErr("Simulated wavefront too small for current x-wind, setting to zero.");
		simparams.wind[0] = 0;
	}
		
	if (simparams.simimgres[1] < ptc.wfs[0].res[1] + 2*simparams.wind[1]) {
		logErr("Simulated wavefront too small for current y-wind, setting to zero.");
		simparams.wind[1] = 0;
	}
	
	// Apply 'wind' to the simulated wavefront
	simparams.curorig[0] += simparams.wind[0];
	simparams.curorig[1] += simparams.wind[1];

 	// if the origin is out of bound, reverse the wind direction and move that way
	// X ORIGIN TOO BIG:
	if (simparams.wind[0] != 0 && simparams.curorig[0] > simparams.simimgres[0]-ptc.wfs[0].res[0]) {
		simparams.wind[0] *= -1;						// Reverse X wind speed
		simparams.curorig[0] += 2*simparams.wind[0];	// move in the new wind direction
	}
	// X ORIGIN TOO SMALL
	if (simparams.wind[0] != 0 && simparams.curorig[0] < 0) {
		simparams.wind[0] *= -1;						// Reverse X wind speed
		simparams.curorig[0] += 2*simparams.wind[0];	// move in the new wind direction
	}
	
	// Y ORIGIN TOO BIG
	if (simparams.wind[1] != 0 && simparams.curorig[1] > simparams.simimgres[1]-ptc.wfs[0].res[1]) {
		simparams.wind[1] *= -1;						// Reverse Y wind speed
		simparams.curorig[1] += 2*simparams.wind[1];	// move in the new wind direction
	}
	// Y ORIGIN TOO SMALL
	if (simparams.wind[1] != 0 && simparams.curorig[1] < 0) {
		simparams.wind[1] *= -1;						// Reverse Y wind speed
		simparams.curorig[1] += 2*simparams.wind[1];	// move in the new wind direction
	}
	
	return EXIT_SUCCESS;
}

int simObj(char *file, float *image) {
	return EXIT_SUCCESS;
}

int simWFC(control_t *ptc, int wfcid, int nact, float *ctrl, float *image) {
	// we want to simulate the tip tilt mirror here. What does it do
	logDebug("WFC %d (%s) has %d actuators, simulating", wfcid, ptc->wfc[wfcid].name, ptc->wfc[wfcid].nact);
	if (wfcid == 0)
		modSimTT(ctrl, image, ptc->wfs[0].res);
	if (wfcid == 1) {
		logDebug("Running modSimDM with %s and %s, nact %d", FOAM_MODSIM_APTMASK, FOAM_MODSIM_ACTPAT, nact);
		modSimDM(FOAM_MODSIM_APTMASK, FOAM_MODSIM_ACTPAT, nact, ctrl, image, ptc->wfs[0].res, -1); // last arg is for niter. -1 for autoset
	}
					
	return EXIT_SUCCESS;
}

// global to hold telescope aperture
float *aperture=NULL;

int simTel(char *file, float *image, int res[2]) {
	int i;
	int x, y;

	// we read the file in here (only pgm now)
//	modReadPGM(file, &aperture, &nx, &ny, &ngray);
	
	SDL_Surface *aperturesurf;
	// read boundary mask file 
	// float *aperture is globally defined and initialized to NULL.
	if (aperture == NULL) {
		if (modReadPGM(file, &aperturesurf) != EXIT_SUCCESS) {
			logErr("Cannot read telescope aperture");
			return EXIT_FAILURE;
		}
		if (res[0] != aperturesurf->w || res[1] != aperturesurf->h) {
			logErr("Telescope aperture resolution incorrect! (%dx%d vs %dx%d)", res[0], res[1], aperturesurf->w, aperturesurf->h);
			return EXIT_FAILURE;
		}
		// copy from SDL_Surface to array
		aperture = calloc(aperturesurf->w *aperturesurf->h, sizeof(*aperture));
		if (aperture == NULL) {
			logErr("Error allocating memory for aperture image");
			return EXIT_FAILURE;
		}
		else {
			for (y=0; y<aperturesurf->h; y++)
				for (x=0; x<aperturesurf->w; x++)
					aperture[y*aperturesurf->w + x] = (float) getpixel(aperturesurf, x, y);
		}
	}
	
	logDebug("Aperture read successfully (%dx%d), processing with image.",  res[0], res[1]);
	
	// Multiply wavefront with aperture
	for (i=0; i < res[0]*res[1]; i++)
	 	if (aperture[i] == 0) image[i] = 0;
	
	return EXIT_SUCCESS;
}

int simAtm(char *file, int res[2], int origin[2], float *image) {
	// ASSUMES WFS_COUNT > 0
	// ASSUMES ptc.wfs[0].image is initialized to float 
	// ASSUMES simparams.simimg is also float
	
	fitsfile *fptr;
	int bitpix, naxis;
	int i,j;
	long fpixel[] = {1,1};
	long naxes[2] = {1,1};
	char newfile[strlen(file)+32];
//	double sum;
	
	logDebug("Simulating atmosphere.");
	
	// If we haven't loaded the simulated wavefront yet, load it now
	if (simparams.simimg == NULL) {
		// Loading was different, loaded only part of a file, slower.
		// sprintf(newfile, "%s[%ld:%ld,%ld:%ld]", file, origin[0], origin[0]+res[0]-1, origin[1], origin[1]+res[1]-1);
		
		// THIS IS A TEMPORARY THING
		// limit the size of the file read, only read 512x512 instead of full 1024x1024 img
		//sprintf(newfile, "%s[%ld:%ld,%ld:%ld]", file, origin[0], origin[0]+512-1, origin[1], origin[1]+512-1);
	
		sprintf(newfile, "%s", file);
		
		logDebug("Loading image from fitsfile %s", newfile);
		fits_open_file(&fptr, newfile, READONLY, &status);			// Open FITS file
		if (status > 0)
			return EXIT_FAILURE;

		fits_get_img_param(fptr, 2,  &bitpix, \
							&naxis, naxes, &status);				// Read header
		if (status > 0)
			return EXIT_FAILURE;
		
		if (naxis != 2) {
			logErr("Number of axes is not two, a one layer 2d fits file is needed.");
			return EXIT_FAILURE;
		}
		else {
			simparams.simimg = calloc(naxes[0] * naxes[1], sizeof(*simparams.simimg));
//			simparams.simimg = calloc(512 * 512, sizeof(simparams.simimg));
			if (simparams.simimg == NULL) {
				logErr("Allocating memory for simul.simimg failed.");
				return EXIT_FAILURE;
			}
			logDebug("Allocated memory for simparams.simimg (%d x %d x %d byte).", 
				naxes[0], naxes[1], sizeof(simparams.simimg));
			
			simparams.simimgres[0] = naxes[0];
			simparams.simimgres[1] = naxes[1];
		}
		
		logDebug("Now reading image with size (%dx%d) from file %s.", naxes[0], naxes[1], newfile);
		fits_read_pix(fptr, TFLOAT, fpixel, \
							naxes[0]*naxes[1], &nulval, simparams.simimg, \
							&anynul, &status);						// Read fitsfile to image
		if (status > 0)
			return EXIT_FAILURE;

		logDebug("Read image, status: %d bitpix: %d, pixel (0,0): %f", status, \
			bitpix, simparams.simimg[0]);
		fits_close_file(fptr, &status);
	}
	
	// If we move outside the simulated wavefront, reverse the 'windspeed'
	// TODO: this is already handled in modSimWind? should not be necessary?
	if ((origin[0] + res[0] >= simparams.simimgres[0]) || (origin[1] + res[1] >= simparams.simimgres[1]) 
		|| (origin[0] < 0) || (origin[1] < 0)) {
		logErr("Simulation out of bound, wind reset failed. (%f,%f) ", origin[0], origin[1]);
		return EXIT_FAILURE;
	}
	
	for (i=0; i<res[1]; i++) { // y coordinate
		for (j=0; j<res[0]; j++) { // x coordinate 
			image[i*res[0] + j] = simparams.simimg[(i+origin[1])*simparams.simimgres[0] + (j+origin[0])];
		}
	}	

	return EXIT_SUCCESS;
}

int drvSetActuator(control_t *ptc, int wfc) {
	// this is a placeholder, since we simulate the DM, we don't need to do 
	// anything here. 
	return EXIT_SUCCESS;
}

int modSimSH() {
	logDebug("Simulating SH WFSs now.");
	
	int i;
	int nx, ny;
	int xc, yc;
	int jp, ip;
	FILE *fp;
	
	double tmp;

	// we have ptc.wfs[0].cells[0] by ptc.wfs[0].cells[1] SH cells, so the size of each cell is:
	int *shsize = ptc.wfs[0].shsize;
	
	nx = (shsize[0] * 2);
	ny = (shsize[1] * 2);

	// init data structures for images, fill with zeroes (no calloc here?)
	if (simparams.shin == NULL) {
		simparams.shin = fftw_malloc ( sizeof ( fftw_complex ) * nx * ny );
		for (i=0; i< nx*ny; i++)
			simparams.shin[i][0] = simparams.shin[i][1] = 0;
	}
	
	if (simparams.shout == NULL) {
		simparams.shout = fftw_malloc ( sizeof ( fftw_complex ) * nx * ny );
		for (i=0; i< nx*ny; i++)
			simparams.shout[i][0] = simparams.shout[i][1] = 0;
	}
	
	// init FFT plan, FFTW_ESTIMATE could be replaces by MEASURE or sth, which should calculate FFT's faster
	if (simparams.plan_forward == NULL) {
		logDebug("Setting up plan for fftw");
		
		fp = fopen(simparams.wisdomfile,"r");
		if (fp == NULL)	{				// File does not exist, generate new plan
			logInfo("No FFTW plan found in %s, generating new plan, this may take a while.", simparams.wisdomfile);
			simparams.plan_forward = fftw_plan_dft_2d(nx, ny, simparams.shin, simparams.shout, FFTW_FORWARD, FFTW_EXHAUSTIVE);

			// Open file for writing now
			fp = fopen(simparams.wisdomfile,"w");
			if (fp == NULL) {
				logDebug("Could not open file %s for saving FFTW wisdom.", simparams.wisdomfile);
				return EXIT_FAILURE;
			}
			fftw_export_wisdom_to_file(fp);
			fclose(fp);
		}
		else {
			logInfo("Importing FFTW wisdom file.");
			logInfo("If this is the first time this program runs on this machine, this is bad.");
			logInfo("In that case, please delete '%s' and rerun the program. It will generate new wisdom which is A Good Thing.", \
				simparams.wisdomfile);
			if (fftw_import_wisdom_from_file(fp) == 0) {
				logDebug("Importing wisdom failed.");
				return EXIT_FAILURE;
			}
			// regenerating plan using wisdom imported above.
			simparams.plan_forward = fftw_plan_dft_2d(nx, ny, simparams.shin, simparams.shout, FFTW_FORWARD, FFTW_EXHAUSTIVE);
			fclose(fp);
		}
		
	}
		
	if (simparams.shout == NULL || simparams.shin == NULL || simparams.plan_forward == NULL) {
		logDebug("Allocation of memory for FFT failed.");
		return EXIT_FAILURE;
	}
	
	if (ptc.wfs[0].cells[0] * shsize[0] != ptc.wfs[0].res[0] || \
		ptc.wfs[0].cells[1] * shsize[1] != ptc.wfs[0].res[1])
		logErr("Incomplete SH cell coverage! This means that nx_subapt * width_subapt != width_img x: (%d*%d,%d) y: (%d*%d,%d)", \
			ptc.wfs[0].cells[0], shsize[0], ptc.wfs[0].res[0], ptc.wfs[0].cells[1], shsize[1], ptc.wfs[0].res[1]);

	
	logDebug("Beginning imaging simulation.");

	// now we loop over each subaperture:
	for (yc=0; yc<ptc.wfs[0].cells[1]; yc++) {
		for (xc=0; xc<ptc.wfs[0].cells[0]; xc++) {
			// we're at subapt (xc, yc) here...
			
			// possible approaches on subapt selection for simulation:
			//  - select only central apertures (use radius)
			//  - use absolute intensity (still partly illuminated apts)
			//  - count pixels with intensity zero
			i = 0;
			for (ip=0; ip<shsize[1]; ip++)
				for (jp=0; jp<shsize[0]; jp++)
					 	if (ptc.wfs[0].image[yc*shsize[1]*ptc.wfs[0].res[0] + xc*shsize[0] + ip*ptc.wfs[0].res[0] + jp] == 0)
							i++;
			
			// allow one quarter of the pixels to be zero, otherwise set subapt to zero and continue
			// TODO: double loop (this one and above) ugly?
			if (i > shsize[1]*shsize[0]/4) {
				for (ip=0; ip<shsize[1]; ip++)
					for (jp=0; jp<shsize[0]; jp++)
						ptc.wfs[0].image[yc*shsize[1]*ptc.wfs[0].res[0] + xc*shsize[0] + ip*ptc.wfs[0].res[0] + jp] = 0;
								
				continue;
			}
			
			// We want to set the helper arrays to zero first
			// otherwise we get trouble? TODO: check this out
			for (i=0; i< nx*ny; i++)
				simparams.shin[i][0] = simparams.shin[i][1] = 0;
			for (i=0; i< nx*ny; i++)
				simparams.shout[i][0] = simparams.shout[i][1] = 0;
			
			// add markers to track copying:
			for (i=0; i< 2*nx; i++)
				simparams.shin[i][0] = 1;

			for (i=0; i< ny; i++)
				simparams.shin[nx/2+i*nx][0] = 1;

			// loop over all pixels in the subaperture, copy them to subapt:
			// I'm pretty sure these index gymnastics are correct (2008-01-18)
			for (ip=0; ip<shsize[1]; ip++) { 
				for (jp=0; jp<shsize[0]; jp++) {
					// we need the ipth row PLUS the rows that we skip at the top (shsize[1]/2+1)
					// the width is not shsize[0] but twice that plus 2, so mulyiply the row number
					// with that. Then we need to add the column number PLUS the margin at the beginning 
					// which is shsize[0]/2 + 1, THAT's the right subapt location.
					// For the image itself, we need to skip to the (ip,jp)th subaperture,
					// which is located at pixel yc * the height of a cell * the width of the picture
					// and the x coordinate times the width of a cell time, that's at least the first
					// subapt pixel. After that we add the subaperture row we want which is located at
					// pixel ip * the width of the whole image plus the x coordinate

					simparams.shin[(ip+ ny/4)*nx + (jp + nx/4)][0] = \
						ptc.wfs[0].image[yc*shsize[1]*ptc.wfs[0].res[0] + xc*shsize[0] + ip*ptc.wfs[0].res[0] + jp];
					// old: simparams.shin[(ip+ shsize[1]/2 +1)*nx + jp + shsize[0]/2 + 1][0] = 
				}
			}
			
			// now image the subaperture, first generate EM wave amplitude
			// this has to be done using complex numbers over the BIG subapt
			// we know that exp ( i * phi ) = cos(phi) + i sin(phi),
			// so we split it up in a real and a imaginary part
			// TODO dit kan hierboven al gedaan worden
			for (ip=shsize[1]/2; ip<shsize[1] + shsize[1]/2; ip++) {
				for (jp=shsize[0]/2; jp<shsize[0]+shsize[0]/2; jp++) {
					tmp = simparams.seeingfac * simparams.shin[ip*nx + jp][0]; // multiply for worse seeing
					//use fftw_complex datatype, i.e. [0] is real, [1] is imaginary
					
					// SPEED: cos and sin are SLOW, replace by taylor series
					// optimilization with parabola, see http://www.devmaster.net/forums/showthread.php?t=5784
					// and http://lab.polygonal.de/2007/07/18/fast-and-accurate-sinecosine-approximation/
					// wrap tmp to (-pi, pi):
					//tmp -= ((int) ((tmp+3.14159265)/(2*3.14159265))) * (2* 3.14159265);
					//simparams.shin[ip*nx + jp][1] = 1.27323954 * tmp -0.405284735 * tmp * fabs(tmp);
					// wrap tmp + pi/2 to (-pi,pi) again, but we know tmp is already in (-pi,pi):
					//tmp += 1.57079633;
					//tmp -= (tmp > 3.14159265) * (2*3.14159265);
					//simparams.shin[ip*nx + jp][0] = 1.27323954 * tmp -0.405284735 * tmp * fabs(tmp);
					
					// used to be:
					simparams.shin[ip*nx + jp][1] = sin(tmp);
					simparams.shin[ip*nx + jp][0] = cos(tmp);
				}
			}

			// now calculate the  FFT, pts is the number of (complex) datapoints
			fftw_execute ( simparams.plan_forward );

			// now calculate the absolute squared value of that, store it in the subapt thing
			for (ip=0; ip<ny; ip++)
				for (jp=0; jp<nx; jp++)
					simparams.shin[ip*nx + jp][0] = \
					 fabs(pow(simparams.shout[ip*nx + jp][0],2) + pow(simparams.shout[ip*nx + jp][1],2));
			
			// copy subaparture back to main images
			// note: we don't want the center of the image, but we want all corners
			// because begins in the origin. Therefore we need to start at coordinates
			//  nx-(nx_subapt/2), ny-(ny_subapt/2)
			// e.g. for 32x32 subapts and (nx,ny) = (64,64), we start at
			//  (48,48) -> (70,70) = (-16,-16)
			// so we need to wrap around the matrix, which results in the following
			// if (ip,jp) starts at (0,0)
			for (ip=ny/4; ip<ny*3/4; ip++) { 
				for (jp=nx/4; jp<nx*3/4; jp++) {
					ptc.wfs[0].image[yc*shsize[1]*ptc.wfs[0].res[0] + xc*shsize[0] + (ip-ny/4)*ptc.wfs[0].res[0] + (jp-nx/4)] = \
						simparams.shin[((ip+ny/2)%ny)*nx + (jp+nx/2)%nx][0];
				}
			}
			
		} 
	} // end looping over subapts
	logDebug("Image simulation done.");	
	//manually copy one subapt to check the alignment and shit
	
	
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

Uint32 getpixel(SDL_Surface *surface, int x, int y) {
    int bpp = surface->format->BytesPerPixel;
    /* Here p is the address to the pixel we want to retrieve */
    Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;
    switch(bpp) {
    case 1:
        return *p;
    case 2:
        return *(Uint16 *)p;
    case 3:
        if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
            return p[0] << 16 | p[1] << 8 | p[2];
        else
            return p[0] | p[1] << 8 | p[2] << 16;
    case 4:
        return *(Uint32 *)p;
    default:
        return 0;       /* shouldn't happen, but avoids warnings */
    }
}

int modReadPGM(char *fname, SDL_Surface **img) {
	
	*img = IMG_Load(fname);
	if (!img) {
		logErr("Error in IMG_Load: %s\n", IMG_GetError());
		return EXIT_FAILURE;
	}
	
	logDebug("Loaded image %s (%dx%d)", fname, (*img)->w, (*img)->h);

	return EXIT_SUCCESS;
}



// int modReadPGM(char *fname, double **dbuf, int *t_nx, int *t_ny, int *t_ngray) {
// 	char		c_int, first_string[110];
// 	unsigned char	b_in;
// 	int		i, j, bin_ind, nx=0, ny=0, ngray;
// 	long		ik;
// 	double		fi;
// 	FILE		*fp;
// 
// 	logDebug("modReadPGM reading %s", fname);
// 	if ((fp = fopen(fname,"r")) == NULL) {
// 	    logErr("Error opening pgm file %s!", fname);
// 		return EXIT_FAILURE;
// 	}
// 
// 	// Read magic number
// 
// 	i = 0;
// 	while((c_int = getc(fp)) != '\n') {
// 	    first_string[i] = c_int;
// 	    i++;
// 	    if(i > 100) i = 100;
// 	}
// 	first_string[i] = 0;
// 	// TvW null char \0
// 	
// 	// strcmp, moet op positie 0 staan
// 	if((strstr(first_string, "P2")) != NULL ) {
// 	  /*	    fprintf(stderr,
// 		    "\tPortable ASCII graymap aperture mask detected \n"); */
// 	    bin_ind = 0;
// 	} else if((strstr(first_string, "P5")) != NULL ){
// 	    //logDebug("Portable binary graymap aperture mask detected."); 
// 	    bin_ind = 1;
// 	} else {
// 	    logErr("Unknown magic in pgm file: %s, should be P2 or P5",first_string);
// 		return EXIT_FAILURE;
// 	}
// 	
// /*
//  *	Skip comments which start with a "#" and read the picture dimensions
//  */
// 
// l1:
// 	i = 0;
// 	while ((c_int = getc(fp)) != '\n') {
// 		first_string[i] = c_int;
// 		i++;
// 		if (i > 100) i = 100;
// 	}
// 	first_string[i] = 0;
// 	
// 	if (first_string[0] == '#')
// 		goto l1;
// 	else
// 		sscanf(first_string, "%d %d", &nx, &ny);
// 	
// 	logDebug("string 1: %s in %s", first_string, fname);
// 		/*  	fprintf(stderr, "\tX and Y dimensions: %d %d\n", nx, ny); */
// 	*t_nx = nx;
// 	*t_ny = ny;
// 
// /*
// 	*	Read the number of grayscales 
// */
// 
// 	i = 0;
// 	while((c_int=getc(fp)) != '\n') {
// 		first_string[i] = c_int;
// 		i++;
// 		if(i > 100) i = 100;
// 	}
// 	first_string[i] = 0;
// 	sscanf(first_string, "%d", &ngray);
// 		/*	fprintf(stderr, "\tNumber of gray levels:\t%d\n", ngray); */
// 	*t_ngray = ngray;
// 
// /*
// 	*	Read in graylevel data
// */
// 
// 	*dbuf = (double *) calloc(nx*ny, sizeof(double));
// 	if(*dbuf == NULL){
// 		logErr("Buffer allocation error!");
// 		return EXIT_FAILURE;
// 	}
// 	logDebug("%p is %d,%d big, storing %s", *dbuf, nx,ny, fname);
// 
// 	ik = 0;
// 	for (i = 1; i <= nx; i += 1) {
// 		for (j = 1; j <= ny; j += 1) {
// 			if (bin_ind) {
// 				if (fread (&b_in, sizeof(unsigned char), 1, fp) != 1) {
// 					logErr("Error reading portable bitmap!");
// 					return EXIT_FAILURE;
// 				}
// 				fi = (double) b_in;
// 			} else {
// 				if ((fscanf(fp,"%le",&fi)) == EOF){
// 					logErr("End of input file reached!");
// 					return EXIT_FAILURE;
// 				}
// 			}
// 			*(*dbuf + ik) = fi;
// 			ik ++;
// 		}  
// 	}
// 
// 	fclose(fp);
// 	return EXIT_SUCCESS;
// 	
// }