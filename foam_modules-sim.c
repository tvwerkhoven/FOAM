/*! 
	@file foam_modules-sim.c
	@author Tim van Werkhoven
	@date November 30 2007

	@brief This file contains the functions to run FOAM in simulation mode, and 
	links to other files like foam_modules-dm.c which simulates the DM.
*/

// HEADERS //
/***********/

#include "cs_library.h"
#include "ao_library.h"
#include "foam_modules.h"

// GLOBAL VARIABLES //
/********************/	

// These are defined in cs_library.c
extern config_t cs_config;
extern control_t ptc;
extern SDL_Surface *screen;

struct simul {
	int wind[2]; 			// 'windspeed' in pixels/cycle
	long curorig[2]; 		// current origin
	float *simimg; 			// pointer to the image we use to simulate stuff
	long simimgres[2];		// resolution of the simulation image
	fftw_complex *shin;		// input for fft algorithm
	fftw_complex *shout;	// output for fft (but shin can be used if fft is inplace)
	fftw_plan plan_forward; // plan, one time calculation on how to calculate ffts fastest
	char wisdomfile[32];
};

struct simul simparams = {
	.wind[0] = 10,
	.wind[1] = 5,
	.curorig[0] = 1,
	.curorig[1] = 1,
	.simimg = NULL,
	.plan_forward = NULL,
	.wisdomfile = "fftw_wisdom.dat",
};

// We need these every time, declare them globally
char errmsg[FLEN_STATUS]; 		// FLEN_STATUS is from fitsio.h
int status = 0;  				// MUST initialize status
float nulval = 0.0;
int anynul = 0;

int drvReadSensor() {
//	int i;
	logDebug("Now reading %d sensors, origin is at (%d,%d).", ptc.wfs_count, simparams.curorig[0], simparams.curorig[1]);
	// TODO: simulate object, atmosphere, telescope, TT, DM, WFS (that's a lot :O)
	
	if (ptc.wfs_count < 1) {
		logErr("Nothing to process, no WFSs defined.");
		return EXIT_FAILURE;
	}
	
		// This simulates the point source + atmosphere (from wavefront.fits)
//	if (simparams.simimg == NULL) {
		if (simAtm("wavefront.fits", ptc.wfs[0].res, simparams.curorig, ptc.wfs[0].image) != EXIT_SUCCESS) { 	
			if (status > 0) {
				fits_get_errstatus(status, errmsg);
				logErr("fitsio error in simAtm(): (%d) %s", status, errmsg);
				status = 0;
			}
			else logErr("error in simAtm().");
		}
		logDebug("simAtm() done");
//	}



	if (simTel("aperture.fits", ptc.wfs[0].res, ptc.wfs[0].image) != EXIT_SUCCESS) { // Simulate telescope (from aperture.fits)
		if (status > 0) {
			fits_get_errstatus(status, errmsg);
			logErr("fitsio error in simTel(): (%d) %s", status, errmsg);
			status = 0;
		}
		else logErr("error in simTel().");
	}

//	logDebug("Now simulating %d WFC(s).", ptc.wfc_count);
	//for (i=0; i < ptc.wfc_count; i++)
	//	simWFC(i, ptc.wfc[i].nact, ptc.wfc[i].ctrl, ptc.wfs[0].image); // Simulate every WFC
	
	
	// Simulate the WFS here. TODO: could be abstracted to a simWFC module or something.
	if (modSimSH() != EXIT_SUCCESS) {
		logDebug("Simulating SH WFSs failed.");
		return EXIT_FAILURE;
	}	
	
	// Check if we have enough simulated wavefront to use wind 
	// (should be at least res+wind, take 2*wind to be sure)
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

// TODO: this only works with 256x256 images...
int simWFC(int wfcid, int nact, float *ctrl, float *image) {
	// we want to simulate the tip tilt mirror here. What does it do
	logDebug("WFC %d (%s) has %d actuators, simulating", wfcid, ptc.wfc[wfcid].name, ptc.wfc[wfcid].nact);
	if (wfcid == 1)
		simDM("dm37/apert15-256.pgm", "dm37/dm37-256.pgm", nact, ctrl, image, 0);
//	if (wfcid == 1)
		//simTT();
	
	return EXIT_SUCCESS;
}

int simTel(char *file, long res[2], float *image) {
	fitsfile *fptr;
	int bitpix, naxis, i;
	long naxes[2], fpixel[] = {1,1};
	long nelements = res[0] * res[1];
	float *aperture;
	if ((aperture = malloc(nelements * sizeof(*aperture))) == NULL)
		return EXIT_FAILURE;
	
	// file should have exactly the same resolution as *image (res[0] x res[1])
	fits_open_file(&fptr, file, READONLY, &status);				// Open FITS file
	if (status > 0)
		return EXIT_FAILURE;
	
	fits_get_img_param(fptr, 2,  &bitpix, \
						&naxis, naxes, &status);				// Read header
	if (status > 0)
		return EXIT_FAILURE;
	if (naxes[0] * naxes[1] != nelements) {
		logErr("Error in simTel(), fitsfile not the same dimension as image (%dx%d vs %dx%d)", \
		naxes[0], naxes[1], res[0], res[1]);
		return EXIT_FAILURE;
	}
					
	fits_read_pix(fptr, TFLOAT, fpixel, \
						nelements, &nulval, aperture, \
						&anynul, &status);						// Read image to image
	if (status > 0)
		return EXIT_FAILURE;
	
	logDebug("Aperture read successfully, processing with image.");
	
	// Multiply wavefront with aperture
	for (i=0; i < nelements; i++)
		image[i] *= aperture[i];
	
	fits_close_file(fptr, &status);
	return EXIT_SUCCESS;
}

int simAtm(char *file, long res[2], long origin[2], float *image) {
	// TODO: broken :(
	// ASSUMES WFS_COUNT > 0
	// ASSUMES ptc.wfs[0].image is initialized to float 
	// ASSUMES simparams.simimg is float
	
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
			
			// TODO: can be done in one go?
			simparams.simimgres[0] = naxes[0];
			simparams.simimgres[1] = naxes[1];
		
			/*for (j=0; j<512; j++) {
				for (i=0; i<512; i++) {
					simparams.simimg[i + 512*j] = (i + j) % (128);
				}
			}*/
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
	if ((origin[0] + res[0] >= simparams.simimgres[0]) || (origin[1] + res[1] >= simparams.simimgres[1]) 
		|| (origin[0] < 0) || (origin[1] < 0)) {
		logErr("Simulation out of bound, wind reset failed. (%f,%f) ", origin[0], origin[1]);
		return EXIT_FAILURE;
	}
	
	// TODO: How do we copy this efficiently?
	for (i=0; i<res[1]; i++) { // y coordinate
		for (j=0; j<res[0]; j++) { // x coordinate 
			image[i*res[0] + j] = simparams.simimg[(i+origin[1])*simparams.simimgres[0] + (j+origin[0])];
		}
	}	

	return EXIT_SUCCESS;
}

int drvSetActuator() {
	int i;
	
	logDebug("%d WFCs to set.", ptc.wfc_count);
	
	for(i=0; i < ptc.wfc_count; i++)
		logDebug("Setting WFC %d with %d acts.", i, ptc.wfc[i].nact);
		
	return EXIT_SUCCESS;
}

float sae(float *subapt, float *refapt, long res[2]) {
	// *ip     pointer to first pixel of each subaperture
	// *rp     pointer to first pixel of (shifted) reference

	float sum=0;
	int i,j;
	
	// loop over alle pixels
	for (i=0; i < res[1]; i++) {
		for (j=0; j<res[0]; j++) {
			sum += fabs(subapt[i*res[0] + j] - refapt[i*res[0] + j]);
		}
	}
	return sum;
}

void imcal(float *corrim, float *image, float *darkim, float *flatim) {
	// substract the dark, multiply with the flat (right?)
	// TODO: dark and flat currently ignored, fix that
	int shsize[2];
	int i,j;
	
	shsize[0] = ptc.wfs[0].res[0]/ptc.wfs[0].cells[0];
	shsize[1] = ptc.wfs[0].res[1]/ptc.wfs[0].cells[1];
	
	
	for (i=0; i<shsize[1]; i++) {
		for (j=0; j<shsize[0]; j++) {
			corrim[i*shsize[0] + j] = image[i*ptc.wfs[0].cells[0]+j];
		}
	}
}


int modParseSH(int wfs) {
	// now calculate the offsets 
	int ns;
	
	// now we loop over each subaperture:
	for (ns=0; ns<ptc.wfs[wfs].nsubap; ns++) {
		// we're now at subapt ns with coordinates (subc[ns][0], subc[ns][1])
		// this array of subapts has already been calibrated by selectSubapts()
/*
		    ip = &imag[suby[sn]*NY+subx[sn]]; // set pointers to various 'images'
		    dp = &dark[sn*SX*SY];
		    gp = &gain[sn*SX*SY];
		    cp = &cimage[sn*SX*SY];
			
			// we shift the subapts by 5 pixels each way, and 
			
			// loop over all pixels in the subaperture, copy them to subapt:
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
					simparams.shin[(ip+ shsize[1]/2 +1)*nx + jp + shsize[0]/2 + 1][0] = \
						ptc.wfs[0].image[yc*shsize[1]*ptc.wfs[0].res[0] + xc*shsize[0] + ip*ptc.wfs[0].res[0] + jp];
				}
			}

			
			// now image the subaperture, first generate EM wave amplitude
			// this has to be done using complex numbers over the BIG subapt
			// we know that exp ( i * phi ) = cos(phi) + i sin(phi),
			// so we split it up in a real and a imaginary part
			for (ip=shsize[1]/2+1; ip<shsize[1] + shsize[1]/2+1; ip++) {
				for (jp=shsize[0]/2+1; jp<shsize[0]+shsize[0]/2+1; jp++) {
					tmp = 6.0*simparams.shin[ip*nx + jp][0]; // 6.0 = 2 * pi :P
					//use fftw_complex datatype, i.e. [0] is real, [1] is imaginary
					simparams.shin[ip*nx + jp][0] = cos(tmp);
					simparams.shin[ip*nx + jp][1] = sin(tmp);
				}
			}

			// now calculate the  FFT, pts is the number of (complex) datapoints
			fftw_execute ( simparams.plan_forward );

			// now calculate the absolute squared value of that, store it in the subapt thing
			for (ip=0; ip<ny; ip++)
				for (jp=0; jp<nx; jp++)
					simparams.shin[ip*nx + jp][0] = \
					 fabs(pow(simparams.shout[ip*nx + jp][0],2) + pow(simparams.shout[ip*nx + jp][1],2));
			
			// copy subaparture back to main image
			// note: we don't want the center of the image, but we want all corners
			// because begins in the origin. Therefore we need to start at coordinates
			//  nx-(nx_subapt/2), ny-(ny_subapt/2)
			// e.g. for 32x32 subapts and (nx,ny) = (66,66), we start at
			//  (50,50) -> (72,72) = (-16,-16)
			// so we need to wrap around the matrix, which results in:
			//  (ip,jp) + (ny,nx)-(shsize[1],shsize[2])/2 % (ny,nx)
			// if (ip,jp) starts at (0,0)
			for (ip=0; ip<shsize[1]; ip++) { 
				for (jp=0; jp<shsize[0]; jp++) {
					ptc.wfs[0].image[yc*shsize[1]*ptc.wfs[0].res[0] + xc*shsize[0] + ip*ptc.wfs[0].res[0] + jp] = \
						simparams.shin[((ip+ny-shsize[1]/2) % shsize[1]) * nx + ((jp + nx - shsize[0]/2) % shsize[0])][0];		
				}
			}		*/
	} // end looping over subapts
	return EXIT_SUCCESS;
	
}

int modSimSH() {
	logDebug("Simulating SH WFSs now.");
	
	int shsize[2];
	int i;
	int nx, ny;
	int xc, yc;
	int jp, ip;
	FILE *fp;
	
	double tmp;

	// we have ptc.wfs[0].cells[0] by ptc.wfs[0].cells[1] SH cells, so the size of each cell is:
	shsize[0] = ptc.wfs[0].res[0]/ptc.wfs[0].cells[0];
	shsize[1] = ptc.wfs[0].res[1]/ptc.wfs[0].cells[1];

	nx = (shsize[0] * 2); // TODO: fixen voor oneven // DONE: eis is dat resolutie even is
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
			// regenerating plan using wisdom imported above. TODO: no extra flags needed?
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
					tmp = 6.0*simparams.shin[ip*nx + jp][0]; // multiply for worse seeing
					//use fftw_complex datatype, i.e. [0] is real, [1] is imaginary
					simparams.shin[ip*nx + jp][0] = cos(tmp);
					simparams.shin[ip*nx + jp][1] = sin(tmp);
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
	
	//manually copy one subapt to check the alignment and shit
	
	
	return EXIT_SUCCESS;
}

int modCalcDMVolt() {
	logDebug("Calculating DM voltages");
	return EXIT_SUCCESS;
}

int displayImg(float *img, long res[2]) {
	// ONLY does float images
	int x, y, i;
	float max=img[0];
	float min=img[0];
	
	//logDebug("Displaying image precalc, min: %f, max: %f (%f,%f).", min, max, img[0], img[100]);
	
	// we need this loop to check the maximum and minimum intensity. Do we need that? can't SDL do that?	
	for (x=0; x < res[0]*res[1]; x++) {
		if (img[x] > max)
			max = img[x];
		if (img[x] < min)
			min = img[x];
	}
	
	logDebug("Displaying image, min: %f, max: %f.", min, max);
	Slock(screen);
	for (x=0; x<res[0]; x++) {
		for (y=0; y<res[1]; y++) {
			i = (int) ((img[y*res[0] + x]-min)/(max-min)*256);
			DrawPixel(screen, x, y, \
				i, \
				i, \
				i);
		}
	}
	Sulock(screen);
	SDL_Flip(screen);
	
	return EXIT_SUCCESS;
}

void Slock(SDL_Surface *screen) {
	if ( SDL_MUSTLOCK(screen) )	{
		if ( SDL_LockSurface(screen) < 0 )
			return;
	}
}

void Sulock(SDL_Surface *screen) {
	if ( SDL_MUSTLOCK(screen) )
		SDL_UnlockSurface(screen);
}

void DrawPixel(SDL_Surface *screen, int x, int y, Uint8 R, Uint8 G, Uint8 B) {
	Uint32 color = SDL_MapRGB(screen->format, R, G, B);
	switch (screen->format->BytesPerPixel) {
		case 1: // Assuming 8-bpp
			{
				Uint8 *bufp;
				bufp = (Uint8 *)screen->pixels + y*screen->pitch + x;
				*bufp = color;
			}
			break;
		case 2: // Probably 15-bpp or 16-bpp
			{
				Uint16 *bufp;
				bufp = (Uint16 *)screen->pixels + y*screen->pitch/2 + x;
				*bufp = color;
			}
			break;
		case 3: // Slow 24-bpp mode, usually not used
			{
				Uint8 *bufp;
				bufp = (Uint8 *)screen->pixels + y*screen->pitch + x * 3;
				if(SDL_BYTEORDER == SDL_LIL_ENDIAN)
				{
					bufp[0] = color;
					bufp[1] = color >> 8;
					bufp[2] = color >> 16;
				} else {
					bufp[2] = color;
					bufp[1] = color >> 8;
					bufp[0] = color >> 16;
				}
			}
			break;
		case 4: { // Probably 32-bpp
				Uint32 *bufp;
				bufp = (Uint32 *)screen->pixels + y*screen->pitch/4 + x;
				*bufp = color;
			}
			break;
	}
}
