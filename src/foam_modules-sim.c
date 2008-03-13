/*! 
	@file foam_modules-sim.c
	@author @authortim
	@date November 30 2007

	@brief This file contains the functions to run @name in simulation mode.
	
	\section Info
	This module provides some functions to run an AO system in simulation mode.
	
	\section Functions

	The functions provided to the outside world are:
	\li drvFilterWheel() - Simulates setting a filterwheel to a certain position
	\li drvReadSensor() - Simulates the readout of a wavefront sensor. This function in turn 
	\li modSimSH() - Simulate a Shack Hartmann lenslet array (i.e. transform from wavefront space to image space)

	\section Dependencies
	
	This module depends on:
	\li foam_modules-display.c - to display the output
	\li foam_modules-dm.c - to simulate TT and DM WFC's
	\li foam_modules-calib.c - to calibrate the simulated WFC and TT mirror
	
	\section License
	This code is licensed under the GPL, version 2.
*/

// HEADERS //
/***********/

#include "foam_modules-sim.h"

#define FOAM_MODSIM_WAVEFRONT "../config/wavefront.pgm"
#define FOAM_MODSIM_APERTURE "../config/apert15-256.pgm"
#define FOAM_MODSIM_APTMASK "../config/apert15-256.pgm"
#define FOAM_MODSIM_ACTPAT "../config/dm37-256.pgm"

// GLOBALS //
/***********/

struct simul simparams = {
	.wind[0] = 0,
	.wind[1] = 0,
	.curorig[0] = 0,
	.curorig[1] = 0,
	.seeingfac = 0.3,
	.simimg = NULL,
	.plan_forward = NULL,
	.wisdomfile = "fftw_wisdom.dat",
};

// temporary for TT correction calibration
FILE *ttfd;

// global to hold telescope aperture
float *aperture=NULL;

// get this from somewhere else (debug)
// extern SDL_Surface *screen;

// ROUTINES //
/************/

int drvReadSensor() {
	int i;
	logDebug("Now reading %d sensors, origin is at (%d,%d).", ptc.wfs_count, simparams.curorig[0], simparams.curorig[1]);
	
	if (ptc.wfs_count < 1) 
		logErr("Nothing to process, no WFSs defined.");
	
	// if filterwheel is set to pinhole, simulate a coherent image
	if (ptc.mode == AO_MODE_CAL && ptc.filter == FILT_PINHOLE) {
		for (i=0; i<ptc.wfs[0].res.x*ptc.wfs[0].res.y; i++)
			ptc.wfs[0].image[i] = 1;
			
	}
	else {
		// This reads in wavefront.pgm (FOAM_MODSIM_WAVEFRONT) to memory at the first call, and each consecutive call
		// selects a subimage of that big image, defined by simparams.curorig
		if (simAtm(FOAM_MODSIM_WAVEFRONT, ptc.wfs[0].res, simparams.curorig, ptc.wfs[0].image) != EXIT_SUCCESS) 
			logErr("Error in simAtm().");

		modSimWind();
		
		logDebug("simAtm() done");
	}
	
	// modDrawStuff(&ptc, 0, screen);
	
	// sleep(1);
	
	// WE APPLY DM OR TT ERRORS HERE, 
	// AND TEST IF THE WFC'S CAN CORRECT IT
	///////////////////////////////////////

	gsl_vector_float *tmpctrl;
	tmpctrl = gsl_vector_float_calloc(ptc.wfc[0].nact);
	
	// regular sawtooth drift is here:
	for (i=0; i<ptc.wfc[0].nact; i++) {
		gsl_vector_float_set(tmpctrl, i, ((ptc.frames % 40)/40.0 * 2 - 1) );
		// * ( round( (ptc.frames % 40)/40.0 )*2 - 1)
	}

	// logDebug("TT: faking tt with : %f, %f", gsl_vector_float_get(tmpctrl, 0), gsl_vector_float_get(tmpctrl, 1));
	// if (ttfd == NULL) ttfd = fopen("ttdebug.dat", "w+");
	// fprintf(ttfd, "%f, %f\n", gsl_vector_float_get(tmpctrl, 0), gsl_vector_float_get(tmpctrl, 1));
	
	//	modSimTT(tmpctrl, ptc.wfs[0].image, ptc.wfs[0].res);
	modSimDM(FOAM_MODSIM_APTMASK, FOAM_MODSIM_ACTPAT, ptc.wfc[0].nact, tmpctrl, ptc.wfs[0].image, ptc.wfs[0].res, -1); // last arg is for niter. -1 for autoset

	// END OF FAKING ERRORS
	///////////////////////
	
	// we simulate WFCs before the telescope to make sure they outer region is zero (Which is done by simTel())
	logDebug("Now simulating %d WFC(s).", ptc.wfc_count);
	// for (i=0; i < ptc.wfc_count; i++)
	// 	simWFC(&ptc, i, ptc.wfc[i].nact, ptc.wfc[i].ctrl, ptc.wfs[0].image); // Simulate every WFC in series
	
	if (simTel(FOAM_MODSIM_APERTURE, ptc.wfs[0].image, ptc.wfs[0].res) != EXIT_SUCCESS) // Simulate telescope (from aperture.fits)
		logErr("error in simTel().");
	
	// Simulate the WFS here.
	if (modSimSH() != EXIT_SUCCESS) {
		logWarn("Simulating SH WFSs failed.");
		return EXIT_FAILURE;
	}	

	// modStuff(ptc, 0, screen);
	
	return EXIT_SUCCESS;
}

int modSimWind() {
	if (simparams.simimgres[0] < ptc.wfs[0].res.x + 2*simparams.wind[0]) {
		logWarn("Simulated wavefront too small for current x-wind, setting to zero.");
		simparams.wind[0] = 0;
	}
		
	if (simparams.simimgres[1] < ptc.wfs[0].res.y + 2*simparams.wind[1]) {
		logWarn("Simulated wavefront too small for current y-wind, setting to zero.");
		simparams.wind[1] = 0;
	}
	
	// Apply 'wind' to the simulated wavefront
	simparams.curorig[0] += simparams.wind[0];
	simparams.curorig[1] += simparams.wind[1];

 	// if the origin is out of bound, reverse the wind direction and move that way
	// X ORIGIN TOO BIG:
	if (simparams.wind[0] != 0 && simparams.curorig[0] + ptc.wfs[0].res.x >= simparams.simimgres[0]) {
		simparams.wind[0] *= -1;						// Reverse X wind speed
		simparams.curorig[0] += 2*simparams.wind[0];	// move in the new wind direction
	}
	// X ORIGIN TOO SMALL
	if (simparams.wind[0] != 0 && simparams.curorig[0] < 0) {
		simparams.wind[0] *= -1;						// Reverse X wind speed
		simparams.curorig[0] += 2*simparams.wind[0];	// move in the new wind direction
	}
	
	// Y ORIGIN TOO BIG
	if (simparams.wind[1] != 0 && simparams.curorig[1] + ptc.wfs[0].res.y >= simparams.simimgres[1]) {
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

int simWFC(control_t *ptc, int wfcid, int nact, gsl_vector_float *ctrl, float *image) {
	// we want to simulate the tip tilt mirror here. What does it do
	
	logDebug("WFC %d (%s) has %d actuators, simulating", wfcid, ptc->wfc[wfcid].name, ptc->wfc[wfcid].nact);
	// logDebug("TT: Control is: %f, %f", gsl_vector_float_get(ctrl,0), gsl_vector_float_get(ctrl,1));
	// if (ttfd == NULL) ttfd = fopen("ttdebug.dat", "w+");
	// fprintf(ttfd, "%f, %f\n", gsl_vector_float_get(ctrl,0), gsl_vector_float_get(ctrl,1));

	if (ptc->wfc[wfcid].type == WFC_TT)
		modSimTT(ctrl, image, ptc->wfs[0].res);
	else if (ptc->wfc[wfcid].type == WFC_DM) {
		logDebug("Running modSimDM with %s and %s, nact %d", FOAM_MODSIM_APTMASK, FOAM_MODSIM_ACTPAT, nact);
		modSimDM(FOAM_MODSIM_APTMASK, FOAM_MODSIM_ACTPAT, nact, ctrl, image, ptc->wfs[0].res, -1); // last arg is for niter. -1 for autoset
	}
	else {
		logWarn("Unknown WFC (%d) encountered (not TT or DM, type: %d, name: %s)", wfcid, ptc->wfc[wfcid].type, ptc->wfc[wfcid].name);
		return EXIT_FAILURE;
	}
					
	return EXIT_SUCCESS;
}


int simTel(char *file, float *image, coord_t res) {
	int i;
	int aptres[2];
	
	// read boundary mask file 
	// float *aperture is globally defined and initialized to NULL.
	if (aperture == NULL) {
		if (modReadPGMArr(file, &aperture, aptres) != EXIT_SUCCESS)
			logErr("Cannot read telescope aperture");

		if (res.x != aptres[0] || res.y != aptres[1])
			logErr("Telescope aperture resolution incorrect! (%dx%d vs %dx%d)", res.x, res.y, aptres[0], aptres[1]);

	}
	
	logDebug("Aperture read successfully (%dx%d), processing with image.",  res.x, res.y);
	
	// Multiply wavefront with aperture
	for (i=0; i < res.x*res.y; i++)
	 	if (aperture[i] == 0) image[i] = 0;
	
	return EXIT_SUCCESS;
}

int simAtm(char *file, coord_t res, int origin[2], float *image) {
	// ASSUMES WFS_COUNT > 0
	// ASSUMES ptc.wfs[0].image is initialized to float and memory is allocated
	// ASSUMES simparams.simimg is also float
	
	int i,j;
	int imgres[2];
	
	logDebug("Simulating atmosphere.");
	
	// If we haven't loaded the simulated wavefront yet, load it now
	if (simparams.simimg == NULL) {
		if (modReadPGMArr(file, &(simparams.simimg), imgres) != EXIT_SUCCESS)
			return EXIT_FAILURE;
		
		simparams.simimgres[0] = imgres[0];
		simparams.simimgres[1] = imgres[1];
	}	
	
	// If we move outside the simulated wavefront, reverse the 'windspeed'
	// TODO: this is already handled in modSimWind? should not be necessary?
	if ((origin[0] + res.x >= simparams.simimgres[0]) || (origin[1] + res.y >= simparams.simimgres[1]) 
		|| (origin[0] < 0) || (origin[1] < 0)) {
		logWarn("Simulation out of bound, wind reset failed. (%f,%f) ", origin[0], origin[1]);
		return EXIT_FAILURE;
	}
	
	for (i=0; i<res.y; i++) { // y coordinate
		for (j=0; j<res.x; j++) { // x coordinate 
			image[i*res.x + j] = simparams.simimg[(i+origin[1])*simparams.simimgres[0] + (j+origin[0])];
		}
	}	

	return EXIT_SUCCESS;
}

int drvSetActuator(control_t *ptc, int wfc) {
	// this is a placeholder, since we simulate the DM, we don't need to do 
	// anything here. 
	return EXIT_SUCCESS;
}

// TODO: add parameters: image, img resolution, lenslet resolution
//int modSimSH(float *img, int imgres[2], int cellres[2]) {
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
	
	// we take the subaperture, which is shsize big, and put it in a larger matrix
	nx = (shsize[0] * 2);
	ny = (shsize[1] * 2);

	// init data structures for images, fill with zeroes
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
				logWarn("Importing wisdom failed.");
				return EXIT_FAILURE;
			}
			// regenerating plan using wisdom imported above.
			simparams.plan_forward = fftw_plan_dft_2d(nx, ny, simparams.shin, simparams.shout, FFTW_FORWARD, FFTW_EXHAUSTIVE);
			fclose(fp);
		}
		
	}
		
	if (simparams.shout == NULL || simparams.shin == NULL || simparams.plan_forward == NULL)
		logErr("Allocation of memory for FFT failed.");
	
	if (ptc.wfs[0].cells[0] * shsize[0] != ptc.wfs[0].res.x || \
		ptc.wfs[0].cells[1] * shsize[1] != ptc.wfs[0].res.y)
		logErr("Incomplete SH cell coverage! This means that nx_subapt * width_subapt != width_img x: (%d*%d,%d) y: (%d*%d,%d)", \
			ptc.wfs[0].cells[0], shsize[0], ptc.wfs[0].res.x, ptc.wfs[0].cells[1], shsize[1], ptc.wfs[0].res.y);

	
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
					 	if (ptc.wfs[0].image[yc*shsize[1]*ptc.wfs[0].res.x + xc*shsize[0] + ip*ptc.wfs[0].res.x + jp] == 0)
							i++;
			
			// allow one quarter of the pixels to be zero, otherwise set subapt to zero and continue
			// TODO: double loop (this one and above) ugly?
			if (i > shsize[1]*shsize[0]/4) {
				for (ip=0; ip<shsize[1]; ip++)
					for (jp=0; jp<shsize[0]; jp++)
						ptc.wfs[0].image[yc*shsize[1]*ptc.wfs[0].res.x + xc*shsize[0] + ip*ptc.wfs[0].res.x + jp] = 0;
								
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
						ptc.wfs[0].image[yc*shsize[1]*ptc.wfs[0].res.x + xc*shsize[0] + ip*ptc.wfs[0].res.x + jp];
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
					ptc.wfs[0].image[yc*shsize[1]*ptc.wfs[0].res.x + xc*shsize[0] + (ip-ny/4)*ptc.wfs[0].res.x + (jp-nx/4)] = \
						simparams.shin[((ip+ny/2)%ny)*nx + (jp+nx/2)%nx][0];
				}
			}
			
		} 
	} // end looping over subapts	
	
	return EXIT_SUCCESS;
}

// TODO: document this
int drvFilterWheel(control_t *ptc, fwheel_t mode) {
	// in simulation this is easy, just set mode in ptc
	ptc->filter = mode;
	return EXIT_SUCCESS;
}
