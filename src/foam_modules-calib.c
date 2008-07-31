/*
 Copyright (C) 2008 Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)
 
 This file is part of FOAM.
 
 FOAM is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 FOAM is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with FOAM.  If not, see <http://www.gnu.org/licenses/>.
 */
/*! 
	@file foam_modules-calib.c
	@author @authortim
	@date 2008-07-15

	@brief This file is used to calibrate actuators like the DM and TT mirrors used in AO systems.

	\section Info
	
	The calibration works on any combination of all WFCs and one WFS, and uses 
	generic controls in the [-1,1] range. These controls must be translated to 
	actual voltages by other routines in other modules. These must also take care 
	that the range [-1, 1] is linear. This module only provides SH WFS calibration.
 
	All WFC's are grouped together during calibration, and the influence function
	for all actuators is measured per WFS. This is done because actual wavefront
	correction is WFS driven: usually a camera runs at a certain speed and at
	each new frame we want to give new control signals to the WFC's. This way we
	can do that nicely. MCAO is not included in this analysis.
	
	\section Functions	
	
	The functions provided to the outside world are:
 
	\li calibPinhole() - Calibrate pinhole coordinates, used as a reference when correcting
	\li calibPinholeChk() - Checks whether pinhole calibration has been performed, and loads calibration data.
	\li calibWFC() - Measures the WFC influence function and inverts this. This calibration is done per WFS for all WFC's.
	\li calibWFCChk() - Checks whether WFC influence function calibration has been performed, and loads calibration data.

	\section Dependencies
	
	This module depends on the shack-hartmann module (modules-sh), as the calibrations are SH-specific.
 
	This module expects that a modOpenInit, modOpenLoop and modOpenFinish 
	sequence does at least the following: startup any camera used as SH WFS,
	measure the offets in all available subapertures, if any, shutdown the 
	camera. It also assumes that these functions do not alter the hardware setup
	or change the actuator voltages. See calibPinhole() and calibWFC() for
	details.
*/

// HEADERS //
/***********/

#include "foam_modules-calib.h"

// ROUTINES //
/************/

int calibPinhole(control_t *ptc, int wfs, mod_sh_track_t *shtrack) {
	int i, j;
	FILE *fd;
	
	if (shtrack->nsubap == 0) {
		logWarn("Cannot calibrate reference coordinates if subapertures are not selected yet");
		return EXIT_FAILURE;
	}
	
	// Set filterwheel to pinhole and set voltages to 180V. This must be done
	// by the prime module and is provided through the function 
	// drvSetupHardware().
	if (drvSetupHardware(ptc, ptc->mode, ptc->calmode) != EXIT_SUCCESS) {
		logWarn("Could not setup hardware for pinhole calibration");
		return EXIT_FAILURE;
	}
	
	// set all actuators to the center
	for (i=0; i < ptc->wfc_count; i++) {
		gsl_vector_float_set_zero(ptc->wfc[i].ctrl);
		drvSetActuator(ptc, i);
	}
		
	// This is the only way I can think of now to reliably get new data in 
	// our buffer. This requires that a modOpenInit, modOpenLoop and modOpenFinish
	// pair at least does the following: start camera, get image, calculate 
	// displacement, and does not modify the hardware in any way (TT, DM).
	// This does not seem like a stringent requirement so I'm leaving it at this.
	if (modOpenInit(ptc) != EXIT_SUCCESS || modOpenLoop(ptc) == EXIT_FAILURE ||
		modOpenFinish(ptc) != EXIT_SUCCESS) {
		logWarn("Could not run an open loop for pinhole calibration");
		return EXIT_FAILURE;
	}
	
	// collect displacement vectors and store as reference
	logInfo(0, "Found following reference coordinates:");
	for (j=0; j < shtrack->nsubap; j++) {
		gsl_vector_float_set(shtrack->refc, 2*j+0, gsl_vector_float_get(shtrack->disp, 2*j+0)); // x
		gsl_vector_float_set(shtrack->refc, 2*j+1, gsl_vector_float_get(shtrack->disp, 2*j+1)); // y
				
		logInfo(LOG_NOFORMAT, "(%f,%f) ", gsl_vector_float_get(shtrack->refc, 2*j+0), gsl_vector_float_get(shtrack->refc, 2*j+1));
	}	
	logInfo(LOG_NOFORMAT, "\n");
	
	// storing to file:
	fd = fopen(shtrack->pinhole, "w+");
	if (!fd) logWarn("Could not open file %s: %s", shtrack->pinhole, strerror(errno));
	gsl_vector_float_fprintf(fd, shtrack->refc, "%.10f");
	fclose(fd);
	logInfo(0, "Successfully stored reference coordinates to %s.", shtrack->pinhole);
	
	return EXIT_SUCCESS;
}

int calibPinholeChk(control_t *ptc, int wfs, mod_sh_track_t *shtrack) {
	FILE *fd;
	
	fd = fopen(shtrack->pinhole, "r");
	if (!fd) {
		logWarn("Could not open file %s: %s", shtrack->pinhole, strerror(errno));
		return EXIT_FAILURE;
	}
	gsl_vector_float_fscanf(fd, shtrack->refc);
	fclose(fd);
	
	return EXIT_SUCCESS;
}

int calibWFC(control_t *ptc, int wfs, mod_sh_track_t *shtrack) {
	int j, i, k, skip;
	int nact=0, nacttot=0;			// total nr of acts for all WFCs
	int nsubap=0; 		// nr of subapts for specific WFS
	int wfc; 			// counters to loop over the WFCs
	float origvolt;
	FILE *fd;
	char *file;
	gsl_matrix_float *infl;
	
	if (shtrack->nsubap == 0) {
		logWarn("Cannot calibrate influence function if subapertures are not selected yet");
		return EXIT_FAILURE;
	}
	
	logInfo(0, "Starting WFC influence function calibration for WFS %d", wfs);
	
	// delete SVD files:
	asprintf(&file, "%s-singular", shtrack->influence);
	if (unlink(file) != 0) logWarn("Problem removing old SVD files: %s", strerror(errno));
	asprintf(&file, "%s-wfsmodes", shtrack->influence);
	if (unlink(file) != 0) logWarn("Problem removing old SVD files: %s", strerror(errno));
	asprintf(&file, "%s-dmmodes", shtrack->influence);
	if (unlink(file) != 0) logWarn("Problem removing old SVD files: %s", strerror(errno));
		
	// set hardware correctly for this calibration
	drvSetupHardware(ptc, ptc->mode, ptc->calmode);
		
	// get total nr of actuators, set all actuators to zero (180 volt for an okotech DM)
	for (i=0; i < ptc->wfc_count; i++) {
		gsl_vector_float_set_zero(ptc->wfc[i].ctrl);
		drvSetActuator(ptc, i);
		nacttot += ptc->wfc[i].nact;
	}
	
	// init the open loop, which starts the camera etc
	modOpenInit(ptc);
	
	// total subapts for WFS
	nsubap = shtrack->nsubap;
	float q0x[nsubap], q0y[nsubap];

	logDebug(0, "Allocating temporary matrix to store influence function (%d x %d)", nsubap*2, nacttot);
	// this will store the influence matrix (which calculates displacements given actuator signals)
	infl = gsl_matrix_float_calloc(nsubap*2, nacttot);
				
	logInfo(0, "Calibrating WFCs using %d actuators and WFS %d with %d subapts, storing in %s.", \
		nacttot, wfs, nsubap, shtrack->influence);
	logInfo(0, "Measuring each act %d times, skipping %d frames each time.", \
			shtrack->measurecount, shtrack->skipframes);
			
	for (wfc=0; wfc < ptc->wfc_count; wfc++) { // loop over all wave front correctors 
		nact = ptc->wfc[wfc].nact;
		
		logInfo(0, "Startin WFC %d calibration with calibration range: (%.2f, %.2f)", \
				wfc, ptc->wfc[wfc].calrange[0], ptc->wfc[wfc].calrange[1]);
		
		for (j=0; j<nact; j++) { // loop over all actuators  and all subapts for (wfc,wfs)
	
			for (i=0; i<nsubap; i++) { // set averaging buffer to zero
				q0x[i] = 0.0;
				q0y[i] = 0.0;
			}

			logInfo(0, "Act %d/%d (WFC %d/%d)", j+1, ptc->wfc[wfc].nact, wfc+1, ptc->wfc_count);
	
			origvolt = gsl_vector_float_get(ptc->wfc[wfc].ctrl, j);
	
			for (k=0; k < shtrack->measurecount; k++) {

				// we set the voltage and then set the actuators manually
				gsl_vector_float_set(ptc->wfc[wfc].ctrl, j, ptc->wfc[wfc].calrange[1]);
                drvSetActuator(ptc, wfs);
				
				// run the open loop a few times, which will approx do the following:
				// read sensor, apply some SH WF sensing
		
				for (skip=0; skip< shtrack->skipframes + 1; skip++) { // skip some frames here
					modOpenLoop(ptc);
					logInfo(LOG_NOFORMAT, ".");
				}
				
		
				// take the shifts and store those (wrt to reference shifts)
	    		for (i=0;i<nsubap;i++) { 	
					q0x[i] += gsl_vector_float_get(shtrack->disp, 2*i+0) / (float) shtrack->measurecount; // x
					q0y[i] += gsl_vector_float_get(shtrack->disp, 2*i+1) / (float) shtrack->measurecount; // y
				}
		
				gsl_vector_float_set(ptc->wfc[wfc].ctrl, j, ptc->wfc[wfc].calrange[0]);				
                drvSetActuator(ptc, wfc);
						
				for (skip=0; skip< shtrack->skipframes +1; skip++) { // skip some frames here
					modOpenLoop(ptc);
					logInfo(LOG_NOFORMAT, ".");
				}
				logInfo(LOG_NOFORMAT, "\n");
		
				// take the shifts and subtract those store those (wrt to reference shifts)
	    		for (i=0;i<nsubap;i++) { 
					q0x[i] -= gsl_vector_float_get(shtrack->disp, 2*i+0) / (float) shtrack->measurecount; // x
					q0y[i] -= gsl_vector_float_get(shtrack->disp, 2*i+1) / (float) shtrack->measurecount; // y
				}
			} // end measurecount loop

			// store the measurements for actuator j (for all subapts) 
			
			for (i=0; i<nsubap; i++) {				
				gsl_matrix_float_set(infl, 2*i+0, j, (float) q0x[i]/(ptc->wfc[wfc].calrange[1] - ptc->wfc[wfc].calrange[0]));
				gsl_matrix_float_set(infl, 2*i+1, j, (float) q0y[i]/(ptc->wfc[wfc].calrange[1] - ptc->wfc[wfc].calrange[0]));
			}
	
			// restore original actuator voltage
			gsl_vector_float_set(ptc->wfc[wfc].ctrl, j, origvolt);
		} // end loop over actuators
	} // end loop over wfcs
	
	// stop the open loop
	modOpenFinish(ptc);
	
	fd = fopen(shtrack->influence, "w+");
	if (!fd) logErr("Could not open file %s: %s", shtrack->influence, strerror(errno));
	gsl_matrix_float_fprintf(fd, infl, "%.10f");
	fclose(fd);
	
	// save metadata too (nact, n measurements, nsubap);
	asprintf(&file, "%s-meta", shtrack->influence);
	fd = fopen(file, "w+");
	if (!fd) logErr("Could not open file for saving metadata %s: %s", file, strerror(errno));
	fprintf(fd, "%d\n%d\n%d\n", nacttot, nsubap, 2*nsubap);
	fclose(fd);

	logInfo(0, "WFS %d (%s) influence function successfully saved for in file %s", wfs, ptc->wfs[wfs].name, shtrack->influence);
	
	calibSVDGSL(ptc, wfs, shtrack);
	
	return EXIT_SUCCESS;
}

int calibWFCChk(control_t *ptc, int wfs, mod_sh_track_t *shtrack) {
	int i, chknact, chksubap;
	int nsubap = shtrack->nsubap, nacttot=0;
	char *outfile;
	FILE *fd;
	
	logInfo(0, "Checking if influence function calibration can be loaded from files");
	// calculate total nr of act for all wfc
	for (i=0; i< ptc->wfc_count; i++)
		nacttot += ptc->wfc[i].nact;
	
	if (shtrack->nsubap == 0) {
		logWarn("Cannot load influence function if subapertures are not selected yet");
		return EXIT_FAILURE;
	}
	
	// CHECK GEOMETRY //
	////////////////////
	asprintf(&outfile, "%s-meta", shtrack->influence);
	fd = fopen(outfile, "r");
	if (!fd) {
		logWarn("Could not open file %s: %s", outfile, strerror(errno));
		return EXIT_FAILURE;
	}
	if (fscanf(fd, "%d\n%d\n", &chknact, &chksubap) != 2) {
		logWarn("Could not read metadata from %s on calibration, please re-calibrate.", outfile);
		return EXIT_FAILURE;
	}
	if (chknact != nacttot || chksubap != nsubap) {
		logWarn("Calibration appears to be old, please re-calibrate");
		logWarn("Calibration parameters do not match current system parameters:");
		logWarn("# act: file: %d current: %d, nsubap: file: %d current: %d", chknact, nacttot, chksubap, nsubap);
		return EXIT_FAILURE;
	}
		
	// CHECK SINGVAL //
	///////////////////
	asprintf(&outfile, "%s-singular", shtrack->influence);
	
	// read vector from file (should be made by calibSVDGSL)
	fd = fopen(outfile, "r");
	if (!fd) {
		logWarn("Could not open singular values file %s: %s", outfile, strerror(errno));
		return EXIT_FAILURE;
	}
	// We set up SVD data (wfsmodes, singular, dmmodes) per WFS, i.e.
	// we take all WFCs together for each WFS, measure the shifts per WFS
	// and then calculate the controls from those shifts. To do this, we 
	// setup data for a WFS, Sum(WFC's) pair, and we need to total nr of 
	// actuators in the system
	if (shtrack->singular == NULL) {
		shtrack->singular = gsl_vector_float_calloc(nacttot);
		if (shtrack->singular == NULL) logErr("Failed to allocate memory for singular values vector.");
	}
	logDebug(0, "Reading singular values into memory from %s now...", outfile);
	gsl_vector_float_fscanf(fd, shtrack->singular);
	fclose(fd);


	// CHECK WFSMODES //
	////////////////////
	asprintf(&outfile, "%s-wfsmodes", shtrack->influence);
	
	// read vector from file (should be made by calibSVDGSL)
	fd = fopen(outfile, "r");
	if (!fd) {
		logWarn("Could not open file %s: %s", outfile, strerror(errno));
		return EXIT_FAILURE;
	}
	if (shtrack->wfsmodes == NULL) {
		shtrack->wfsmodes = gsl_matrix_float_calloc(nsubap*2, nacttot);
		if (shtrack->wfsmodes == NULL) logErr("Failed to allocate memory for wfsmodes matrix.");
	}

	logDebug(0, "Reading WFS modes into memory from %s now...", outfile);
	gsl_matrix_float_fscanf(fd, shtrack->wfsmodes);
	fclose(fd);
	
	// CHECK DMMODES //
	///////////////////
	asprintf(&outfile, "%s-dmmodes", shtrack->influence);
	
	// read vector from file (from modSVDGSL)
	fd = fopen(outfile, "r");
	if (!fd) {
		logWarn("Could not open file %s: %s", outfile, strerror(errno));
		return EXIT_FAILURE;
	}
	if (shtrack->dmmodes == NULL) {
		shtrack->dmmodes = gsl_matrix_float_calloc(nacttot, nacttot);
		if (shtrack->dmmodes == NULL) logErr("Failed to allocate memory for dmmodes matrix.");
	}

	logDebug(0, "Reading DM modes into memory from %s now...", outfile);
	gsl_matrix_float_fscanf(fd, shtrack->dmmodes);
	fclose(fd);

	logInfo(0, "Successfully read influence function calibration & decomposition into memory.");
 	return EXIT_SUCCESS;
}

int calibSVDGSL(control_t *ptc, int wfs, mod_sh_track_t *shtrack) {
	FILE *fd;
	int i, j, nact=0, nsubap = shtrack->nsubap;
	double tmp;
	if (nsubap == 0) {
		logWarn("Cannot do SVD if no subapertures are selected");
		return EXIT_FAILURE;
	}
	for (i=0; i< ptc->wfc_count; i++)
		nact += ptc->wfc[i].nact;
	
	logInfo(0, "Doing SVD of influence function for %d subaps and %d actuators", nsubap, nact);
	// temporary matrices to store stuff in
	gsl_matrix *mat, *v;
	gsl_vector *sing, *work, *testin, *testout;
	gsl_matrix_float *matf;
	gsl_vector_float *testinf, *testoutf, *testinrecf, *workf;
	gsl_vector *testinrec, *testoutrec;
	char *outfile;
	
	// allocating space for SVD:
	mat = gsl_matrix_calloc(nsubap*2, nact);
	matf = gsl_matrix_float_calloc(nsubap*2, nact);	
	v = gsl_matrix_calloc(nact, nact);
	sing = gsl_vector_calloc(nact);
	work = gsl_vector_calloc(nact);
	testin = gsl_vector_calloc(nact); // test input vector
	testout = gsl_vector_calloc(nsubap*2); // test output vector (calculate using normal matrix)

	workf = gsl_vector_float_calloc(nact);
	testinf = gsl_vector_float_calloc(nact); // test input vector
	testinrecf = gsl_vector_float_calloc(nact); // reconstrcuted input vector (use pseudo inverse from SVD)
	testoutf = gsl_vector_float_calloc(nsubap*2); // test output vector (calculate using normal matrix)

	testinrec = gsl_vector_calloc(nact); // reconstrcuted input vector (use pseudo inverse from SVD)
	testoutrec = gsl_vector_calloc(nsubap*2); // test output vector (calculate using SVD)
	
	// generate test input:
	for (j=0; j<nact; j++) {
		tmp = drand48()*2-1;
		gsl_vector_set(testin, j, tmp);
		gsl_vector_float_set(testinf, j, tmp);
	}
	
	// read influence matrix from file (from calibWFC)
	fd = fopen(shtrack->influence, "r");
	if (!fd) {
		logWarn("Could not open file %s: %s", shtrack->influence, strerror(errno));
		return EXIT_FAILURE;
	}
	gsl_matrix_fscanf(fd, mat);
	
	fclose(fd);
	fd = fopen(shtrack->influence, "r");
	if (!fd) {
		logWarn("Could not open file %s: %s", shtrack->influence, strerror(errno));
		return EXIT_FAILURE;
	}
	gsl_matrix_float_fscanf(fd, matf);
	fclose(fd);
	
	// calculate test output vector
	gsl_blas_dgemv(CblasNoTrans, 1.0, mat, testin, 0.0, testout);
	gsl_blas_sgemv(CblasNoTrans, 1.0, matf, testinf, 0.0, testoutf);
	
	logInfo(0, "Performing SVD on matrix from %s. nsubap: %d, nact: %d.", shtrack->influence, nsubap, nact);
	
	// perform SVD
	gsl_linalg_SV_decomp(mat, v, sing, work);
	
	// save stuff to file and re-read into float vector and matrices 
	
	// write matrix U to file (wfs modes):
	asprintf(&outfile, "%s-wfsmodes", shtrack->influence);
	fd = fopen(outfile, "w+");
	if (!fd) logErr("Error opening output file %s: %s", outfile, strerror(errno));
	gsl_matrix_fprintf (fd, mat, "%.15lf");
	fclose(fd);
	
	// write matrix V to file (wfs dmmodes):
	asprintf(&outfile, "%s-dmmodes", shtrack->influence);
	fd = fopen(outfile, "w+");
	if (!fd) logErr("Error opening output file %s: %s", outfile, strerror(errno));
	gsl_matrix_fprintf (fd, v, "%.15lf");
	fclose(fd);
	
	// write out singular values:
	asprintf(&outfile, "%s-singular", shtrack->influence);
	fd = fopen(outfile, "w+");
	if (!fd) logErr("Error opening output file %s: %s", outfile, strerror(errno));
	gsl_vector_fprintf (fd, sing, "%.15lf");
	fclose(fd);

	logDebug(0, "Re-reading stored matrices and vector into memory");
	if (calibWFCChk(ptc, wfs, shtrack) != EXIT_SUCCESS) {
		logWarn("Re-reading stored SVD files failed.");
		return EXIT_FAILURE;
	}
	
	logInfo(0, "SVD complete, sanity checking begins");
	// check if the SVD worked:
	// V^T . testin
	// gsl_blas_sgemv(CblasTrans, 1.0, ptc->wfs[wfs].dmmodes, testin, 0.0, work);
	// 
	// // singular . (V^T . testin)
	// for (j=0; j<nact; j++)
	// 	gsl_vector_set(work, j, gsl_vector_get(work, j) *gsl_vector_get(ptc->wfs[wfs].singular, j));
	// 	
	// // U . (singular . (V^T . testin))
	// gsl_blas_dgemv(CblasNoTrans, 1.0, ptc->wfs[wfs].wfsmodes, work, 0.0, testoutrec);
	
//    gsl_vector *workf = gsl_vector_calloc (N);

    gsl_blas_sgemv (CblasTrans, 1.0, shtrack->wfsmodes, testoutf, 0.0, workf);

	int singvals=nact;
	for (i = 0; i < nact; i++) {
		float wi = gsl_vector_float_get (workf, i);
		float alpha = gsl_vector_float_get (shtrack->singular, i);
		if (alpha != 0) {
			alpha = 1.0 / alpha;
			singvals--;
		}
		gsl_vector_float_set (workf, i, alpha * wi);
	}

    gsl_blas_sgemv (CblasNoTrans, 1.0, shtrack->dmmodes, workf, 0.0, testinrecf);
	// testinrecf should hold the float version of the reconstructed vector 'testinf'
	
	// calculate inverse
	gsl_linalg_SV_solve(mat, v, sing, testout, testinrec);
	
	// testinrec should hold the double version of the recon vector 'testin'
	
	// calculate difference
	double diffin=0, diffout=0;
	float cond, min, max;
	
	logDebug(0, "Reconstruction test double: (values per line should be equal)");
	for (j=0; j<nact; j++) {
		diffin += gsl_vector_get(testinrec, j)/gsl_vector_get(testin, j);
		logDebug(LOG_NOFORMAT, "%f, %f\n", gsl_vector_get(testinrec, j), gsl_vector_get(testin, j));
	}
	diffin /= nact;
	
	logDebug(0, "Reconstruction test float: (values per line should be equal)");
	for (j=0; j<nact; j++) {
		diffout += gsl_vector_float_get(testinrecf, j)/gsl_vector_float_get(testinf, j);
		logDebug(LOG_NOFORMAT, "%f, %f\n", gsl_vector_float_get(testinrecf, j), gsl_vector_float_get(testinf, j));
	}
	diffout /= nact;
	// get max and min to calculate condition
	gsl_vector_float_minmax(shtrack->singular, &min, &max);
	cond = max/min;

	logInfo(0, "SVD Succeeded, decomposition (U, V and Sing) stored to files.");
	logInfo(0, "SVD # of zero singvals (0 is good): %d. Condition (close to 1 would be nice): %lf.", singvals, cond);
	logInfo(0, "SVD quality: in (double), in (float) ratio (must be 1): %lf and %lf", diffin, diffout);

	return EXIT_SUCCESS;
}
