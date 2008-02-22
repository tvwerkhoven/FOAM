/*! 
	@file foam_modules-calib.c
	@author @authortim
	@date 2008-02-07

	@brief This file is used to calibrate actuators like the DM and TT mirrors used in AO systems.
	Works on any combination of WFC and WFS, and uses generic controls in the [-1,1] range
	
	This only works on SH WFs\n
	
	TODO: document further
	
*/

#include "foam_modules-calib.h"

#define DM_MAXVOLT 1	// these are normalized and converted to the right values by drvSetActuator()
#define DM_MIDVOLT 0
#define DM_MINVOLT -1
#define FOAM_MODCALIB_DMIF "../config/ao_dmif"

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
int modCalWFC(control_t *ptc, int wfs) {
	int j, i, k, skip;
	int nact=0;			// total nr of acts for all WFCs
	int nsubap=0;		// nr of subapts for specific WFS
	int wfc; 			// counters to loop over the WFCs
	float origvolt;
	int measurecount=1;
	int skipframes=1;
	FILE *fp;
	int fildes;
	struct stat buf;
	
	logDebug("pinhole %s",ptc->wfs[wfs].pinhole);
	
	// check if the pinhole calibration worked ok file is OK
	if ((fildes = open(ptc->wfs[wfs].pinhole, O_RDONLY)) == -1) {
		logErr("Pinhole calibration file %s not found: %s", ptc->wfs[wfs].pinhole, strerror(errno));
		return EXIT_FAILURE;
	}
	else {
		if (fstat(fildes, &buf) == 0) {
			if (buf.st_size < 5) {
				logErr("Pinhole calibration file %s is corrupt (filesize %d).", ptc->wfs[wfs].pinhole, buf.st_size);
				return EXIT_FAILURE;
			}
		}
		else {
			logErr("Cannot stat pinhole calibration file %s: %s", ptc->wfs[wfs].pinhole, strerror(errno));
			return EXIT_FAILURE;
		}
	}
	
	logDebug("Starting WFC calibration");
	
	// set filterwheel to pinhole
	drvFilterWheel(ptc, FILT_PINHOLE);
	
	// set control vector to zero (+-180 volt for an okotech DM)
	for (i=0; i < ptc->wfc_count; i++) {
		for (j=0; j < ptc->wfc[i].nact; j++) {
			ptc->wfc[i].ctrl[j] = 0;
		}
	}
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
	nsubap = ptc->wfs[wfs].nsubap;

	// this stores some vectors :P
	float q0x[nsubap], q0y[nsubap];
				
	logInfo("Calibrating WFC's using %d actuators and wfs %d with %d subapts, storing in %s.", \
		nact, wfs, nsubap, ptc->wfs[wfs].influence);
	fp = fopen(ptc->wfs[wfs].influence,"w+");
	fprintf(fp,"%d\n%d\n", nact, nsubap*2); // we have nact actuators (variables) and nsubap*2 measurements
	
	for (wfc=0; wfc < ptc->wfc_count; wfc++) { // loop over all wave front correctors 
		nact = ptc->wfc[wfc].nact;
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
	} // end loop over wfcs
	fclose(fp);
	logInfo("WFS %d (%s) influence function saved for in file %s", wfs, ptc->wfs[wfs].name, ptc->wfs[wfs].influence);
	
	modSVD(ptc, wfs);
	
	return EXIT_SUCCESS;
}


/* ---------------------------------------------------------------------------
 * file: svdproc3.c
 *
 * Calculates the pseudo-inverse of the influence matrix
 * through singular value decomposition.
 * This version is for all observation modes and is used as a procedure,
 * This is not a stand-alone program
 *
 * based on code created by Mark Ammons
 * last change: 21 January 2005 by ckeller@noao.edu
 * ---------------------------------------------------------------------------
 */

// Singular Value Decomposition Algorithm Copyright (C) 2000, James Arvo  
//
// Given a matrix Q, this algorithm will find its singular value
// decomposition Q = A * D * (R^T).  The algorithm replaces Q with A (they
// have the same dimensions)  D is a diagonal matrix with the singular
// values positioned, in decreasing order, along the main diagonal.
// R is output in its transposed form (as R^T).

// The program reads the input matrix Q from "influence.dat."  The first
// two entries of "influence.dat" are ACTUATORS and 2 * TOTALSPOTS, or the
// number of columns and rows of the influence matrix, respectively.

// The program generates the pseudo-inverse of the influence matrix based
// the SVD equation Q^-1 = R * D' * (A^T).  D' is a diagonal matrix with 
// the reciprocal singular values on the main diagonal.  For an overdetermined
// system (in which there are more spot offset measurements than actuators), 
// the pseudo-inverse will produce the least-squares solution to a linear 
// set of equations.  For an underdetermined system (in which there are
// more actuators than spot offset measurements), no unique solution to the
// set of linear equations is expected, but the pseudo-inverse will 
// produce the result closest to the zero vector (which may not be the
// desired result).

// The program outputs the inverse of the influence matrix to "inverse.dat."
// The first two entries of the file are ACTUATORS and 2 * TOTALSPOTS, or the
// number of rows and columns of the inverse matrix, respectively.


//#include "ctrla3.h"

// This constant defines how many iterations that the singular
// value decomposition algorithm in "svdcmp" will use.
#define MAXITERATIONS 50

int modSVD(control_t *ptc, int wfs) {
  // ptc->wfs[wfs].scandir    defines whether x,y,or both axes are to be used
  //         possible values for axis are defined in ctrla2.h

  int i, j, k, l=0, p, q, r, t, iter; // counters for decomposition
  int nact=0, nsubap=0;
  int n, m;

  FILE *in; 			    // input Q from "influence.dat" (ptc->wfs[wfs].pinhole)
//  FILE *out;	 // output inverse matrix to "inverse.dat" (ptc->wfs[wfs].pinhole . inverse)
  FILE *out2;    // output singular values to "singular.dat" (ptc->wfs[wfs].pinhole . singular)

  // get nsubap and nact in two different ways (from file and from *ptc), these should match
  in = fopen(ptc->wfs[wfs].influence, "r");
  if (in == NULL) {
    logErr("error opening input file %s", ptc->wfs[wfs].influence);
    return EXIT_FAILURE;
  }
  fscanf(in,"%d\n",&n);
  fscanf(in,"%d\n",&m);

	for (i=0; i < ptc->wfc_count; i++) {
		nact += ptc->wfc[i].nact;
	}
  nsubap = ptc->wfs[wfs].nsubap;

  int bindex;			    // row/col of the largest singular value
  int jump=0;			    // variable used for decomposition
//  int modes;                        // number of singular values to keep

  double c, f, h, s, x, y, z;	    // variable used for decomposition
  double norm  = 0.0;		    // variable used for decomposition
  double g     = 0.0;		    // variable used for decomposition
  double scale = 0.0;		    // variable used for decomposition
  double biggest;		    // largest singular value
  double value;			    // used to swap singular value entries
  
  double Q[2 * nsubap][nact];         // influence matrix
  double R[nact][nact];   // result matrix of decomposition
  double D[nact][nact];   // matrix containing singular values

  double solution1[nact][nact];    // result of (R * D')
  double solution2[nact][2 * nsubap]; // pseudo-inverse
  
  double Temp[nact];	    // vector used for decomposition
  double diag[nact];	    // vector storing singular values
	
  
//  int flag;		   // 0 if Q cannot be input from "influence.dat"
//  char string[256];	   //stores error message

    
  //Initialize variables and matrices

  for (r = 0; r < m; ++r) {
    for (t = 0; t < n; ++t) {
      Q[r][t] = 0.0;
      solution2[t][r] = 0.0;
    }
  }

  for (r = 0; r < n; ++r) {
    for (t = 0; t < n; ++t) {
      solution1[r][t] = 0.0;
      D[r][t] = 0.0;
      R[r][t] = 0.0;
    }
  }

  //--------------------------------------------------------------------------
  //   get the influence matrix
  //--------------------------------------------------------------------------

logInfo("Starting singular value decomposition for WFS %d (%s)", wfs, ptc->wfs[wfs].influence);
  // printf("%d nact;   %d subapertures*2\n",n,m);
  if (n == nact && m == 2*nsubap) {
    for (t = 0; t < n; t++) {
      for (r = 0; r < m; r++) {
    	fscanf(in,"%lg\n",&Q[r][t]);
	  }
	}
    fclose(in);
  } else {
    logErr("input not in correct format. Program ended.");
    fclose(in);
	return EXIT_FAILURE;
  }

  // set values in one or the other axis to zero for 1-D operation
  if (ptc->wfs[wfs].scandir == AO_AXES_X) {
    for (t = 0; t < n; t++) 
      for (r = 1; r < m; r = r + 2)
	Q[r][t] = 0.0;
  }
  if (ptc->wfs[wfs].scandir == AO_AXES_Y) {
    for (t = 0; t < n; t++) 
      for (r = 0; r < m; r = r + 2)
	Q[r][t] = 0.0;
  }

    
  //-----------------------------------------------------------------------
  //  singular value decomposition
  //-----------------------------------------------------------------------
      
  for (i = 0; i < n; i++) {
		
    Temp[i] = scale * g;
    scale   = 0.0;
    g       = 0.0;
    s       = 0.0;
    l       = i + 1;
    
    if (i < m) {
      for (k = i; k < m; k++) 
	scale += fabs(Q[k][i]);
      if (scale != 0.0) {
	for(k = i; k < m; k++) {
	  Q[k][i] /= scale;
	  s += pow(Q[k][i], 2);
	}
	f = Q[i][i];
	if (f >= 0.0) 
	  g = -1.0 * sqrt(s);
	else
	  g = sqrt(s);
	h = (f * g) - s;
	Q[i][i] = f - g;
	if (i != (n - 1)) {
	  for (j = l; j < n; j++) {
	    s = 0.0;
	    for(k = i; k < m; k++)
	      s += Q[k][i] * Q[k][j];
	    f = s / h;	
	    for(k = i; k < m; k++) 
	      Q[k][j] += f * Q[k][i];
	  }	
	}
	for (k = i; k < m; k++)
	  Q[k][i] *= scale;
      }
    }
    
    diag[i] = scale * g;
    g       = 0.0;
    s       = 0.0;
    scale   = 0.0;
    
    if (i < m && i != n - 1) {
      for (k = l; k < n; k++)
	scale += fabs(Q[i][k]);
      if (scale != 0.0) {
	for( k = l; k < n; k++) {
	  Q[i][k] /= scale;
	  s += pow(Q[i][k], 2);
	}
	f = Q[i][l];
	if (f >= 0.0) 
	  g = -1.0 * sqrt(s);
	else
	  g = sqrt(s);
	h = (f * g) - s;
	Q[i][l] = f - g;
	for (k = l; k < n; k++)
	  Temp[k] = Q[i][k] / h;
	if (i != m - 1) {
	  for (j = l; j < m; j++) {
	    s = 0.0;
	    for (k = l; k < n; k++) 
	      s += Q[j][k] * Q[i][k];
	    for (k = l; k < n; k++)
	      Q[j][k] += s * Temp[k];
	  }
	}
	for (k = l; k < n; k++)
	  Q[i][k] *= scale;
      }
    }
    if (norm < (fabs(diag[i]) + fabs(Temp[i]))) 
      norm = fabs(diag[i]) + fabs(Temp[i]);
  }
  
  for (i = n - 1; i >= 0; i--) {
    if (i < n - 1) {
      if (g != 0.0) {
	for (j = l; j < n; j++)
	  R[i][j] = (Q[i][j] / Q[i][l]) / g;
	for (j = l; j < n; j++) {
	  s = 0.0;
	  for (k = l; k < n; k++)
	    s += Q[i][k] * R[j][k];
	  for (k = l; k < n; k++)
	    R[j][k] += s * R[i][k];
	}
      }
      for (j = l; j < n; j++) {
	R[i][j] = 0.0;
	R[j][i] = 0.0;
      }
    }
    R[i][i] = 1.0;
    g = Temp[i];
    l = i;
  }
  
  for (i = n - 1; i >= 0; i--) {
    l = i + 1;
    g = diag[i];
    
    if (i < n - 1)
      for(j = l; j < n; j++)
	Q[i][j] = 0.0;
    
    if (g != 0.0) {
      g = 1.0 / g;
      if (i != n - 1) {
	for (j = l; j < n; j++) {
	  s = 0.0;
	  for (k = l; k < m; k++)
	    s += Q[k][i] * Q[k][j];
	  f = (s / Q[i][i]) * g;
	  for (k = i; k < m; k++)
	    Q[k][j] += f * Q[k][i];
	}
      }
      for (j = i; j < m; j++)
	Q[j][i] *= g;
    } 
    else {
      for (j = i; j < m; j++)
	Q[j][i] = 0.0;
    }
    Q[i][i] += 1.0;
  }
  
  for (k = n - 1; k >= 0; k--) {
    for (iter = 1; iter <= MAXITERATIONS; iter++) {
      
      for (l = k; l >= 0; l--) {
	q = l - 1;
	if (fabs(Temp[l]) + norm == norm) { 
	  jump = 1;
	  break;
	}
	if (fabs(diag[q]) + norm == norm) {
	  jump = 0;
	  break;
	}
      }
      
      if(!jump) {
	c = 0.0;
	s = 1.0;
	for (i = l; i <= k; i++) {
	  f = s * Temp[i];
	  Temp[i] *= c;
	  if (fabs(f) + norm == norm) 
	    break;
	  g = diag[i];
	  
	  if (fabs(f) > fabs(g))
	    h = fabs(f) * sqrt(1.0 + pow((fabs(g) / fabs(f)), 2));
	  else if (fabs(g) > 0.0)
	    h = fabs(g) * sqrt(1.0 + pow((fabs(f) / fabs(g)), 2));
	  else 
	    h = 0.0;
	  
	  diag[i] = h;
	  h = 1.0 / h;
	  c = g * h;
	  s = (-1.0 * f) * h;
	  for (j = 0; j < m; j++) {
	    y = Q[j][q];
	    z = Q[j][i];
	    Q[j][q] = (y * c) + (z * s);
	    Q[j][i] = (z * c) - (y * s);
	  }
	}
      }
      
      z = diag[k];
      if (l == k) {
	if (z < 0.0) {
	  diag[k] = (-1.0 * z);
	  for (j = 0; j < n; j++)
	    R[k][j] *= -1.0; 
	}
	break;
      }
      if (iter >= MAXITERATIONS)
	return 0;
      x = diag[l];
      q = k - 1;
      y = diag[q];
      g = Temp[q];
      h = Temp[k];
      f = (((y - z) * (y + z) + (g - h) * (g + h)) / (2.0 * h * y));
      
      if (fabs(f) > 1.0)
	g = fabs(f) * sqrt(1.0 + pow((1.0 / f), 2));
      else
	g = sqrt(1.0 + pow(f, 2));
      
      if (f >= 0.0) 
	f = ((x - z) * (x + z) + h * ((y / (f + fabs(g))) - h)) / x;
      else
	f = ((x - z) * (x + z) + h * ((y / (f - fabs(g))) - h)) / x;
      c = 1.0;
      s = 1.0;
      for(j = l; j <= q; j++) {
	i = j + 1;
	g = Temp[i];
	y = diag[i];
	h = s * g;
	g = c * g;
	
	if (fabs(f) > fabs(h))
	  z = fabs(f) * sqrt(1.0 + pow((fabs(h) / fabs(f)), 2));
	else if (fabs(h) > 0.0)
	  z = fabs(h) * sqrt(1.0 + pow((fabs(f) / fabs(h)), 2));
	else 
	  z = 0.0;
	Temp[j] = z;
	c = f / z;
	s = h / z;
	f = (x * c) + (g * s);
	g = (g * c) - (x * s);
	h = y * s;
	y = y * c;
	for (p = 0; p < n; p++) {
	  x = R[j][p];
	  z = R[i][p];
	  R[j][p] = (x * c) + (z * s);
	  R[i][p] = (z * c) - (x * s);
	}
	if (fabs(f) > fabs(h))
	  z = fabs(f) * sqrt(1.0 + pow((fabs(h) / fabs(f)), 2));
	else if (fabs(h) > 0.0)
	  z = fabs(h) * sqrt(1.0 + pow((fabs(f) / fabs(h)), 2));
	else 
	  z = 0.0;
	diag[j] = z;
	
	if (z != 0.0) {
	  z = 1.0 / z;
	  c = f * z;
	  s = h * z;
	}
	f = (c * g) + (s * y);
	x = (c * y) - (s * g);
	for (p = 0; p < m; p++) {
	  y = Q[p][j];
	  z = Q[p][i];
	  Q[p][j] = (y * c) + (z * s);
	  Q[p][i] = (z * c) - (y * s);
	}
      }
      Temp[l] = 0.0;
      Temp[k] = f;
      diag[k] = x;
    }
  }
  
  // Sort the singular values into descending order.
      
  for (i = 0; i < n - 1; i++) {
	
    biggest = diag[i];  		// Biggest singular value so far.
    bindex  = i;        		// The row/col it occurred in.
    for (j = i + 1; j < n; j++) {
      if (diag[j] > biggest) {
	biggest = diag[j];
	bindex = j;
      }            
    }
    if (bindex != i) { 	// Need to swap rows and columns.
      
      // Swap columns in Q.
      for (p = 0; p < m; ++p) {
	value = Q[p][i];
	Q[p][i] = Q[p][bindex];
	Q[p][bindex] = value;
      }
      
      // Swap rows in R.
      for (p = 0; p < n; ++p) {
	value = R[i][p];
	R[i][p] = R[bindex][p];
	R[bindex][p] = value;
      }
      
      // Swap elements in diag
      value = diag[i];
      diag[i] = diag[bindex];
      diag[bindex] = value;
    }
  }
  
  // write decomposed inverse matrix into files
char *outfile;
asprintf(&outfile, "%s-singular", ptc->wfs[wfs].influence);

  out2 = fopen(outfile, "w+");
  if (out2==NULL) {
    logErr("Error opening output file %s", outfile);
    return EXIT_FAILURE;
  }

  for (i = 0; i < n; ++i) {
    fprintf(out2,"%f\n",diag[i]);
    //    printf("%d  %f\n",i+1,diag[i]);
  }
  fclose(out2);

  // write mirror modes into file
asprintf(&outfile, "%s-dmmodes", ptc->wfs[wfs].influence);

out2 = fopen(outfile, "w+");
if (out2==NULL) {
  logErr("Error opening output file %s", outfile);
  return EXIT_FAILURE;
}

  for (i = 0; i < n; ++i) {
    for (j=0;j<n;j++) {
      fprintf(out2,"%f\n",R[i][j]);
    }
  }
  fclose(out2);


  // write wavefront-sensor modes into file
asprintf(&outfile, "%s-wfsmodes", ptc->wfs[wfs].influence);

out2 = fopen(outfile, "w+");
if (out2==NULL) {
  logErr("Error opening output file %s", outfile);
  return EXIT_FAILURE;
}
  fprintf(out2,"%d\n%d\n",nact, 2 * nsubap);
  for (r = 0; r < n; ++r) 
    for (t = 0; t < m; ++t)
      fprintf(out2,"%.12g\n",Q[t][r]);

  fclose(out2);

  logInfo("SVD complete, saved data to %s-singular, %s-dmmodes and %s-wfsmodes", \
	ptc->wfs[wfs].influence, ptc->wfs[wfs].influence, ptc->wfs[wfs].influence);

  return EXIT_SUCCESS;
  
}