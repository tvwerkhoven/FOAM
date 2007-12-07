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
#include "fitsio.h"

// GLOBAL VARIABLES //
/********************/	

// These are defined in cs_library.c
extern config_t cs_config;
extern control_t ptc;

static int simObj(char *file);
static int simAtm(char *file);
static int simTel(char *file);
static int simWFC(int wfcid);

// We need these every time...
char errmsg[FLEN_STATUS];
int status = 0;  /* MUST initialize status */
float nulval = 0.0;
int anynul = 0;

int drvReadSensor() {
	int i;
	logDebug("Now reading %d sensors.", ptc.wfs_count);
	// TODO: simulate object, atmosphere, telescope, TT, DM, WFS (that's a lot :O)
	
	if (ptc.wfs_count < 1) {
		logErr("Nothing to process, no WFSs defined.");
		return EXIT_FAILURE;
	}
	
	logDebug("Parsing WFS %s (ptc poinst to %p and %p)", ptc.wfs[0].name, ptc, ptc.wfs);
	
	if (simAtm("wavefront.fits") != EXIT_SUCCESS) { // This simulates the point source + atmosphere (from wavefront.fits)
		if (status > 0) {
			fits_get_errstatus(status, errmsg);
			logErr("fitsio error in simAtm(): %s", errmsg);
			status = 0;
		}
		else logErr("error in simAtm().");
	}
	
	if (simTel("aperture.fits") != EXIT_SUCCESS) { // Simulate telescope (from aperture.fits)
		if (status > 0) {
			fits_get_errstatus(status, errmsg);
			logErr("fitsio error in simTel(): %s", errmsg);
			status = 0;
		}
		else logErr("error in simTel().");
	}
	
	logDebug("Now simulating %d WFC(s).", ptc.wfc_count);
	for (i=0; i < ptc.wfc_count; i++) 
		simWFC(i); // Simulate TT mirror
	
	return EXIT_SUCCESS;
}

static int simObj(char *file) {
	return EXIT_SUCCESS;
}

static int simWFC(int wfcid) {
	// we want to simulate the tip tilt mirror here. What does it do
	logDebug("WFC %d (%s) has  actuators.", wfcid, ptc.wfs[0].name);
	// TODO: bus error on ptc.wfc[wfcid].name ? seems like a pointer?
	
	return EXIT_SUCCESS;
}

static int simTel(char *file) {
	fitsfile *fptr;
	int bitpix, naxis, i;
	long naxes[2], fpixel[] = {1,1};
	long nelements = ptc.wfs[0].resx * ptc.wfs[0].resy;
	float *aperture;
	if ((aperture = malloc(nelements * sizeof(aperture))) == NULL)
		return EXIT_FAILURE;
	
	fits_open_file(&fptr, file, READONLY, &status);				// Open FITS file
	if (status > 0)
		return EXIT_FAILURE;
	
	fits_get_img_param(fptr, 2,  &bitpix, \
						&naxis, naxes, &status);				// Read header
	if (status > 0)
		return EXIT_FAILURE;
	if (naxes[0] * naxes[1] != nelements) {
		logErr("Error in simTel(), fitsfile not the same dimension as ptc.wfs[0].image (%dx%d vs %dx%d)", \
		naxes[0], naxes[1], ptc.wfs[0].resx, ptc.wfs[0].resy);
		return EXIT_FAILURE;
	}
					
	fits_read_pix(fptr, TFLOAT, fpixel, \
						nelements, &nulval, aperture, \
						&anynul, &status);						// Read image to ptc.wfs[0].image
	if (status > 0)
		return EXIT_FAILURE;
	
	logDebug("Aperture read successfully, processing with image.");
	
	// Multiply wavefront with aperture
	for (i=0; i < nelements; i++)
		ptc.wfs[0].image[i] *= aperture[i];
	
	return EXIT_SUCCESS;
}

static int simAtm(char *file) {
	// ASSUMES WFS_COUNT > 0
	// ASSUMES ptc.wfs[0].image is initialized to float 
	fitsfile *fptr;
	int bitpix, naxis;
	long naxes[2], fpixel[] = {1,1};
	long nelements = ptc.wfs[0].resx * ptc.wfs[0].resy;
	
	fits_open_file(&fptr, file, READONLY, &status);				// Open FITS file
	if (status > 0)
		return EXIT_FAILURE;
	
	fits_get_img_param(fptr, 2,  &bitpix, \
						&naxis, naxes, &status);				// Read header
	if (status > 0)
		return EXIT_FAILURE;
					
	fits_read_pix(fptr, TFLOAT, fpixel, \
						nelements, &nulval, ptc.wfs[0].image, \
						&anynul, &status);						// Read image to ptc.wfs[0].image
	if (status > 0)
		return EXIT_FAILURE;
	
	logDebug("Read image, status: %d bitpix: %d, pixel (100,100): %f", status, \
		bitpix, ptc.wfs[0].image[100 * ptc.wfs[0].resy]);
	
	return EXIT_SUCCESS;	
}

int drvSetActuator() {
	int i;
	
	logDebug("%d WFCs to set.", ptc.wfc_count);
	
	for(i=0; i < ptc.wfc_count; i++)  {
		logDebug("Setting WFC %d with %d acts.", i, ptc.wfc[i].nact);
	}
	return EXIT_SUCCESS;
}

int modParseSH() {
	logDebug("Parsing SH WFSs now.");
	
	return EXIT_SUCCESS;
}

int modCalcDMVolt() {
	logDebug("Calculating DM voltages");
	return EXIT_SUCCESS;
}