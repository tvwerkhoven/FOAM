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
	int wind[2]; 		// 'windspeed' in pixels/cycle
	long curorig[2]; 	// current origin
	int shcells[2]; 	// number of SH cells in both directions
	float *simimg; 		// pointer to the image we use to simulate stuff
	long simimgres[2];	// resolution of the simulation image
};

struct simul simparams = {
	.wind[0] = 10,
	.wind[1] = 0,
	.curorig[0] = 1,
	.curorig[1] = 1,
	.shcells[0] = 8,
	.shcells[1] = 8,
	.simimg = NULL
};

// We need these every time, declare them globally
char errmsg[FLEN_STATUS]; 		// FLEN_STATUS is from fitsio.h
int status = 0;  				// MUST initialize status
float nulval = 0.0;
int anynul = 0;

int drvReadSensor() {
	int i;
	int once=0;
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
	

	//displayImg(ptc.wfs[0].image, ptc.wfs[0].res);
	
	if (simTel("aperture.fits", ptc.wfs[0].res, ptc.wfs[0].image) != EXIT_SUCCESS) { // Simulate telescope (from aperture.fits)
		if (status > 0) {
			fits_get_errstatus(status, errmsg);
			logErr("fitsio error in simTel(): (%d) %s", status, errmsg);
			status = 0;
		}
		else logErr("error in simTel().");
	}
	
	
	//displayImg(simparams.simimg, simparams.simimgres);
		
	logDebug("Now simulating %d WFC(s).", ptc.wfc_count);
	//for (i=0; i < ptc.wfc_count; i++)
	//	simWFC(i, ptc.wfc[i].nact, ptc.wfc[i].ctrl, ptc.wfs[0].image); // Simulate every WFC
	
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
	if (simparams.wind[1] != 0 && simparams.curorig[1] > simparams.simimgres[1]-ptc.wfs[1].res[1]) {
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
	double sum;
	
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
	
    
	// Crop the image to and take only a portion res around origin
	
	// If we move outside the simulated wavefront, reverse the 'windspeed'
	if ((origin[0] + res[0] >= simparams.simimgres[0]) || (origin[1] + res[1] >= simparams.simimgres[1]) 
		|| (origin[0] < 0) || (origin[1] < 0)) {
		logErr("Simulation out of bound, wind reset failed. (%f,%f) ", origin[0], origin[1]);
		return EXIT_FAILURE;
	}
	
	// TODO: How do we copy this efficiently?
	for (j=0; j<res[0]; j++) // y coordinate
	{
		for (i=0; i<res[1]; i++) // x coordinate 
		{
			image[i + j*res[0]] = simparams.simimg[(i+origin[0])+(j+origin[1])*simparams.simimgres[0]];
			sum += simparams.simimg[i+j*simparams.simimgres[0]];
//			if (i% 100 == 0 && j % 75 == 0) 
//				logDebug("set coordinate %d to %f (should be %f)", 
//					i + j*res[0], image[i + j*res[0]], simparams.simimg[i+j*res[0]]);
		}
	}
	logDebug("Img sum is %lf", sum, naxes[0], naxes[1]);
			// TODO: use origin
			// naxes[0] is the width of the total wavefront, use this for origin[1]
	

	return EXIT_SUCCESS;
}

int drvSetActuator() {
	int i;
	
	logDebug("%d WFCs to set.", ptc.wfc_count);
	
	for(i=0; i < ptc.wfc_count; i++)
		logDebug("Setting WFC %d with %d acts.", i, ptc.wfc[i].nact);
		
	return EXIT_SUCCESS;
}

int modParseSH() {
	logDebug("Parsing SH WFSs now.");

	// we need to port this to C:

	// imageOut = wavefront
	// ssize = (SIZE(wavefront))[1:2]
	// 
	// ;;shsize defines the cellsize of a sh-sensor-cell
	// shsize = ssize[0]/SHsens
	
	int shsize[2];
	float *subapt;
	int i;
	int xc, yc;
	int xp, yp;

	gsl_fft_complex_wavetable * wavetable;
	gsl_fft_complex_workspace * workspace;	

	double *csubapt; // TODO: do we need double precision here?
	float tmp;
		
	// we have simparams.shcells[0] by simparams.shcells[1] SH cells, so the size of each cell is:
	shsize[0] = ptc.wfs[0].res[0]/simparams.shcells[0];
	shsize[1] = ptc.wfs[0].res[1]/simparams.shcells[1];
	
	// TODO: again we only support float images here :<

	int pts = (shsize[0] * 2 + 2) * ( shsize[1] * 2 + 2);
	subapt = calloc(pts, sizeof(*ptc.wfs[0].image)); // We store the subaperture here, +2 is to make sure we 
																// have enough space on both sides lateron (even & odd sized SHcells)
	if (subapt == NULL) {
		logErr("Failed to allocate memory for subaperture.");
		return EXIT_FAILURE;
	}
	
	// 
	// if (shsize*SHsens NE ssize[0]) then $
	// 	PRINT,'WARNING: incomplete SH cell coverage!'
	
	if (simparams.shcells[0] * shsize[0] != ptc.wfs[0].res[0] || \
		simparams.shcells[1] * shsize[1] != ptc.wfs[0].res[1])
		logErr("Incomplete SH cell coverage! This means that nx_subapt * width_subapt != width_img");

	// 
	// im = COMPLEX(0,1)
	// 
	// ;;zero pads  (+1 is to be sure, with odd numbers)
	// zeroside = DBLARR(shsize/2+1,shsize)
	// zerotop = DBLARR((shsize/2+1)*2+shsize,shsize/2+1)
	// 
	
	// now we loop over each subaperture:
	for (yc=0; yc<simparams.shcells[1]; yc++) {
		for (xc=0; xc<simparams.shcells[0]; xc++) {
			// we're at subapt (xc, yc) here...
			// loop over all pixels in the subaperture, copy them to subapt:
			for (yp=0; yp<shsize[1]; yp++) {
				for (xp=0; xp<shsize[0]; xp++) {
					// we need the ypth row PLUS the rows that we skip at the top (shsize[1]/2+1)
					// the width is not shsize[0] but twice that plus 2, so mulyiply the row number
					// with that. Then we need to add the column number PLUS the margin at the beginning 
					// which is shsize[0]/2 + 1, THAT's the right subapt location.
					// For the image itself, we need to skip to the (yp,xp)th subaperture,
					// which is located at pixel yc * the height of a cell * the width of the picture
					// and the x coordinate times the width of a cell time, that's at least the first
					// subapt pixel. After that we add the subaperture row we want which is located at
					// pixel yp * the width of the whole image plus the x coordinate
					subapt[(yp+ shsize[1]/2 +1)*(shsize[0]*2+2) + xp + shsize[0]/2 + 1] = \
					ptc.wfs[0].image[yc*shsize[1]*ptc.wfs[0].res[0] + xc*shsize[0] + yp*ptc.wfs[0].res[0] + xp];
				}
			}
			
			// now image the subaperture, first generate EM wave amplitude
			// this has to be done using complex numbers.
			// we know that exp ( i * phi ) = cos(phi) + i sin(phi),
			// so we split it up in a real and a imaginary part
			csubapt = calloc(2*pts, sizeof(*csubapt));
			for (yp=shsize[1]/2+1; yp<shsize[1] + shsize[1]/2+1; yp++) {
				for (xp=shsize[0]/2+1; xp<shsize[0]+shsize[0]/2+1; xp++) {
					tmp = subapt[yp*(shsize[0]*2+2) + xp];
					//use gsl_complex_packed_array type for complex data
					csubapt[yp*(shsize[0]*2+2) + xp] = cos(tmp);
					csubapt[yp*(shsize[0]*2+2) + xp + 1] = sin(tmp);
				}
			}

			// now calculate the  FFT, pts is the number of (complex) datapoints
			wavetable = gsl_fft_complex_wavetable_alloc (pts);
			workspace = gsl_fft_complex_workspace_alloc (pts);
			gsl_fft_complex_forward(csubapt, 1, pts, wavetable, workspace);
			
			// now calculate the absolute squared value of that, store it in the subapt thing
			for (yp=0; yp<(2*shsize[1]+2); yp++) {
				for (xp=0; xp<(2*shsize[0]+2); xp++) {
					subapt[yp*(shsize[0]*2+2) + xp] = pow(csubapt[yp*(shsize[0]*2+2) + xp],2) + pow(csubapt[yp*(shsize[0]*2+2) + xp + 1],2);
					//printf("%f ", subapt[yp*(shsize[0]*2+2) + xp]);
					//printf("%d ",(int) subapt[yc*(shsize[0]*2+2) + xc]);
				}
				//printf("\n");
			}
		}
	}
	logDebug("Parsed subaperture imaging, copying to main image.");
	
	for (yc=0; yc<simparams.shcells[1]; yc++) {
		for (xc=0; xc<simparams.shcells[0]; xc++) {
			// we're at subapt (xc, yc) here...
			// loop over all pixels in the subaperture, copy them to subapt:
			for (yp=0; yp<shsize[1]; yp++) {
				for (xp=0; xp<shsize[0]; xp++) {
					// we need the ypth row PLUS the rows that we skip at the top (shsize[1]/2+1)
					// the width is not shsize[0] but twice that plus 2, so mulyiply the row number
					// with that. Then we need to add the column number PLUS the margin at the beginning 
					// which is shsize[0]/2 + 1, THAT's the right subapt location.
					// For the image itself, we need to skip to the (yp,xp)th subaperture,
					// which is located at pixel yc * the height of a cell * the width of the picture
					// and the x coordinate times the width of a cell time, that's at least the first
					// subapt pixel. After that we add the subaperture row we want which is located at
					// pixel yp * the width of the whole image plus the x coordinate
					ptc.wfs[0].image[yc*shsize[1]*ptc.wfs[0].res[0] + xc*shsize[0] + yp*ptc.wfs[0].res[0] + xp] = \
						subapt[(yp+ shsize[1]/2 +1)*(shsize[0]*2+2) + xp + shsize[0]/2 + 1];
				}
			}
		}
	}
	gsl_fft_complex_wavetable_free(wavetable);
	gsl_fft_complex_workspace_free(workspace);	

	displayImg(ptc.wfs[0].image, ptc.wfs[0].res); // TODO: totale crap?
	return EXIT_SUCCESS;
	
	// ;;split up the image matrix in SHsens^2  pieces
	// FOR K=0,SHsens-1 DO BEGIN
	//  FOR M=0,SHsens-1 DO BEGIN
	//   wavepiece = wavefront[K*shsize:(K+1)*shsize-1,M*shsize:(M+1)*shsize-1]
	// 
	//   IF mode eq 'fft' THEN BEGIN
	//    ;;calculate actual EM wave amplitude:
	//    wave = 1D * EXP(im*wavepiece)
	// 
	//    ;;we need to pad the array when using FFT because otherwise edge effects
	//    ;;come into play
	//    ;padded2 = [zeroside,wave,zeroside]
	//    ;padded2 = [[zerotop],[padded2],[zerotop]]
	//    padded = padmat(wave)
	// 
	//    ;help,padded2,padded
	//    ;print,TOTAL(padded2-padded)
	// 
	//    ;;we now have a wavefront with a padded edge of value 0, calculate FFT
	//    image = ABS(FFT(padded))^2
	//    ;;shift to center
	//    image = SHIFT(image,(shsize+(shsize/2+1)*2)/2,(shsize+(shsize/2+1)*2)/2)
	// 
	//    ;crop1 = image[shsize/2+1:shsize*3/2,shsize/2+1:shsize*3/2]
	//    crop = unpadmat(image)
	// 
	//    ;help,crop1,crop
	//    ;print,TOTAL(crop1-crop)
	// 
	//    ;;crop image to original size:
	//    imageOut[K*shsize:(K+1)*shsize-1,M*shsize:(M+1)*shsize-1] = crop
	// 
	//   ENDIF
	// 
	//  ENDFOR
	// ENDFOR
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
	
	logDebug("Displaying image precalc, min: %f, max: %f (%f,%f).", min, max, img[0], img[100]);
	
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
