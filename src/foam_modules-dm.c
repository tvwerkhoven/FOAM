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

// INCLUDES //
/************/

#include "foam_modules-dm.h"


// GLOBALS //
/***********/

// for modSimDM:
float *resp=NULL;		// we store the mirror response here
float *boundary=NULL;	// we store the aperture here
float *act=NULL;		// we store the actuator pattern here
float *actvolt=NULL;	// we store the actuator pattern with voltages applied here


// FUNCTIONS BEGIN //
/*******************/

int modSimTT(float *ctrl, float *image, coord_t res) {
	int i,j;
	float amp = 2;
		
	// first simulate rails (i.e. crop ctrl above abs(1))
	// if (ctrl[0] > 1.0) ctrl[0] = 1.0;
	// if (ctrl[0] < -1.0) ctrl[0] = -1.0;
	// if (ctrl[1] > 1.0) ctrl[1] = 1.0;
	// if (ctrl[1] < -1.0) ctrl[1] = -1.0;
	
	for (i=0; i<res.y; i++)
		for (j=0; j<res.x; j++)
			image[i*res.x + j] += ((((float) i/(res.y-1))-0.5) * 2 * amp * ctrl[1]) + \
								((((float) j/(res.x-1) )-0.5) * 2 * amp * ctrl[0]);

	// this had problems with integer divisions:
	//image[i*res.x + j] += (((i/res.y)-0.5) * 2 * amp * ctrl[1]) + (((j/res.x)-0.5) * 2 * amp * ctrl[0]);
	
	return EXIT_SUCCESS;	
}

int modSimDM(char *boundarymask, char *actuatorpat, int nact, float *ctrl, float *image, coord_t res, int niter) {
	int i, x, y;							// random counters
	int ii; 								// iteration counter
	int voltage[nact]; 						// voltage settings for electrodes (in digital units)

	float pi, rho, omega = 1.0, sum, sdif, update;
	int ik;
	pi = 4.0*atan(1);

	SDL_Surface *boundarysurf, *actsurf;	// we load the aperture and actuator images in these SDL surfaces
	float amp=0.3;							// amplitude of the DM response (used for calibration)

	// read boundary mask file if this has not already been done before
	if (boundary == NULL) {
		if (modReadPGM(boundarymask, &boundarysurf) != EXIT_SUCCESS) {
			logErr("Cannot read boundary mask");
			return EXIT_FAILURE;
		}
		if (boundarysurf->w != res.x || boundarysurf->h != res.y) {
			logErr("Boundary mask resolution incorrect! (%dx%d vs %dx%d)", res.x, res.y, boundarysurf->w, boundarysurf->h);
			return EXIT_FAILURE;			
		}
		
		// copy from SDL_Surface to array so we can work with it
		boundary = calloc( res.x*res.y, sizeof(*boundary));
		if (boundary == NULL) {
			logErr("Error allocating memory for boundary image");
			return EXIT_FAILURE;
		}
		else {
			for (y=0; y<res.y; y++)
				for (x=0; x<res.x; x++)
					boundary[y*res.x + x] = (float) getpixel(boundarysurf, x, y);
		
			// normalize the boundary pattern
			for (ik = 0; ik < res.x*res.y; ik++) {
				if (*(boundary + ik) > 0) *(boundary + ik) = 1.;
				else *(boundary + ik) = 0.;
			}
		}
		
		logInfo("Read boundary '%s' succesfully (%dx%d)", boundarymask, res.x, res.y);
	}
	
	// read actuator pattern file if this has not already been done before
	if (act == NULL) {
		if (modReadPGM(actuatorpat, &actsurf) != EXIT_SUCCESS) {
			logErr("Cannot read boundary mask");
			return EXIT_FAILURE;
		}

		if (actsurf->w != res.x || actsurf->h != res.y) {
			logErr("Actuatorn pattern resolution incorrect! (%dx%d vs %dx%d)", res.x, res.y, actsurf->w, actsurf->h);
			return EXIT_FAILURE;			
		}
		
		// copy from SDL_Surface to array so we can work with it
		act = calloc((res.x) * (res.y), sizeof(*act));
		if (act == NULL) {
			logErr("Error allocating memory for actuator image");
			return EXIT_FAILURE;
		}
		else {
			for (y=0; y<res.y; y++)
				for (x=0; x<res.x; x++)
					act[y*res.x + x] = (float) getpixel(actsurf, x, y);
		}
		logInfo("Read actuator pattern '%s' succesfully (%dx%d)", actuatorpat, res.x, res.y);
	}

	// allocate memory for actuator pattern with voltages
	if (actvolt == NULL) {
		actvolt = calloc((res.x) * (res.y), sizeof(*actvolt));
		if (actvolt == NULL) {
			logErr("Error allocating memory for actuator voltage image");
			return EXIT_FAILURE;
		}
	}

	// input linear and c=[-1,1], 'output' must be v=[0,255] and linear in v^2
	logDebug("Simulating DM with voltages:");
	for (ik = 0; ik < nact; ik++) {
		// first simulate rails (i.e. crop ctrl above abs(1))
		if (ctrl[ik] > 1.0) ctrl[ik] = 1.0;
		if (ctrl[ik] < -1.0) ctrl[ik] = -1.0;
		// we do Sqrt(255^2 (i+1) * 0.5) here to convert from [-1,1] (linear) to [0,255] (quadratic)
		voltage[ik] = (int) round( sqrt(65025*(ctrl[ik]+1)*0.5 ) ); //65025 = 255^2
		logDirect("%d ", voltage[ik]);
	}
	logDirect("\n");
	
	// set actuator voltages on electrodes *act is the actuator pattern,
	// where the value of the pixel associates that pixel with an actuator
	// *actvolt will hold this same pattern, but then with voltage
	// related values which serve as input for solving the mirror function
	for (ik = 0; ik < res.x*res.y; ik++){
		i = (int) act[ik];
		if(i > 0)
			actvolt[ik] = pow(voltage[i-1] / 255.0, 2.0) / 75.7856;
			// 75.7856*2 gets 3 um deflection when all voltages 
			// are set to 180; however, since the reflected wavefront sees twice
			// the surface deformation, we simply put a factor of two here
	}

	// compute spectral radius rho and SOR omega factor. These are appro-
	// ximate values. Get the number of iterations.

	rho = (cos(pi/((double)res.x)) + cos(pi/((double)res.y)))/2.0;
	omega = 2.0/(1.0 + sqrt(1.0 - rho*rho));

	if (niter <= 0) // Set the number of iterations if the argument was <= 0
		niter = (int)(2.0*sqrt((double) res.x*res.y));

	// Calculation of the response. This is a Poisson PDE problem
	// where the actuator pattern represents the source term and and
	// mirror boundary a Dirichlet boundary condition. Because of the
	// arbitrary nature of the latter we use a relaxation algorithm.
	// This is a Simultaneous Over-Relaxation (SOR) algorithm described
	// in Press et al., "Numerical Recipes", Section 17.
	
	if (resp == NULL) {
		logDebug("Allocating memory for resp: %dx%d.", res.x, res.y);
		resp = calloc(res.x*res.y, sizeof(*resp));
		if (resp == NULL) {
			logErr("Allocation error, exiting");
			return EXIT_FAILURE;
		}
	}

	// loop over all iterations
	for (ii = 1; ii <= niter; ii++) {
		
		sum = 0.0;
		sdif = 0.0;
		// new loop, hopefully a little smarter. start at 1+res.x because this pixel has four neighbours,
		// stop at the last but (1+res.x) pixel but again this has four neighbours.
		for (i=1+res.x; i<(res.x*res.y)-1-res.x; i++) {
			if (*(boundary + i) > 0) { 		// pixel is in the boundary?
				update = -resp[i] - (actvolt[i] - resp[i-res.x] - resp[i+res.x] - resp[i+1] - resp[i-1])/4.;
				resp[i] = resp[i] + omega*update;
				sum += resp[i];
				sdif += (omega*update)*(omega*update);
			}
			else
				resp[i] = 0.;
			
		}

		sdif = sqrt(sdif/(sum*sum));
		
		if (sdif < SOR_LIM) // Stop iterating if changes are small engouh
			break;
			
	} // end iteration loop

	// *update* the image, so there should already be an image
	for (i = 0; i < res.x*res.y; i++)
		image[i] += amp*resp[i];

	return EXIT_SUCCESS; // successful completion
}

