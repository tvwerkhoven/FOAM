/*! 
	@file foam_modules-sim.c
	@author Tim van Werkhoven
	@date November 30 2007

	@brief This file contains the functions to run FOAM in simulation mode.
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
	int wind[2]; // 'windspeed' in pixels/cycle
	long curorig[2]; // current origin
	int shcells[2]; // number of SH cells in both directions
};

struct simul simparams = {
	.wind[0] = 10,
	.curorig[0] = 1,
	.curorig[1] = 1,
	.shcells[0] = 8,
	.shcells[1] = 8	
};

// We need these every time...
char errmsg[FLEN_STATUS]; // FLEN_STATUS is from fitsio.h
int status = 0;  /* MUST initialize status */
float nulval = 0.0;
int anynul = 0;

int drvReadSensor() {
	int i;
	logDebug("Now reading %d sensors, origin is at (%d,%d).", ptc.wfs_count, simparams.curorig[0], simparams.curorig[1]);
	// TODO: simulate object, atmosphere, telescope, TT, DM, WFS (that's a lot :O)
	
	if (ptc.wfs_count < 1) {
		logErr("Nothing to process, no WFSs defined.");
		return EXIT_FAILURE;
	}


	if (simAtm("wavefront.fits", ptc.wfs[0].res, simparams.curorig, ptc.wfs[0].image) != EXIT_SUCCESS) { // This simulates the point source + atmosphere (from wavefront.fits)
		if (status > 0) {
			fits_get_errstatus(status, errmsg);
			logErr("fitsio error in simAtm(): (%d) %s", status, errmsg);
			status = 0;
		}
		else logErr("error in simAtm().");
	}

	//displayImg(ptc.wfs[0].image, ptc.wfs[0].res);
	
	if (simTel("aperture.fits", ptc.wfs[0].res, ptc.wfs[0].image) != EXIT_SUCCESS) { // Simulate telescope (from aperture.fits)
		if (status > 0) {
			fits_get_errstatus(status, errmsg);
			logErr("fitsio error in simTel(): (%d) %s", status, errmsg);
			status = 0;
		}
		else logErr("error in simTel().");
	}
	
	displayImg(ptc.wfs[0].image, ptc.wfs[0].res);
		
//	logDebug("Now simulating %d WFC(s).", ptc.wfc_count);
	for (i=0; i < ptc.wfc_count; i++)
		simWFC(i, ptc.wfc[i].nact, ptc.wfc[i].ctrl, ptc.wfs[0].image); // Simulate every WFC
	
	simparams.curorig[0] += simparams.wind[0];
	simparams.curorig[1] += simparams.wind[1];
	if (simparams.curorig[0] > 1024-ptc.wfs[0].res[0]) // TODO: 1024 is hardcoded, should instead get width of wavefront.fits
		simparams.curorig[0] = 1;
	if (simparams.curorig[1] > 1024-ptc.wfs[0].res[1])
		simparams.curorig[1] = 1;
	
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
	// ASSUMES WFS_COUNT > 0
	// ASSUMES ptc.wfs[0].image is initialized to float 
	fitsfile *fptr;
	int bitpix, naxis;
	long fpixel[] = {1,1};
	long naxes[2];
	long nelements = res[0] * res[1];
	char newfile[strlen(file)+32];
	
	// limit loading to requested part only
	sprintf(newfile, "%s[%ld:%ld,%ld:%ld]", file, origin[0], origin[0]+res[0]-1, origin[1], origin[1]+res[1]-1);
	logDebug("Loading fits file %s", newfile);
	fits_open_file(&fptr, newfile, READONLY, &status);				// Open FITS file
	if (status > 0)
		return EXIT_FAILURE;
	
	fits_get_img_param(fptr, 2,  &bitpix, \
						&naxis, naxes, &status);				// Read header
	if (status > 0)
		return EXIT_FAILURE;
	
	fits_read_pix(fptr, TFLOAT, fpixel, \
						nelements, &nulval, image, \
						&anynul, &status);						// Read image to image
	if (status > 0)
		return EXIT_FAILURE;
	
	logDebug("Read image, status: %d bitpix: %d, pixel (100,100): %f", status, \
		bitpix, image[100 * res[1]]);
	
	fits_close_file(fptr, &status);
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
	shsize[0] = ptc.wfs[0].res[0]/simparams.shcells[0];
	shsize[1] = ptc.wfs[0].res[1]/simparams.shcells[1];
	
	// TODO: again we only support float images here :<
	float *subapt;
	subapt = calloc((shsize[0] * 2 + 2) * (shsize[1] * 2 + 2), sizeof(subapt)); // We store the subaperture here, +2 is to make sure we 
																// have enough space on both sides lateron (even & odd sized SHcells)
	
	// 
	// if (shsize*SHsens NE ssize[0]) then $
	// 	PRINT,'WARNING: incomplete SH cell coverage!'
	
	if (simparams.shcells[0] * shsize[0] != ptc.wfs[0].res[0] || \
		simparams.shcells[1] * shsize[1] != ptc.wfs[0].res[1])
		logErr("Incomplete SH cell coverage!");

	// 
	// im = COMPLEX(0,1)
	// 
	// ;;zero pads  (+1 is to be sure, with odd numbers)
	// zeroside = DBLARR(shsize/2+1,shsize)
	// zerotop = DBLARR((shsize/2+1)*2+shsize,shsize/2+1)
	// 
	int x, y;
	int xx, yy;
	for (x=0; x < simparams.shcells[0]; x++) {
		for (y=0; y < simparams.shcells[1]; y++) {
			// Copy the subaperture to subapt
			for (xx=0; xx < shsize[0]; xx++) { // TODO: do these boundaries work properly?
				for (yy=0; yy < shsize[1]; yy++) {
					subapt[(yy+shsize[1]/2)*(shsize[0] * 2 + 2) + (xx+shsize[0]/2)] = \
						ptc.wfs[0].image[y*shsize[0]*ptc.wfs[0].res[0] + x*shsize[1] + yy*shsize[0] + xx];
					// TODO: split this up in readable bits...
				}
			}
		 

		}
	}
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
		
	return EXIT_SUCCESS;
}

int modCalcDMVolt() {
	logDebug("Calculating DM voltages");
	return EXIT_SUCCESS;
}

int displayImg(float *img, long res[2]) {
	// ONLY does float images
	int x, y;
	float max=img[0];
	float min=img[0];
	
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
			DrawPixel(screen, x, y, \
				(int) ((img[y*res[0] + x]-min)/(max-min)*256), \
				0, \
				0);
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
