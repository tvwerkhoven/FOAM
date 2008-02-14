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
		
	fprintf(fp,"%d\n", ptc->wfs[wfs].nsubap * 2);

	// collect displacement vectors and store as reference
	logInfo("Found following reference coordinates:");
	for (j=0; j < ptc->wfs[wfs].nsubap; j++) {
		ptc->wfs[wfs].refc[j][0] = ptc->wfs[wfs].disp[j][0];
		ptc->wfs[wfs].refc[j][1] = ptc->wfs[wfs].disp[j][1];
		logDirect("(%f,%f) ", ptc->wfs[wfs].refc[j][0], ptc->wfs[wfs].refc[j][1]);
		fprintf(fp,"%f %f\n", (double) ptc->wfs[wfs].refc[j][0], (double) ptc->wfs[wfs].refc[j][1]);
	}
	// set the rest to zero
	// for (j=ptc->wfs[wfs].nsubap; j < ptc->wfs[wfs].cells[0] * ptc->wfs[wfs].cells[1]; j++)
	// 	fprintf(fp,"%f %f\n", (double) 0, (double) 0);
	
	logDirect("\n");
	
	fclose(fp);
	return EXIT_SUCCESS;
}

// TODO: document
// TODO: might need to set EVERY wfc to zero, instead of just the one
//       being measured
int modCalWFC(control_t *ptc) {
	int j, i, k, skip;
	int nact=0;			// total nr of acts
	int nsubap=0;		// total nr of subapts
	int wfc, wfs; 		// counters to loop over the WFSs and WFCs
	float origvolt;
	int measurecount=1;
	int skipframes=1;
	FILE *fp;
	char *filename;
	
	logDebug("Starting WFC calibration");
	
	// run openInit once to read the sensors and get subapertures
	modOpenInit(ptc);

	// get total nr of actuators, set all actuators to zero (180 volt for an okotech DM)
	for (i=0; i < ptc->wfc_count; i++) {
		for (j=0; j < ptc->wfc[i].nact; j++) {
			ptc->wfc[i].ctrl[j] = 0;
		}
		nact += ptc->wfc[i].nact;
	}
	
	// get total nr of subapertures in the complete system
	for (i=0; i < ptc->wfs_count; i++)
		nsubap += ptc->wfs[i].nsubap;

	// this stores some vectors :P
	float q0x[nsubap], q0y[nsubap];
			
	if (asprintf(&filename, "%s_nact%d_nsens%d.txt", FOAM_MODCALIB_DMIF, nact, nsubap) < 0)
		logErr("Failed to construct filename.");
			
	logInfo("Calibrating WFC's using %d actuators and %d subapts, storing in %s.", nact, nsubap, filename);
	fp = fopen(filename,"w+");
	fprintf(fp,"%d\n%d\n", nact, nsubap*2); // we have nact actuators (variables) and nsubap*2 measurements
	
	for (wfc=0; wfc < ptc->wfc_count; wfc++) { // loop over all wave front correctors 
		for (wfs=0; wfs < ptc->wfs_count; wfs++) { // loop over all wave front sensors
			nact = ptc->wfc[wfc].nact;
			nsubap = ptc->wfs[wfs].nsubap;
			for (j=0; j<nact; j++) { /* loop over all actuators  and all subapts for (wfc,wfs) */
		
				for (i=0; i<nsubap; i++) { // set averaging buffer to zero
					q0x[i] = 0.0;
					q0y[i] = 0.0;
				}
	
				logInfo("Act %d/%d (WFC %d/%d)", j+1, ptc->wfc[wfc].nact, wfc+1, ptc->wfc_count);
		
				origvolt = ptc->wfc[wfc].ctrl[j];
		
				for (k=0; k < measurecount; k++) {

					// we set the voltage and then set the actuators manually
					ptc->wfc[wfc].ctrl[j] = DM_MAXVOLT;
					
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
			
					ptc->wfc[wfc].ctrl[j] = DM_MINVOLT;
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
		
				ptc->wfc[wfc].ctrl[j] = origvolt;
			} // end loop over actuators
		} // end loop over wfss
	} // end loop over wfcs
	fclose(fp);
	logInfo("WFC %d (%s) influence function saved for in file %s", wfc, ptc->wfc[wfc].name, filename);
	
	return EXIT_SUCCESS;
}
