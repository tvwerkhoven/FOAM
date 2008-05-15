/*! 
	@file foam_modules-calib.c
	@author @authortim
	@date 2008-02-07

	@brief This file is used to calibrate actuators like the DM and TT mirrors used in AO systems.

	\section Info
	
	The calibration works on any combination of WFC and WFS, and uses generic controls in the [-1,1] range. 
	These controls must be translated to actual voltages by other routines in other modules. These must also
	take care that the range [-1, 1] is linear. Currently calibration only works on SH WFS.
	
	\section Functions	
	
	The functions provided to the outside world are:
 
	\li modCalPinhole() - Calibrate pinhole coordinates, used as a reference when correcting
	\li modCalPinholeChk() - Checks whether pinhole calibration has been performed
	\li modCalWFC() - Measures the WFC influence function and inverts this. This calibration is done per WFS for all WFC's.
	\li modCalWFCChk() - Checks whether WFC calibration has been performed.

	\section Dependencies
	
	This module depends on the shack-hartmann module (modules-sh), as most calibration is SH-specific.

	\section License
	
	This code is licensed under the GPL, version 2.
			
*/

#include "foam_modules-calib.h"

// obsoleted by ptc->wfs[x].calrange[0] and calrange[1] for the calibration range
//#define DM_MAXVOLT 1	// these are normalized and should be converted to the right values by drvSetActuator()
//#define DM_MIDVOLT 0
//#define DM_MINVOLT -1

// TODO: document
int modCalPinhole(control_t *ptc, int wfs, mod_sh_track_t *shtrack) {
	int i, j;
	FILE *fd;

	// set filterwheel to pinhole and set voltages to 180V
	drvSetupHardware(ptc, ptc->mode, ptc->calmode);
	
	// set control vector to zero (+-180 volt for an okotech DM)
	for (i=0; i < ptc->wfc_count; i++)
		for (j=0; j < ptc->wfc[i].nact; j++)
			gsl_vector_float_set(ptc->wfc[i].ctrl, j , 0.0);
		
	// TvW, TODO, May 13, 2008, this may not be a very portable solution
	// because we don't know what openInit and openLoop do. All we want
	// is that the hardware is setup right and that we have a fresh image
	// in our buffer. Solution: use drvSetupHardware and drvGetImg as 
	// general routines that are further specified by the prime mod?
	
	// run open loop initialisation once
	modOpenInit(ptc);
	
	// run open loop once
	modOpenLoop(ptc);
	
	// collect displacement vectors and store as reference
	logInfo(0, "Found following reference coordinates:");
	for (j=0; j < shtrack->nsubap; j++) {
		gsl_vector_float_set(shtrack->refc, 2*j+0, gsl_vector_float_get(shtrack->disp, 2*j+0)); // x
		gsl_vector_float_set(shtrack->refc, 2*j+1, gsl_vector_float_get(shtrack->disp, 2*j+1)); // y
		// gsl_vector_float_set(ptc->wfs[wfs].refc, 2*j+0, 7); // x
		// gsl_vector_float_set(ptc->wfs[wfs].refc, 2*j+1, 7); // y
				
		logInfo(LOG_NOFORMAT, "(%f,%f) ", gsl_vector_float_get(shtrack->refc, 2*j+0), gsl_vector_float_get(shtrack->refc, 2*j+1));
	}
	// set the rest to zero
	// for (j=ptc->wfs[wfs].nsubap; j < ptc->wfs[wfs].cells[0] * ptc->wfs[wfs].cells[1]; j++)
	// 	fprintf(fp,"%f %f\n", (double) 0, (double) 0);
	
	logInfo(LOG_NOFORMAT, "\n");
	
	// storing to file:
	logInfo(0, "Storing reference coordinates to %s.", shtrack->pinhole);
	fd = fopen(shtrack->pinhole, "w+");
	if (!fd) logErr("Could not open file %s: %s", shtrack->pinhole, strerror(errno));
	gsl_vector_float_fprintf(fd, shtrack->refc, "%.10f"); // TvW TODO: 10 digits enough? too much?
	fclose(fd);
	
	return EXIT_SUCCESS;
}

int modCalPinholeChk(control_t *ptc, int wfs, mod_sh_track_t *shtrack) {
	int nsubap = shtrack->nsubap;
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


// TODO: incomplete
/*
int modLinTest(control_t *ptc, int wfs) {
	int i, j, k, wfc;			// some counters
	int nsubap, nact;
	int skip, skipframes=3;		// skip this amount of frames before measuring
	int niter=50;				// number of measurements for linearity test
	char *outfile;
	float origvolt;
	FILE *fp;


	// get total nr of subapertures in the complete system
	nsubap = ptc->wfs[wfs].nsubap;

	// this stores some vectors :P
	float q0x[nsubap], q0y[nsubap];
	
	// set filterwheel to pinhole
	drvSetupHardware(ptc, ptc->mode, ptc->calmode);
	
	// run openInit once to read the sensors and get subapertures
	modOpenInit(ptc);

	// get total nr of actuators, set all actuators to zero (180 volt for an okotech DM)
	for (i=0; i < ptc->wfc_count; i++) {
		for (j=0; j < ptc->wfc[i].nact; j++) {
			ptc->wfc[i].ctrl[j] = 0;
		}
//		nacttot += ptc->wfc[i].nact;
	}

	for (wfc=0; wfc < ptc->wfc_count; wfc++) { // loop over all wave front correctors 
		nact = ptc->wfc[wfc].nact;
		// write linearity test to file
		asprintf(&outfile, "%s_wfs%d_wfc%d", FOAM_MODCALIB_LIN, wfs, wfc);
		fp = fopen(outfile,"w+");
		if (!fp) {
			logWarn("Cannot open file %s for writing linearity test.", outfile);
			return EXIT_FAILURE;
		}
		fprintf(fp,"3\n");
		fprintf(fp,"%d\n%d\n%d\n", nact, niter, nsubap); // we have nact actuators (variables) and nsubap*2 measurements
		
		for (j=0; j<nact; j++) { // loop over all actuators and all subapts for (wfc,wfs)
	
			for (i=0; i<nsubap; i++) { // set averaging buffer to zero
				q0x[i] = 0.0;
				q0y[i] = 0.0;
			}
	
			origvolt = ptc->wfc[wfc].ctrl[j];
			
			for (k=0; k<niter; k++) {	// split up the range DM_MAXVOLT - DM_MINVOLT in niter pieces
				ptc->wfc[wfc].ctrl[j] = (k+1)/(niter) * (DM_MAXVOLT - DM_MINVOLT) + DM_MINVOLT;

				drvSetActuator(&(ptc->wfc[wfc]));
				
				for (skip=0; skip<skipframes+1; skip++) // skip some frames here
					modOpenLoop(ptc);
				
				// take the shifts and store those (wrt to reference shifts)
	    		for (i=0;i<nsubap;i++) {
					fprintf(fp,"%.12f\n%.12f\n", \
						(ptc->wfs[wfs].disp[i][0] - ptc->wfs[wfs].refc[i][0]),
						(ptc->wfs[wfs].disp[i][1] - ptc->wfs[wfs].refc[i][1]));
				}
			} // end iteration loop 
			
			ptc->wfc[wfc].ctrl[j] = origvolt;
		} // end actuator loop
		
		fclose(fp);
	} // end wfc loop
	
	logInfo(0, "Linearity test using WFS %d (%s) done, stored in file %s", wfs, ptc->wfs[wfs].name, FOAM_MODCALIB_LIN);
	
	return EXIT_SUCCESS;
}
*/

// TODO: document
int modCalWFC(control_t *ptc, int wfs, mod_sh_track_t *shtrack) {
	int j, i, k, skip;
	int nact=0, nacttot=0;			// total nr of acts for all WFCs
	int nsubap=0; 		// nr of subapts for specific WFS
	int wfc; 			// counters to loop over the WFCs
	float origvolt;
	FILE *fd;
	char *file;
	gsl_matrix_float *infl;
		
	// run open loop initialisation once (to get subapertures etc)
	modOpenInit(ptc);
	
	logDebug(0, "Checking pinhole calibration, file %s", shtrack->pinhole);
	
	if (modCalPinholeChk(ptc, wfs, shtrack) != EXIT_SUCCESS) {
		logWarn("WFC calibration failed, first perform pinhole calibration.");
		return EXIT_FAILURE;
	}
	
	logDebug(0, "Starting WFC calibration");
	
	// TvW: set to zero, what does that do?
	for (i=0; i < ptc->wfc_count; i++)
		for (j=0; j < ptc->wfc[i].nact; j++)
			gsl_vector_float_set(ptc->wfc[i].ctrl, j , 0.0);
	
	// delete SVD files:
	asprintf(&file, "%s-singular", shtrack->influence);
	if (unlink(file) != 0) logWarn("Problem removing old SVD files: %s", strerror(errno));
	asprintf(&file, "%s-wfsmodes", shtrack->influence);
	if (unlink(file) != 0) logWarn("Problem removing old SVD files: %s", strerror(errno));
	asprintf(&file, "%s-dmmodes", shtrack->influence);
	if (unlink(file) != 0) logWarn("Problem removing old SVD files: %s", strerror(errno));
		
	// set filterwheel to pinhole
	drvSetupHardware(ptc, ptc->mode, ptc->calmode);
		
	// get total nr of actuators, set all actuators to zero (180 volt for an okotech DM)
	for (i=0; i < ptc->wfc_count; i++) {
		gsl_vector_float_set_zero(ptc->wfc[i].ctrl);
		nacttot += ptc->wfc[i].nact;
	}
	
	// total subapts for WFS
	nsubap = shtrack->nsubap;
	float q0x[nsubap], q0y[nsubap];

	logDebug(0, "Allocating temporary matrix to store influence function (%d, %d x %d)", nsubap, nsubap*2, nact);
	// this will store the influence matrix (which calculates displacements given actuator signals)
	infl = gsl_matrix_float_calloc(nsubap*2, nacttot);
				
	logInfo(0, "Calibrating WFC's using %d actuators and WFS %d with %d subapts, storing in %s.", \
		nacttot, wfs, nsubap, shtrack->influence);
	// fp = fopen(ptc->wfs[wfs].influence,"w+");
	// fprintf(fp,"3\n");								// 3 dimensions (nact, nsub, 2d-vectors)
	// fprintf(fp,"%d\n%d\n%d\n", nact, nsubap, 2); 	// we have nact actuators (variables) and nsubap*2 measurements
	
	for (wfc=0; wfc < ptc->wfc_count; wfc++) { // loop over all wave front correctors 
		nact = ptc->wfc[wfc].nact;
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
//				gsl_vector_float_set(ptc->wfc[wfc].ctrl, j, DM_MAXVOLT);
				
                drvSetActuator(&(ptc->wfc[wfc]));
				// run the open loop *once*, which will approx do the following:
				// read sensor, apply some SH WF sensing, set actuators, 
		
				for (skip=0; skip< shtrack->skipframes + 1; skip++) // skip some frames here
					modOpenLoop(ptc);
		
				// take the shifts and store those (wrt to reference shifts)
	    		for (i=0;i<nsubap;i++) { 	
					q0x[i] += gsl_vector_float_get(shtrack->disp, 2*i+0) / (float) shtrack->measurecount; // x
					q0y[i] += gsl_vector_float_get(shtrack->disp, 2*i+1) / (float) shtrack->measurecount; // y
					// q0x[i] += (ptc->wfs[wfs].disp[i][0] - ptc->wfs[wfs].refc[i][0]) / (float) measurecount;
					// q0y[i] += (ptc->wfs[wfs].disp[i][1] - ptc->wfs[wfs].refc[i][1]) / (float) measurecount;
				}
		
//				ptc->wfc[wfc].ctrl[j] = DM_MINVOLT;
				
//				gsl_vector_float_set(ptc->wfc[wfc].ctrl, j, DM_MINVOLT);
				gsl_vector_float_set(ptc->wfc[wfc].ctrl, j, ptc->wfc[wfc].calrange[0]);				
                drvSetActuator(&(ptc->wfc[wfc]));
						
				for (skip=0; skip< shtrack->skipframes +1; skip++) // skip some frames here
					modOpenLoop(ptc);
		
				// take the shifts and subtract those store those (wrt to reference shifts)
	    		for (i=0;i<nsubap;i++) { 
					q0x[i] -= gsl_vector_float_get(shtrack->disp, 2*i+0) / (float) shtrack->measurecount; // x
					q0y[i] -= gsl_vector_float_get(shtrack->disp, 2*i+1) / (float) shtrack->measurecount; // y
					// q0x[i] -= (ptc->wfs[wfs].disp[i][0] - ptc->wfs[wfs].refc[i][0]) / (float) measurecount;
					// q0y[i] -= (ptc->wfs[wfs].disp[i][1] - ptc->wfs[wfs].refc[i][1]) / (float) measurecount;
				}
			} // end measurecount loop

			// store the measurements for actuator j (for all subapts) 
			
			
			for (i=0; i<nsubap; i++) {				
				gsl_matrix_float_set(infl, 2*i+0, j, (float) q0x[i]/(ptc->wfc[wfc].calrange[1] - ptc->wfc[wfc].calrange[0]));
				gsl_matrix_float_set(infl, 2*i+1, j, (float) q0y[i]/(ptc->wfc[wfc].calrange[1] - ptc->wfc[wfc].calrange[0]));
//				gsl_matrix_float_set(infl, 2*i+0, j, (float) q0x[i]/(DM_MAXVOLT - DM_MINVOLT));
//				gsl_matrix_float_set(infl, 2*i+1, j, (float) q0y[i]/(DM_MAXVOLT - DM_MINVOLT));
//				fprintf(fp,"%.12g\n%.12g\n", (double) q0x[i]/(DM_MAXVOLT - DM_MINVOLT), (double) q0y[i]/(DM_MAXVOLT - DM_MINVOLT));
			}
	
//			ptc->wfc[wfc].ctrl[j] = origvolt;
			gsl_vector_float_set(ptc->wfc[wfc].ctrl, j, origvolt);
		} // end loop over actuators
	} // end loop over wfcs
//	fclose(fp);
	
	fd = fopen(shtrack->influence, "w+");
	if (!fd) logErr("Could not open file %s: %s", shtrack->pinhole, strerror(errno));
	gsl_matrix_float_fprintf(fd, infl, "%.10f");
	fclose(fd);
	
	// save metadata too (nact, n measurements, nsubap);
	asprintf(&file, "%s-meta", shtrack->influence);
	fd = fopen(file, "w+");
	if (!fd) logErr("Could not open file for saving metadata %s: %s", shtrack->pinhole, strerror(errno));
	fprintf(fd, "%d\n%d\n%d\n", nacttot, nsubap, 2*nsubap);
	fclose(fd);

	logInfo(0, "WFS %d (%s) influence function saved for in file %s", wfs, ptc->wfs[wfs].name, shtrack->influence);
	
	modSVDGSL(ptc, wfs, shtrack);
	
	return EXIT_SUCCESS;
}

// this will hold an offloading mechanism of some sort
// !!!:tim:20080416 document
int modOffloadTTDM() {
	
	return EXIT_SUCCESS;
}

int modCalWFCChk(control_t *ptc, int wfs, mod_sh_track_t *shtrack) {
	int i, chknact, chksubap;
	int nsubap = shtrack->nsubap, nacttot=0;
	char *outfile;
	FILE *fd;
	
	// calculate total nr of act for all wfc
	for (i=0; i< ptc->wfc_count; i++)
		nacttot += ptc->wfc[i].nact;
	
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
		logDebug(0, "Calibration parameters do not match current system parameters: nact: %d vs %d, nsubap: %d vs %d", chknact, nacttot, chksubap, nsubap);
		return EXIT_FAILURE;
	}
		
	// CHECK SINGVAL //
	///////////////////
	asprintf(&outfile, "%s-singular", shtrack->influence);
	
	// read vector from file (from modSVDGSL)
	fd = fopen(outfile, "r");
	if (!fd) {
		logWarn("Could not open file %s: %s", outfile, strerror(errno));
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
	
	// read vector from file (from modSVDGSL)
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

 	return EXIT_SUCCESS;
}

int modCalDarkFlat(float *image, float *dark, float *flat, gsl_matrix_float *corr) {
	// TODO: incomplete, no dark/flatfield correction yet.
	// !!!:tim:20080506 only used by sim and simstatic, time to deprecate
	unsigned int i, j;
	for (i=0; i<corr->size1; i++)
		for (j=0; j<corr->size2; j++)
			// corr->data[i * corr->tda + j] = image[i*corr->size1 + j];
			gsl_matrix_float_set(corr, i, j, image[i*corr->size1 + j]);
			
	
	return EXIT_SUCCESS;
}

int modSVDGSL(control_t *ptc, int wfs, mod_sh_track_t *shtrack) {
	FILE *fd;
	int i, j, nact=0, nsubap;
	double tmp;
	gsl_matrix *mat, *v;
	gsl_vector *sing, *work, *testin, *testout;
	gsl_matrix_float *matf;
	gsl_vector_float *testinf, *testoutf, *testinrecf, *workf;
	gsl_vector *testinrec, *testoutrec;
	char *outfile;

	// get total nr of actuators, set all actuators to zero (180 volt for an okotech DM)
	for (i=0; i < ptc->wfc_count; i++) {
		gsl_vector_float_set_zero(ptc->wfc[i].ctrl);
		nact += ptc->wfc[i].nact;
	}
	
	// get total nr of subapertures for this WFS
	nsubap = shtrack->nsubap;
	
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
	
	// read matrix from file (from modCalWFC)
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
	if (modCalWFCChk(ptc, wfs, shtrack) != EXIT_SUCCESS) {
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
	
	for (j=0; j<nact; j++) {
		diffin += gsl_vector_get(testinrec, j)/gsl_vector_get(testin, j);
		// logInfo(LOG_NOFORMAT, "%f, %f\n", gsl_vector_get(testinrec, j), gsl_vector_get(testin, j));
	}
	diffin /= nact;
	
	// logDebug(0, "and other vectors:");
	for (j=0; j<nact; j++) {
		diffout += gsl_vector_float_get(testinrecf, j)/gsl_vector_float_get(testinf, j);
		// logInfo(LOG_NOFORMAT, "%f, %f\n", gsl_vector_float_get(testinrecf, j), gsl_vector_float_get(testinf, j));
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
