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
 
	This module expects the following defines to be present (with example values):

	\li FOAM_MODSIM_WAVEFRONT "../config/wavefront.pgm"
	\li FOAM_MODSIM_APERTURE "../config/apert15-256.pgm"
	\li FOAM_MODSIM_APTMASK "../config/apert15-256.pgm"
	\li FOAM_MODSIM_ACTPAT "../config/dm37-256.pgm"
	 
	\li FOAM_MODSIM_WINDX 0	
	\li FOAM_MODSIM_WINDY 0 	
	\li FOAM_MODSIM_SEEING 0.2	
	 
	\li FOAM_MODSIM_ERR 1	
	\li FOAM_MODSIM_ERRWFC 0
	\li FOAM_MODSIM_ERRTYPE 1	
	\li FOAM_MODSIM_ERRVERB 1
	
*/

// HEADERS //
/***********/

#include "foam_modules-sim.h"

// ROUTINES //
/************/

int simInit(mod_sim_t *simparams) {
//	float stats[3];	
	
	// Initialize the module:
	// - Load the wavefront into memory
	if (modReadIMGArrByte(simparams->wf, &(simparams->wfimg), &(simparams->wfres)) != EXIT_SUCCESS)
		return EXIT_FAILURE;
	
	// // !!!:tim:20080703 doesnt work for uint8_t, try 0-255 or not?
//	// Normalize the wavefront between -1 and 1. Get min and max first 
//	imgGetStats(simparams->wfimg, DATA_UINT8, simparams->wfres, -1, stats);	
//	// Now apply correction to the image
//	for (i=0; i < simparams->wfres.x * simparams->wfres.y; i++) { // loop over all pixels
//		simparams->wfimg[i] = (2*(simparams->wfimg[i]-min)/(max-min))-1;
//	}
	
	// - Load the aperture into memory	
	if (modReadIMGArrByte(simparams->apert, &(simparams->apertimg), &(simparams->apertres)) != EXIT_SUCCESS)
		return EXIT_FAILURE;
	
	// - Load the actuator pattern into memory
	if (modReadIMGArrByte(simparams->actpat, &(simparams->actpatimg), &(simparams->actpatres)) != EXIT_SUCCESS)
		return EXIT_FAILURE;
	
	// - Check sanity of values in simparams
	if (simparams->wfres.x < simparams->currimgres.x + 2*simparams->wind.x) {
		logWarn("Simulated wavefront too small (%d) for current x-wind (%d), setting to zero.", 
				simparams->wfres.x, simparams->wind.x);
		simparams->wind.x = 0;
	}
	
	if (simparams->wfres.y < simparams->currimgres.y + 2*simparams->wind.y) {
		logWarn("Simulated wavefront too small (%d) for current y-wind (%d), setting to zero.", 
				simparams->wfres.y, simparams->wind.y);
		simparams->wind.y = 0;
	}

	// - Allocate memory for (simulated) WFS output
	simparams->currimg = (uint8_t *) malloc(simparams->currimgres.x	 * simparams->currimgres.y * sizeof(uint8_t));


	logInfo(0, "Simulation module initialized. Currimg (%dx%d)", simparams->currimgres.x, simparams->currimgres.y);
	return EXIT_SUCCESS;
}

// This function moves the current origin around using 'wind'
int simWind(mod_sim_t *simparams) {
	// Check to see if we are using wind
	if (simparams->wind.x == 0 && simparams->wind.y == 0)
		return EXIT_SUCCESS;
	
	logDebug(LOG_SOMETIMES, "Simulating wind.");
	// Apply 'wind' to the simulated wavefront (i.e. move the origin a few pixels)
	simparams->currorig.x += simparams->wind.x;
	simparams->currorig.y += simparams->wind.y;
	
 	// if the origin is out of bound, reverse the wind direction and move that way
	// X ORIGIN TOO BIG:
	if (simparams->currorig.x + simparams->currimgres.x >= simparams->wfres.x) {
		simparams->wind.x *= -1;						// Reverse X wind speed
		simparams->currorig.x += 2*simparams->wind.x;	// move in the new wind direction, 
		// twice to compensate for the wind already applied above
	}
	// X ORIGIN TOO SMALL
	if (simparams->currorig.x < 0) {
		simparams->wind.x *= -1;	
		simparams->currorig.x += 2*simparams->wind.x;
	}
	
	// Y ORIGIN TOO BIG
	if (simparams->currorig.y + simparams->currimgres.y >= simparams->wfres.y) {
		simparams->wind.y *= -1;						// Reverse Y wind speed
		simparams->currorig.y += 2*simparams->wind.y;	// move in the new wind direction
	}
	// Y ORIGIN TOO SMALL
	if (simparams->currorig.y < 0) {
		simparams->wind.y *= -1;
		simparams->currorig.y += 2*simparams->wind.y;
	}
	
	
	return EXIT_SUCCESS;
}

// This function simulates the wind in the wavefront, mainly
int simAtm(mod_sim_t *simparams) {
	int i,j;
	logDebug(LOG_SOMETIMES, "Simulating atmosphere.");
	uint8_t min, max, pix;
	uint64_t sum;
	
	// Now crop the image, i.e. take a part of the big wavefront and store it in 
	// the location of the wavefront sensor image
	min = max = simparams->wfimg[0];
	for (i=0; i< simparams->currimgres.y; i++) { // y coordinate
		for (j=0; j < simparams->currimgres.x; j++) { // x coordinate 
			pix = simparams->wfimg[(simparams->currorig.y + i)*simparams->wfres.x + simparams->currorig.x + j];
			simparams->currimg[i*simparams->currimgres.x + j] = pix;
//			if (pix > max) pix = max;
//			else if (pix < min) pix = min;
//			sum += pix;
		}
	}
	//logDebug(LOG_SOMETIMES, "Atmosphere: min %f, max %f, sum: %f, avg: %f", min, max, sum, sum/( simparams->currimgres.x *  simparams->currimgres.y));
	
	return EXIT_SUCCESS;
}

int simSensor(mod_sim_t *simparams, mod_sh_track_t *shwfs) {
	
	
//	logDebug(0, "Now simulating %d WFC(s).", ptc.wfc_count);
//	for (i=0; i < ptc->wfc_count; i++)
//		simWFC(&(ptc->wfc[i]), simparams->currimg); // Simulate every WFC in series
	
	
	// if filterwheel is set to pinhole, simulate a coherent image
//	if (ptc.mode == AO_MODE_CAL && ptc.filter == FILT_PINHOLE) {
//		for (i=0; i<ptc.wfs[0].res.x*ptc.wfs[0].res.y; i++)
//			ptc.wfs[0].image[i] = 1;
//			
//	}

	
	// introduce error here,
	// first param: wfc to use for simulation (dependent on ptc.wfc[wfc])
	// second param: 0 for regular sawtooth, 1 for random drift
	// third param: 0 for no output, 1 for ctrl vec output
#ifdef FOAM_MODSIM_ERR
//	modSimError(FOAM_MODSIM_ERRWFC, FOAM_MODSIM_ERRTYPE, FOAM_MODSIM_ERRVERB);
#endif
	
	
//	if (simTel(FOAM_MODSIM_APERTURE, ptc.wfs[0].image, ptc.wfs[0].res) != EXIT_SUCCESS) // Simulate telescope (from aperture.fits)
//		logErr("error in simTel().");
	
//	modDrawStuff(&ptc, 0, screen);
//	sleep(1);

	// Simulate the WFS here.
//	if (modSimSH() != EXIT_SUCCESS) {
//		logWarn("Simulating SH WFSs failed.");
//		return EXIT_FAILURE;
//	}	

//	modDrawStuff(&ptc, 0, screen);
//	sleep(1);

	
	return EXIT_SUCCESS;
}

int simTel(mod_sim_t *simparams) {
	int i;
	
	// Multiply wavefront with aperture
	for (i=0; i < simparams->currimgres.x * simparams->currimgres.y; i++)
	 	if (simparams->apertimg[i] == 0) simparams->currimg[i] = 0;
	
	return EXIT_SUCCESS;
}


//int modSimSH(float *img, int imgres[2], int cellres[2]) {
int simSHWFS(mod_sim_t *simparams, mod_sh_track_t *shwfs) {
	logDebug(LOG_SOMETIMES, "Simulating SH WFSs now.");
	
	int zeropix, i;
	int nx, ny;
	int xc, yc;
	int jp, ip;
	FILE *fp;
	
	double tmp;
	double min = 0.0, max = 0.0;
	
	// we take the subaperture, which is shsize.x * .y big, and put it in a larger matrix
	nx = (shwfs->shsize.x * 2);
	ny = (shwfs->shsize.y * 2);

	// init data structures for images, fill with zeroes
	if (simparams->shin == NULL) {
		simparams->shin = fftw_malloc ( sizeof ( fftw_complex ) * nx * ny );
		for (i=0; i< nx*ny; i++)
			simparams->shin[i][0] = simparams->shin[i][1] = 0;
	}
	
	if (simparams->shout == NULL) {
		simparams->shout = fftw_malloc ( sizeof ( fftw_complex ) * nx * ny );
		for (i=0; i< nx*ny; i++)
			simparams->shout[i][0] = simparams->shout[i][1] = 0;
	}
	
	// init FFT plan, FFTW_ESTIMATE could be replaces by MEASURE or sth, which should calculate FFT's faster
	if (simparams->plan_forward == NULL) {
		logDebug(LOG_SOMETIMES, "Setting up plan for fftw");
		
		fp = fopen(simparams->wisdomfile,"r");
		if (fp == NULL)	{				// File does not exist, generate new plan
			logInfo(LOG_SOMETIMES, "No FFTW plan found in %s, generating new plan, this may take a while.", simparams->wisdomfile);
			simparams->plan_forward = fftw_plan_dft_2d(nx, ny, simparams->shin, simparams->shout, FFTW_FORWARD, FFTW_EXHAUSTIVE);

			// Open file for writing now
			fp = fopen(simparams->wisdomfile,"w");
			if (fp == NULL) {
				logDebug(LOG_SOMETIMES, "Could not open file %s for saving FFTW wisdom.", simparams->wisdomfile);
				return EXIT_FAILURE;
			}
			fftw_export_wisdom_to_file(fp);
			fclose(fp);
		}
		else {
			logInfo(LOG_SOMETIMES, "Importing FFTW wisdom file.");
			logInfo(LOG_SOMETIMES, "If this is the first time this program runs on this machine, this is bad.");
			logInfo(LOG_SOMETIMES, "In that case, please delete '%s' and rerun the program. It will generate new wisdom which is A Good Thing.", \
				simparams->wisdomfile);
			if (fftw_import_wisdom_from_file(fp) == 0) {
				logWarn("Importing wisdom failed.");
				return EXIT_FAILURE;
			}
			// regenerating plan using wisdom imported above.
			simparams->plan_forward = fftw_plan_dft_2d(nx, ny, simparams->shin, simparams->shout, FFTW_FORWARD, FFTW_EXHAUSTIVE);
			fclose(fp);
		}
		
	}
		
	if (simparams->shout == NULL || simparams->shin == NULL || simparams->plan_forward == NULL)
		logErr("Some allocation of memory for FFT failed.");
	
//	if (ptc.wfs[0].cells[0] * shsize[0] != ptc.wfs[0].res.x || \
//		ptc.wfs[0].cells[1] * shsize[1] != ptc.wfs[0].res.y)
//		logErr("Incomplete SH cell coverage! This means that nx_subapt * width_subapt != width_img x: (%d*%d,%d) y: (%d*%d,%d)", \
//			ptc.wfs[0].cells[0], shsize[0], ptc.wfs[0].res.x, ptc.wfs[0].cells[1], shsize[1], ptc.wfs[0].res.y);

	
	logDebug(LOG_SOMETIMES, "Beginning imaging simulation.");

	// now we loop over each subaperture:
	for (yc=0; yc< shwfs->cells.y; yc++) {
		for (xc=0; xc< shwfs->cells.x; xc++) {
			// we're at subapt (xc, yc) here...

			// possible approaches on subapt selection for simulation:
			//  - select only central apertures (use radius)
			//  - use absolute intensity (still partly illuminated apts)
			//  - count pixels with intensity zero

			zeropix = 0;
			for (ip=0; ip< shwfs->shsize.y; ip++)
				for (jp=0; jp< shwfs->shsize.x; jp++)
					 	if (simparams->currimg[yc * shwfs->shsize.y * simparams->currimgres.x + xc*shwfs->shsize.x + ip * simparams->currimgres.x + jp] == 0)
							zeropix++;
			
			// allow one quarter of the pixels to be zero, otherwise set subapt to zero and continue
			if (zeropix > shwfs->shsize.y*shwfs->shsize.x/4) {
				for (ip=0; ip<shwfs->shsize.y; ip++)
					for (jp=0; jp<shwfs->shsize.x; jp++)
						simparams->currimg[yc*shwfs->shsize.y*simparams->currimgres.x + xc*shwfs->shsize.x + ip*simparams->currimgres.x + jp] = 0;
				
				// skip over the rest of the for loop started ~20 lines back
				continue;
			}
			
			// We want to set the helper arrays to zero first
			// otherwise we get trouble? TODO: check this out
			for (i=0; i< nx*ny; i++)
				simparams->shin[i][0] = simparams->shin[i][1] = \
					simparams->shout[i][0] = simparams->shout[i][1] = 0;
//			for (i=0; i< nx*ny; i++)
			
			// add markers to track copying:
			//for (i=0; i< 2*nx; i++)
				//simparams->shin[i][0] = 1;

			//for (i=0; i< ny; i++)
				//simparams->shin[nx/2+i*nx][0] = 1;

			// loop over all pixels in the subaperture, copy them to subapt:
			// I'm pretty sure these index gymnastics are correct (2008-01-18)
			for (ip=0; ip < shwfs->shsize.y; ip++) { 
				for (jp=0; jp < shwfs->shsize.x; jp++) {
					// we need the ipth row PLUS the rows that we skip at the top (shwfs->shsize.y/2+1)
					// the width is not shwfs->shsize.x but twice that plus 2, so mulyiply the row number
					// with that. Then we need to add the column number PLUS the margin at the beginning 
					// which is shwfs->shsize.x/2 + 1, THAT's the right subapt location.
					// For the image itself, we need to skip to the (ip,jp)th subaperture,
					// which is located at pixel yc * the height of a cell * the width of the picture
					// and the x coordinate times the width of a cell time, that's at least the first
					// subapt pixel. After that we add the subaperture row we want which is located at
					// pixel ip * the width of the whole image plus the x coordinate

					simparams->shin[(ip+ ny/4)*nx + (jp + nx/4)][0] = \
						simparams->currimg[yc*shwfs->shsize.y*simparams->currimgres.x + xc*shwfs->shsize.x + ip*simparams->currimgres.x + jp];
					// old: simparams->shin[(ip+ shwfs->shsize.y/2 +1)*nx + jp + shwfs->shsize.x/2 + 1][0] = 
				}
			}
			
			// now image the subaperture, first generate EM wave amplitude
			// this has to be done using complex numbers over the BIG subapt
			// we know that exp ( i * phi ) = cos(phi) + i sin(phi),
			// so we split it up in a real and a imaginary part
			// TODO dit kan hierboven al gedaan worden
			for (ip=shwfs->shsize.y/2; ip<shwfs->shsize.y + shwfs->shsize.y/2; ip++) {
				for (jp=shwfs->shsize.x/2; jp<shwfs->shsize.x+shwfs->shsize.x/2; jp++) {
					tmp = simparams->seeingfac * simparams->shin[ip*nx + jp][0]; // multiply for worse seeing
					//use fftw_complex datatype, i.e. [0] is real, [1] is imaginary
					
					// SPEED: cos and sin are SLOW, replace by taylor series
					// optimilization with parabola, see http://www.devmaster.net/forums/showthread.php?t=5784
					// and http://lab.polygonal.de/2007/07/18/fast-and-accurate-sinecosine-approximation/
					// wrap tmp to (-pi, pi):
					//tmp -= ((int) ((tmp+3.14159265)/(2*3.14159265))) * (2* 3.14159265);
					//simparams->shin[ip*nx + jp][1] = 1.27323954 * tmp -0.405284735 * tmp * fabs(tmp);
					// wrap tmp + pi/2 to (-pi,pi) again, but we know tmp is already in (-pi,pi):
					//tmp += 1.57079633;
					//tmp -= (tmp > 3.14159265) * (2*3.14159265);
					//simparams->shin[ip*nx + jp][0] = 1.27323954 * tmp -0.405284735 * tmp * fabs(tmp);
					
					// used to be:
					simparams->shin[ip*nx + jp][1] = sin(tmp);
					// TvW, disabling sin/cos
					simparams->shin[ip*nx + jp][0] = cos(tmp);
					//simparams->shin[ip*nx + jp][0] = tmp;
				}
			}

			// now calculate the  FFT
			fftw_execute ( simparams->plan_forward );

			// now calculate the absolute squared value of that, store it in the subapt thing
			// also find min and maximum here
			for (ip=0; ip<ny; ip++) {
				for (jp=0; jp<nx; jp++) {
					tmp = simparams->shin[ip*nx + jp][0] = \
					 fabs(pow(simparams->shout[ip*nx + jp][0],2) + pow(simparams->shout[ip*nx + jp][1],2));
					 if (tmp > max) max = tmp;
					 else if (tmp < min) min = tmp;
				}
			}
			// copy subaparture back to main images
			// note: we don't want the center of the fourier transformed image, but we want all corners
			// because the FT begins in the origin. Therefore we need to start at coordinates
			//  nx-(nx_subapt/2), ny-(ny_subapt/2)
			// e.g. for 32x32 subapts and (nx,ny) = (64,64), we start at
			//  (48,48) -> (70,70) = (-16,-16)
			// so we need to wrap around the matrix, which results in the following
			float tmppixf;
			//uint8_t tmppixb;
			for (ip=ny/4; ip<ny*3/4; ip++) { 
				for (jp=nx/4; jp<nx*3/4; jp++) {
					tmppixf = simparams->shin[((ip+ny/2)%ny)*nx + (jp+nx/2)%nx][0];

					simparams->currimg[yc*shwfs->shsize.y*simparams->currimgres.x + xc*shwfs->shsize.x + (ip-ny/4)*simparams->currimgres.x + (jp-nx/4)] = 255.0*(tmppixf-min)/(max-min);
				}
			}
			
		} 
	} // end looping over subapts	
	
	return EXIT_SUCCESS;
}
// !!!:tim:20080703 codedump below here, not used
#if (0)

void modSimError(int wfc, int method, int verbosity) {
	gsl_vector_float *tmpctrl;
	int nact, i;
	int repeat=40;
	float ctrl;
	
	// get the number of actuators for the WFC to simulate
	nact = ptc.wfc[wfc].nact;
	
	tmpctrl = gsl_vector_float_calloc(nact);
	// fake control vector to make error
	if (method == 1) {
		// regular sawtooth drift is here:
		for (i=0; i<nact; i++) {
			ctrl = ((ptc.frames % repeat)/(float)repeat * 2 - 1) * ( round( (ptc.frames % repeat)/(float)repeat )*2 - 1);
			gsl_vector_float_set(tmpctrl, i, ctrl);
			// * ( round( (ptc.frames % 40)/40.0 )*2 - 1)
		}
	}
	else {
		// random drift is here:
		for (i=0; i<nact; i++) {
			ctrl = gsl_vector_float_get(tmpctrl,i) + (float) drand48()*0.1;
			gsl_vector_float_set(tmpctrl, i, ctrl);
			// * ( round( (ptc.frames % 40)/40.0 )*2 - 1)
		}
	}
	
	// What routine do we need to call to simulate this WFC?
	if (ptc.wfc[wfc].type == WFC_DM)
		modSimDM(FOAM_MODSIM_APTMASK, FOAM_MODSIM_ACTPAT, ptc.wfc[0].nact, tmpctrl, ptc.wfs[0].image, ptc.wfs[0].res, -1); // last arg is for niter. -1 for autoset
	else if (ptc.wfc[wfc].type == WFC_TT)
		modSimTT(tmpctrl, ptc.wfs[0].image, ptc.wfs[0].res);
	
	if (verbosity == 1) {
		logInfo(LOG_SOMETIMES, "Error: %d with %d acts:", wfc, nact);
		for (i=0; i<nact; i++)
			logInfo(LOG_SOMETIMES | LOG_NOFORMAT, "(%.2f) ", gsl_vector_float_get(tmpctrl,i));
			
		logInfo(LOG_SOMETIMES | LOG_NOFORMAT, "\n");
	}
	
	// old stuff:
	// WE APPLY DM OR TT ERRORS HERE, 
	// AND TEST IF THE WFC'S CAN CORRECT IT
	///////////////////////////////////////

	// gsl_vector_float *tmpctrl;
	// tmpctrl = gsl_vector_float_calloc(ptc.wfc[0].nact);
	
	// regular sawtooth drift is here:
	// for (i=0; i<ptc.wfc[0].nact; i++) {
	// 	gsl_vector_float_set(tmpctrl, i, ((ptc.frames % 40)/40.0 * 2 - 1) * ( round( (ptc.frames % 40)/40.0 )*2 - 1));
	// 	// * ( round( (ptc.frames % 40)/40.0 )*2 - 1)
	// }

	// logDebug(0, "TT: faking tt with : %f, %f", gsl_vector_float_get(tmpctrl, 0), gsl_vector_float_get(tmpctrl, 1));
	// if (ttfd == NULL) ttfd = fopen("ttdebug.dat", "w+");
	// fprintf(ttfd, "%f, %f\n", gsl_vector_float_get(tmpctrl, 0), gsl_vector_float_get(tmpctrl, 1));
	
	//	modSimTT(tmpctrl, ptc.wfs[0].image, ptc.wfs[0].res);
	// modSimDM(FOAM_MODSIM_APTMASK, FOAM_MODSIM_ACTPAT, ptc.wfc[0].nact, tmpctrl, ptc.wfs[0].image, ptc.wfs[0].res, -1); // last arg is for niter. -1 for autoset

	// END OF FAKING ERRORS
	///////////////////////
	
}



int simWFC(control_t *ptc, int wfcid, int nact, gsl_vector_float *ctrl, float *image) {
	// we want to simulate the tip tilt mirror here. What does it do
	
	logDebug(LOG_SOMETIMES, "WFC %d (%s) has %d actuators, simulating", wfcid, ptc->wfc[wfcid].name, ptc->wfc[wfcid].nact);
	// logDebug(0, "TT: Control is: %f, %f", gsl_vector_float_get(ctrl,0), gsl_vector_float_get(ctrl,1));
	// if (ttfd == NULL) ttfd = fopen("ttdebug.dat", "w+");
	// fprintf(ttfd, "%f, %f\n", gsl_vector_float_get(ctrl,0), gsl_vector_float_get(ctrl,1));

	if (ptc->wfc[wfcid].type == WFC_TT)
		modSimTT(ctrl, image, ptc->wfs[0].res);
	else if (ptc->wfc[wfcid].type == WFC_DM) {
		logDebug(LOG_SOMETIMES, "Running modSimDM with %s and %s, nact %d", FOAM_MODSIM_APTMASK, FOAM_MODSIM_ACTPAT, nact);
		modSimDM(FOAM_MODSIM_APTMASK, FOAM_MODSIM_ACTPAT, nact, ctrl, image, ptc->wfs[0].res, -1); // last arg is for niter. -1 for autoset
	}
	else {
		logWarn("Unknown WFC (%d) encountered (not TT or DM, type: %d, name: %s)", wfcid, ptc->wfc[wfcid].type, ptc->wfc[wfcid].name);
		return EXIT_FAILURE;
	}
					
	return EXIT_SUCCESS;
}



int drvSetActuator(control_t *ptc, int wfc) {
	// this is a placeholder, since we simulate the DM, we don't need to do 
	// anything here. 
	return EXIT_SUCCESS;
}


// TODO: document this
int drvFilterWheel(control_t *ptc, fwheel_t mode) {
	// in simulation this is easy, just set mode in ptc
	ptc->filter = mode;
	return EXIT_SUCCESS;
}

#endif /* #if (0) */
