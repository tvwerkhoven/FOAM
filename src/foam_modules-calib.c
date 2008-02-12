/*! 
	@file foam_modules-calib.c
	@author @authortim
	@date 2008-02-07

	@brief This file is used to calibrate actuators like the DM and TT mirrors used in AO systems.
	Works on any combination of WFC and WFS, and uses generic controls in the [-1,1] range
	
	TODO: document further
	
*/

#define DM_MAXVOLT 1	// these are normalized and converted to the right values by drvSetActuator()
#define DM_MIDVOLT 0
#define DM_MINVOLT -1
#define FOAM_MODCALIB_DMIF "../config/ao_dmif"
#define FOAM_MODCALIB_PINHOLE "../config/ao_pinhole"

// TODO: document
int modCalPinhole(control_t *ptc, int wfs) {
	int i, j;
	FILE *fp;
	
	logDebug("Performing pinhole calibration for WFS %d", wfs);
		
	// set filterwheel to pinhole
	drvFilterWheel(ptc, FILT_PINHOLE);
	
	// set control vector to zero (+-180 volt for an okotech DM)
	for (i=0; i < ptc->wfc_count; i++)
		for (j=0; j < ptc->wfc[i].nact; j++)
			ptc->wfc[i].ctrl[j] = 0;
		
	// run open loop initialisation once
	modOpenInit(ptc);
	
	// run open loop once
	modOpenLoop(ptc);
	
	// open file to store vectors
	fp = fopen(ptc->wfs[wfs].pinhole,"w+");
	if (fp == NULL)
		logErr("Error opening pinhole calibration file for wfs %d (%s)", wfs, strerror(errno));
		
	fprintf(fp,"%d\n", ptc->wfs[wfs].nsubap*2);

	// collect displacement vectors and store as reference
	logInfo("Found following reference coordinates:");
	for (j=0; j < ptc->wfs[wfs].nsubap; j++) {
		ptc->wfs[wfs].refc[j][0] = ptc->wfs[wfs].disp[j][0];
		ptc->wfs[wfs].refc[j][1] = ptc->wfs[wfs].disp[j][1];
		logDirect("(%f,%f) ", ptc->wfs[wfs].refc[j][0], ptc->wfs[wfs].refc[j][1]);
		fprintf(fp,"%.12g\n%.12g\n", (double) ptc->wfs[wfs].refc[j][0], (double) ptc->wfs[wfs].refc[j][1]);
	}
	logDirect("\n");
	
	fclose(fp);
	return EXIT_SUCCESS;
}

// TODO: document
// TODO: might need to set EVERY wfc to zero, instead of just the one
//       being measured
int modCalWFC(control_t *ptc, int wfc, int wfs) {
	int j, i, k, skip;
	int nact = ptc->wfc[wfc].nact;
	int nsubap = ptc->wfs[wfs].nsubap;
	float *ctrl = ptc->wfc[wfc].ctrl;
		
	float q0x[nsubap], q0y[nsubap];
	float origvolt;
	int measurecount=1;
	int skipframes=1;
	FILE *fp;
	char *filename;
		
	if (asprintf(&filename, "%s_wfs%d_wfc%d.txt", FOAM_MODCALIB_DMIF, wfs, wfc) < 0)
		logErr("Failed to construct filename.");
		
	logInfo("Calibrating WFC %d for WFS %d, storing in %s.", wfc, wfs, filename);
	fp = fopen(filename,"w+");
	fprintf(fp,"%d\n%d\n", nact, nsubap*2);
	
	// setting all actuators to zero (180 volt for an okotech DM)
	for (j=0; j<nact; j++)
		ctrl[j] = DM_MIDVOLT;
	
	modOpenInit(ptc);
	
	for (j=0; j<nact; j++) { /* loop over all actuators */
		
		for (i=0; i<nsubap; i++) { // set averaging buffer to zero
			q0x[i] = 0.0;
			q0y[i] = 0.0;
		}
	
		logInfo("Act %d/%d",j, nact);
		
		origvolt = ctrl[j];
		
		for (k=0; k < measurecount; k++) {

			// we set the voltage and then set the actuators manually
			ctrl[j] = DM_MAXVOLT;
			drvSetActuator(ptc, wfc);
			// run the open loop *once*, which will approx do the following:
			// read sensor, apply some SH WF sensing, set actuators, 
			// TODO: open or closed loop? --> open?
			
			for (skip=0; skip<skipframes+1; skip++) // skip some frames here
				modOpenLoop(ptc);
			
			// take the shifts and store those (wrt to reference shifts)
    		for (i=0;i<nsubap;i++) { 	
				q0x[i] += (ptc->wfs[wfs].disp[i][0] - ptc->wfs[wfs].refc[i][0]) / (float) measurecount;
				q0y[i] += (ptc->wfs[wfs].disp[i][1] - ptc->wfs[wfs].refc[i][1]) / (float) measurecount;
			}
			
			ctrl[j] = DM_MINVOLT;
			drvSetActuator(ptc, wfc);			
			for (skip=0; skip<skipframes; skip++) // skip some frames here
				modOpenLoop(ptc);
			
			// take the shifts and subtract those store those (wrt to reference shifts)
    		for (i=0;i<nsubap;i++) { 
				q0x[i] -= (ptc->wfs[wfs].disp[i][0] - ptc->wfs[wfs].refc[i][0]) / (float) measurecount;
				q0y[i] -= (ptc->wfs[wfs].disp[i][1] - ptc->wfs[wfs].refc[i][1]) / (float) measurecount;
			}

		} // end measurecount loop

		// store the measurements for actuator j (for all subapts) 
		for (i=0; i<nsubap; i++) { // set averaging buffer to zero
			fprintf(fp,"%.12g\n%.12g\n", (double) q0x[i], (double) q0y[i]);
		}
		
		origvolt = ctrl[j];
	} // end loop over actuators
	fclose(fp);
	logInfo("WFC %d (%s) influence function saved for in file %s", wfc, ptc->wfc[wfc].name, filename);
	
	return EXIT_SUCCESS;
}
