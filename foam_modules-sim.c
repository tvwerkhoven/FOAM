/*! 
	@file foam_modules-sim.c
	@author Tim van Werkhoven
	@date November 30 2007

	@brief This file contains the functions to run FOAM in simulation mode.
*/

#include "fitsfile.h" // for fitsfile.c

int drvReadSensor() {
	logDebug("Now reading %d sensors.", ptc.wfs_count);
	// TODO: simulate object, atmosphere, telescope, TT, DM, WFS (that's a lot :O)

	if (ptc.wfs_count < 1) {
		logErr("Nothing to process, no WFSs defined.");
		return EXIT_FAILURE;
	}
	
	_simObj("testfile.fits"); // This simulates the object
	
	return EXIT_SUCCESS;
}

int _simObj(char *file) {
	// ASSUMES WFS_COUNT > 0
	// ASSUMES ptc.wfs[0].image HAS THE SAME RESOLUTION AS THE FITSFILE
	int lhead, nbhead;
	char *header;
	
	// we want to read fits stuff here
	header = fitsrhead(image, &lhead, &nbhead);
	
	ptc.wfs[0].image = fitsrfull(image, nbhead, header);

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