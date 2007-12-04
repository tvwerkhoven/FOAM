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
#include "qfits.h"

// GLOBAL VARIABLES //
/********************/	

// These are defined in cs_library.c
extern control_t ptc;
extern config_t cs_config;

static int simObj(char *file);
static int simAtm();
static int simTel();
	
int drvReadSensor() {
	logDebug("Now reading %d sensors.", ptc.wfs_count);
	// TODO: simulate object, atmosphere, telescope, TT, DM, WFS (that's a lot :O)

	if (ptc.wfs_count < 1) {
		logErr("Nothing to process, no WFSs defined.");
		return EXIT_FAILURE;
	}
	
	simObj("test.fits"); // This simulates the object
//	simAtm();			// Simulate atmosphere ->
//	simTel();			// Simulate telescope (circular aperture)
	
	return EXIT_SUCCESS;
}

static int simAtm() {
	return EXIT_SUCCESS;
}

static int simTel() {
	return EXIT_SUCCESS;
}

static int simObj(char *file) {
	// ASSUMES WFS_COUNT > 0
	// ASSUMES ptc.wfs[0].image HAS THE SAME RESOLUTION AS THE FITSFILE
//	int i,j;
//	int h=ptc.wfs[0].resy;
//	int w=ptc.wfs[0].resx;
	int bitpix;
	qfits_header *header;
	qfitsloader ql;
	
/*	header = qfits_header_read(file); 							// Get the header
	
	if ((ptc.wfs[0].resy = qfits_header_getint(header, "NAXIS", 0)) <= 0) {
		logErr("could not determin NAXIS of FITS file %s, appears to be malformed.", file);
		return EXIT_FAILURE;
	}
	
	if ((ptc.wfs[0].resy = qfits_header_getint(header, "NAXIS1", 0)) <= 0) {
		logErr("could not determine pixel height of FITS file %s.", file);
		return EXIT_FAILURE;
	}
	if ((ptc.wfs[0].resx = qfits_header_getint(header, "NAXIS2", 0)) <= 0) {
		logErr("could not determine pixel height of FITS file %s.", file);
		return EXIT_FAILURE;
	}

	bitpix = abs(qfits_header_getint(header, "BITPIX", 0));		// Get the datatype TODO: is this required?
	
	if (bitpix == 8)
    	ql.ptype = PTYPE_INT;
	else if (bitpix == 16)
    	ql.ptype = PTYPE_FLOAT;
	else if (bitpix == 32)
    	ql.ptype = PTYPE_DOUBLE;
	else {
		logErr("In fits file %s, unknown bitpix (not in 8, 16, 32): %d.", file, bitpix);
		return EXIT_FAILURE;
	}
		
	// Initialize loader struct
    ql.filename = file;
    ql.xtnum = 0;
    ql.pnum = 0;
    ql.map = 1;

    if (qfitsloader_init(&ql) != 0) { // This writes somewhere it shouldn't..., right?
        logErr("Cannot initialize loader on FITS file '%s'.", file);
        return EXIT_FAILURE;
    }

	logDebug("Loading pix buffer.");
	
    if (qfits_loadpix(&ql) != 0) {
        logErr("cannot load data from FITS file '%s'.", file);
        return EXIT_FAILURE;
    }

	// FITS File loaded in ql.fbuf, 1D array
	// Link image to loaded fbuf:
	
//	free(ptc.wfs[0].image);
//	ptc.wfs[0].image = ql.fbuf;
	
	logInfo("Fits file loaded successfully.");
*/
	return EXIT_SUCCESS;	
	// we want to read fits stuff here
	// header = fitsrhead(file, &lhead, &nbhead);
	
	// FOR THE TIME BEING, WE MAKE AN IMAGE OF THE LETTER 'T'
	/*for(j=h*3/20; j < h*5/20; j++){
		for(i=w*1/4; i<w * 4/3; i++) {
			ptc.wfs[0].image[i+j*w] = 1; // 1D arr
		}
	}
	
	for(j=h*1/4; j <h*3/4; j++) {
		for(i=w*9/20; i<w * 11/20; i++) {
			ptc.wfs[0].image[i+j*w] = 1;
		}
	}*/

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