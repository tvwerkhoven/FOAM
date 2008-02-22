/*! 
	@file foam_modules-dm.c
	@author @authortim
	@date December 7, 2007

	@brief This file contains routines to simulate an n-actuator DM.

	\section Info
	To run, see help at modSimDM(). This module can simulate the shape of an N-actuator piezo-electric driven membrane mirror.
	
	\section Functions
	
	\li modSimDM
	
	\section History
	This file is based on response2.c by C. Keller (ckeller@noao.edu)
	which was last update on December 19, 2002. This code in turn was based
	on response.c by Gleb Vdovin (gleb@okotech.com) with the following copyright notice:

\verbatim
(C) Gleb Vdovin 1997
Send bug reports to gleb@okotech.com

Modified and cleaned in 1998
by Prof. Oskar von der Luehe
ovdluhe@kis.uni-freiburg.de
\endverbatim 

	\section License
	Gleb Vdovin agreed to release his code into the public domain under the GPL on January 26, 2008.
*/

// include files

#include "foam_modules-dm.h"

// FUNCTIONS BEGIN //
/*******************/

int modSimTT(float *ctrl, float *image, int res[2]) {
	// ctrl is between -1 and 1, and should be linear, and is a 2 element array
	// image is the output we want to add to
	// simulating the TT is very simple, it just tilts if ctrl != [0,0]
	
	// tilting is done as:
	// from -amp to +amp over res[0] or res[1], with 0 in the middle and multiplied with ctrl[0]
	// i.e. over i (y direction, res[1], ctrl[1]):
	// ((i/res[1])-0.5) * 2 * amp * ctrl[1]
	// and for j (over x)
	// ((j/res[0])-0.5) * 2 * amp * ctrl[0]
	int i,j;
	float amp = 4;
	
	// first simulate rails (i.e. crop ctrl above abs(1))
	// TODO: can be done faster?
	if (ctrl[0] > 1.0) ctrl[0] = 1.0;
	if (ctrl[0] < -1.0) ctrl[0] = -1.0;
	if (ctrl[1] > 1.0) ctrl[1] = 1.0;
	if (ctrl[1] < -1.0) ctrl[1] = -1.0;
	
	for (i=0; i<res[1]; i++)
		for (j=0; j<res[0]; j++)
			image[i*res[0] + j] += ((((float) i/(res[1]-1))-0.5) * 2 * amp * ctrl[1]) + \
								((((float) j/(res[0]-1) )-0.5) * 2 * amp * ctrl[0]);

	// this had problems with integer divisions:
	//image[i*res[0] + j] += (((i/res[1])-0.5) * 2 * amp * ctrl[1]) + (((j/res[0])-0.5) * 2 * amp * ctrl[0]);
	
	return EXIT_SUCCESS;	
}

// globals for modSimDM so we don't need to read in the PGM files every time
float *resp=NULL;		// we store the mirror response here
float *boundary=NULL;	// we store the aperture here
float *act=NULL;		// we store the actuator pattern here
float *actvolt=NULL;	// we store the actuator pattern with voltages applied here


int modSimDM(char *boundarymask, char *actuatorpat, int nact, float *ctrl, float *image, int res[2], int niter) {
	int i, j, x, y;
	int ii, i_1j, i1j, ij_1, ij1;
//int nx=0, ny=0;	
	int voltage[nact]; // voltage settings for electrodes (in digital units)

	float pi, rho, omega = 1.0, sum, sdif, update;
	int ik;
	
	SDL_Surface *boundarysurf, *actsurf;

	float amp=0.5;	
	pi = 4.0*atan(1);

	logDebug("Boundarymask: %s, actuator pattern file: %s", boundarymask, actuatorpat);

	// read boundary mask file 
	if (boundary == NULL) {
		if (modReadPGM(boundarymask, &boundarysurf) != EXIT_SUCCESS) {
			logErr("Cannot read boundary mask");
			return EXIT_FAILURE;
		}
		if (boundarysurf->w != res[0] || boundarysurf->h != res[1]) {
			logErr("Boundary mask resolution incorrect! (%dx%d vs %dx%d)", res[0], res[1], boundarysurf->w, boundarysurf->h);
			return EXIT_FAILURE;			
		}
		// copy from SDL_Surface to array
		boundary = calloc( res[0]*res[1], sizeof(*boundary));
		if (boundary == NULL) {
			logErr("Error allocating memory for boundary image");
			return EXIT_FAILURE;
		}
		else {
			for (y=0; y<res[1]; y++)
				for (x=0; x<res[0]; x++)
					boundary[y*res[0] + x] = (float) getpixel(boundarysurf, x, y);
		}
		
		logInfo("Read boundary '%s' succesfully (%dx%d)", boundarymask, res[0], res[1]);
	}
	

	
	// read actuator pattern file 
	if (act == NULL) {
		if (modReadPGM(actuatorpat, &actsurf) != EXIT_SUCCESS) {
			logErr("Cannot read boundary mask");
			return EXIT_FAILURE;
		}

		if (actsurf->w != res[0] || actsurf->h != res[1]) {
			logErr("Actuatorn pattern resolution incorrect! (%dx%d vs %dx%d)", res[0], res[1], actsurf->w, actsurf->h);
			return EXIT_FAILURE;			
		}
		
		// copy from SDL_Surface to array
		act = calloc((res[0]) * (res[1]), sizeof(*act));
		if (act == NULL) {
			logErr("Error allocating memory for actuator image");
			return EXIT_FAILURE;
		}
		else {
			for (y=0; y<res[1]; y++)
				for (x=0; x<res[0]; x++)
					act[y*res[0] + x] = (float) getpixel(actsurf, x, y);
		}
		logInfo("Read actuator pattern '%s' succesfully (%dx%d)", actuatorpat, res[0], res[1]);
	}

	// allocate memory for actuator pattern with voltages
	if (actvolt == NULL) {
		actvolt = calloc((res[0]) * (res[1]), sizeof(*actvolt));
		if (actvolt == NULL) {
			logErr("Error allocating memory for actuator voltage image");
			return EXIT_FAILURE;
		}
	}
	

		
	// Input linear and c=[-1,1], 'output' must be v=[0,255] and linear in v^2
	logDebug("Simulating DM with voltages:");
	for (ik = 0; ik < nact; ik++) {
		// we do Sqrt(255^2 (i+1) * 0.5) here to convert from [-1,1] (linear) to [0,255] (quadratic)
		voltage[ik] = (int) round( sqrt(65025*(ctrl[ik]+1)*0.5 ) ); //65025 = 255^2

		// TODO: this line is fishy, uncommenthing it results in crap 
		 logDirect("%d ", voltage[ik]); // TODO: we don't want printf here
	}
	logDirect("\n");
	

	for (ik = 0; ik < res[0]*res[1]; ik++) {
		if (*(boundary + ik) > 0) {
			*(boundary + ik) = 1.;
		} else {
			*(boundary + ik) = 0.;
		}
	}

// read actuator pattern file
	// if (modReadPGM(actuatorpat,&act,&nx,&ny,&ngray2) != EXIT_SUCCESS) {
	// 	logErr("Cannot read actuator pattern file");
	// 	return EXIT_FAILURE;
	// }


	// set actuator voltages on electrodes
	for (ik = 0; ik < res[0]*res[1]; ik++){
		i = (int) act[ik];
//		logDirect("%f ",act[ik]);
		if(i > 0) {
			// 75.7856*2 gets 3 um deflection when all voltages 
			// are set to 180; however, since the reflected wavefront sees twice
			// the surface deformation, we simply put a factor of two here
			actvolt[ik] = pow(voltage[i-1] / 255.0, 2.0) / 75.7856;
			
		}
	}

	// compute spectral radius rho and SOR omega factor. These are appro-
	// ximate values. Get the number of iterations.

	rho = (cos(pi/((double)res[0])) + cos(pi/((double)res[1])))/2.0;
	omega = 2.0/(1.0 + sqrt(1.0 - rho*rho));
	// fprintf(stderr,"Spectral radius:\t%g\tSOR Omega:\t%g\n",rho,omega);

	if (niter <= 0) // Set the number of iterations if the argument was <= 0
		niter = (int)(2.0*sqrt((double) res[0]*res[1]));

	// fprintf(stderr,"Number of iterations:\t%d\n", niter);

// Calculation of the response. This is a Poisson PDE problem
// where the actuator pattern represents the source term and and
// mirror boundary a Dirichlet boundary condition. Because of the
// arbitrary nature of the latter we use a relaxation algorithm.
// This is a Simultaneous Over-Relaxation (SOR) algorithm described
// in Press et al., "Numerical Recipes", Section 17.
	
	if (resp == NULL) {
		logDebug("Allocating memory for resp: %dx%d.", res[0], res[1]);
		resp = calloc(res[0]*res[1], sizeof(*resp));
		if (resp == NULL) {
			logErr("Allocation error, exiting");
			return EXIT_FAILURE;
		}
	}

	/*  fprintf(stderr,"Iterating:\n"); */

	/* loop over all iterations */
		for (ii = 1; ii <= niter; ii++) {
			sum = 0.0;
			sdif = 0.0;
			for (i = 2; i <= res[0]-1; i += 1) { /* loops over 2-D aperture */
				for (j = 2; j <= res[1]-1; j += 1) {
	/* --->>> might need to be res[0] instead of res[1], also everywhere below */
				ik = (i - 1)*res[1] + j - 1; /* the pixel location in the 1-d array */
				if (*(boundary + ik) > 0) { /* pixel is within membrane boundary */
	/* calculate locations of neighbouring pixels */
					i_1j = (i - 2)*res[1] + j - 1;
					i1j = i*res[1] + j - 1;
					ij_1 = (i - 1)*res[1] + j - 2;
					ij1 = (i - 1)*res[1] + j;

					update = -resp[ik] - (actvolt[ik] - resp[i_1j] - resp[i1j] - resp[ij1] - resp[ij_1])/4.;
					resp[ik] = resp[ik] + omega*update;
					sum += resp[ik];
					sdif += (omega*update)*(omega*update);
				} 
				else
					resp[ik] = 0.0;

				}
			} /* end of loop over aperture */

		sdif = sqrt(sdif/(sum*sum));
		/*    fprintf(stderr,"\t%d\t%g    \r",ii,sdif); */
		if (sdif < SOR_LIM)		/* stop iteration if changes are */
			break;			/* small enough			 */
	}



	// response output
	for (i = 0; i < res[0]*res[1]; i++){
		// TvW: += UPDATES the image, so there should be an image in image, or it should be zero
		image[i] += amp*resp[i];
		// printf("%e\n",resp[ik]);
	//printf("\n");
	}


	//stat = write_pgm("response2.pgm",resp,nx,ny,255);

	// free(act);
	// free(boundary);
	// free(resp);

	return EXIT_SUCCESS; // successful completion
}


