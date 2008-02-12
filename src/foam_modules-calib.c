/*! 
	@file foam_modules-calib.c
	@author @authortim
	@date 2008-02-07

	@brief This file is used to calibrate actuators like the DM and TT mirrors used in AO systems.
	Works on any combination of WFC and WFS, and uses generic controls in the [-1,1] range
	
	TODO: document further
	
*/

// TODO: document
// TODO: what arguments do we *at least* need?
// TODO: only works with ONE wfs (wfs 0)
int modCalWFC(control_t *ptc, int wfc, int wfs) {
	logDebug("now in calib module");
	int j, i, k, skip;
	int nact = ptc->wfc[wfc].nact;
	int nsubap = ptc->wfs[wfs].nsubap;
	float *ctrl = ptc->wfc[wfc].ctrl;
		
	float q0x[nsubap], q0y[nsubap];
	float origvolt;
	int measurecount=10;
	int skipframes=1;
	FILE *fp;
	char *filename;
	logDebug("now in calib module");
		
	asprintf(&filename, "%s_wfs%d_wfc%d.txt", FOAM_MODCALIB_DMIF_PRE, wfs, wfc);
	logInfo("Calibrating WFC %d for WFS %d, storing in %s.", wfc, wfs, filename);
	fp = fopen(filename,"w+");
	fprintf(fp,"%d\n%d\n", nact, nsubap*2);
	
	for (j=0; j<nact; j++) { /* loop over all actuators */
		
		for (i=0; i<nsubap; i++) { // set averaging buffer to zero
			q0x[i] = 0.0;
			q0y[i] = 0.0;
		}
	
		logInfo("Measuring actuator %d",j);
		
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
			
			// take the shifts and store those
			// SPEED: we could divide after the loop, slightly faster
    		for (i=0;i<nsubap;i++) { 
				q0x[i] += ptc->wfs[wfs].subc[i][0] / (float) measurecount;
				q0y[i] += ptc->wfs[wfs].subc[i][1] / (float) measurecount;
			}
			
			ctrl[j] = DM_MINVOLT;
			drvSetActuator(ptc, wfc);			
			for (skip=0; skip<skipframes; skip++) // skip some frames here
				modOpenLoop(ptc);
			
			// take the shifts and store those
			// SPEED: we could divide after the loop, slightly faster
    		for (i=0;i<nsubap;i++) { 
				q0x[i] -= ptc->wfs[wfs].subc[i][0] / (float) measurecount;
				q0y[i] -= ptc->wfs[wfs].subc[i][1] / (float) measurecount;
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
