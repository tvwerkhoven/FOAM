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

int modSimDM(char *boundarymask, char *actuatorpat, int nact, float *ctrl, float *image, int niter) {
	int	 i, j, status;
	long	 ii, i_1j, i1j, ij_1, ij1;
	int    voltage[nact]; // voltage settings for electrodes (in digital units)
	double *resp=NULL;
	double pi, rho, omega = 1.0, sum, sdif, update;
	long   ik;
	
	double *boundary=NULL, *act=NULL;
	int nx, ny, ngray1, ngray2;



	pi = 4.0*atan(1);

	// read boundary mask file 
	if (boundary == NULL) {
		if ((status = read_pgm(boundarymask,&boundary,&nx,&ny,&ngray1)) != EXIT_SUCCESS) {
			logErr("Cannot read boundary mask");
			return EXIT_FAILURE;
		}	
	}
	logDebug("DM controls are:");
	for(ik = 0; ik < nact; ik++)
		logDirect("%3f ", ctrl[ik]);
		
	// Input linear and c=[-1,1], 'output' must be v=[0,255] and linear in v^2
	logDebug("Simulating DM with voltages:");
	for(ik = 0; ik < nact; ik++) {
		// we do Sqrt(255^2 (i+1) * 0.5) here to convert from [-1,1] to [0,255] 
		voltage[ik] = (int) round( sqrt(65025*(ctrl[ik]+1)*0.5 ) ); //65025 = 255^2
		logDirect("%d ", voltage[ik]); // TODO: we don't want printf here
	}
	logDirect("\n");

	for (ik = 0; ik < ny*nx; ik++) {
		if (*(boundary + ik) > 0) {
			*(boundary + ik) = 1.;
		} else {
			*(boundary + ik) = 0.;
		}
	}

// read actuator pattern file
	if (act == NULL) {
		if ((status = read_pgm(actuatorpat,&act,&nx,&ny,&ngray2)) != EXIT_SUCCESS) {
			logErr("Cannot read actuator pattern file");
			return EXIT_FAILURE;
		}
	}

// set actuator voltages on electrodes
	for (ik = 0; ik < nx*ny; ik++){
		i = (int) act[ik];
		if(i>0) {
			act[ik] = pow(voltage[i-1] / 255.0, 2.0) / 75.7856;
		}
// 75.7856*2 gets 3 um deflection when all voltages 
// are set to 180; however, since the reflected wavefront sees twice
// the surface deformation, we simply put a factor of two here
	}

// compute spectral radius rho and SOR omega factor. These are appro-
// ximate values. Get the number of iterations.

	rho = (cos(pi/((double)nx)) + cos(pi/((double)ny)))/2.0;
	omega = 2.0/(1.0 + sqrt(1.0 - rho*rho));
// fprintf(stderr,"Spectral radius:\t%g\tSOR Omega:\t%g\n",rho,omega);

	if (niter <= 0) // Set the number of iterations if the argument was <= 0
		niter = (int)(2.0*sqrt((double)nx*ny));

// fprintf(stderr,"Number of iterations:\t%d\n", niter);

// Calculation of the response. This is a Poisson PDE problem
// where the actuator pattern represents the source term and and
// mirror boundary a Dirichlet boundary condition. Because of the
// arbitrary nature of the latter we use a relaxation algorithm.
// This is a Simultaneous Over-Relaxation (SOR) algorithm described
// in Press et al., "Numerical Recipes", Section 17.


	
	if (resp == NULL) {
		logDebug("Allocating memory for resp: %dx%d.", nx, ny);
		resp = (double *) calloc(nx*ny, sizeof(double));
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
			for (i = 2; i <= nx-1; i += 1) { /* loops over 2-D aperture */
				for (j = 2; j <= ny-1; j += 1) {
	/* --->>> might need to be nx instead of ny, also everywhere below */
				ik = (i - 1)*ny + j - 1; /* the pixel location in the 1-d array */
				if (*(boundary + ik) > 0) { /* pixel is within membrane boundary */
	/* calculate locations of neighbouring pixels */
					i_1j = (i - 2)*ny + j - 1;
					i1j = i*ny + j - 1;
					ij_1 = (i - 1)*ny + j - 2;
					ij1 = (i - 1)*ny + j;

					update = -resp[ik] - (act[ik] - resp[i_1j] - resp[i1j] - 
					resp[ij1] - resp[ij_1])/4.;
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

	ik = 0;
	for (i = 1; i <= nx; i += 1){
		for (j = 1; j <= ny; j += 1){
			image[ik] = resp[ik];
	//printf("%e\n",resp[ik]);
			ik ++;
		} 
	//printf("\n");
	}

	//stat = write_pgm("response2.pgm",resp,nx,ny,255);

	free(act);
	free(boundary);
	free(resp);

	return EXIT_SUCCESS; // successful completion
}


/*
 *============================================================================
 *
 *	read_pgm:	read a portable gray map into double buffer pointed
 *			to by dbuf, leaves dimensions and no. of graylevels.
 *
 *	return value:	0 	normal return
 *			-1	error return
 *
 *============================================================================
 */

int read_pgm(char *fname, double **dbuf, int *t_nx, int *t_ny, int *t_ngray) {
	char		c_int, first_string[110];
	unsigned char	b_in;
	int		i, j, bin_ind, nx, ny, ngray;
	long		ik;
	double		fi;
	FILE		*fp;

	if((fp = fopen(fname,"r"))==NULL){
	    logErr("Error opening pgm file %s!", fname);
		return EXIT_FAILURE;
	}

	// Read magic number

	i = 0;
	while((c_int = getc(fp)) != '\n') {
	    first_string[i] = c_int;
	    i++;
	    if(i > 100) i = 100;
	}
	
	if((strstr(first_string, "P2")) != NULL ) {
	  /*	    fprintf(stderr,
		    "\tPortable ASCII graymap aperture mask detected \n"); */
	    bin_ind = 0;
	} else if((strstr(first_string, "P5")) != NULL ){
	    //logDebug("Portable binary graymap aperture mask detected."); 
	    bin_ind = 1;
	} else {
	    logErr("Unknown magic in pgm file: %s, should be P2 or P5",first_string);
		return EXIT_FAILURE;
	}
	
/*
 *	Skip comments which start with a "#" and read the picture dimensions
 */

l1:
	i = 0;
	while ((c_int = getc(fp)) != '\n') {
		first_string[i] = c_int;
		i++;
		if (i > 100) i = 100;
	}
	if (first_string[0] == '#')
		goto l1;
	else
		sscanf(first_string, "%d %d", &nx, &ny);
		
		/*  	fprintf(stderr, "\tX and Y dimensions: %d %d\n", nx, ny); */
	*t_nx = nx;
	*t_ny = ny;

/*
	*	Read the number of grayscales 
*/

	i = 0;
	while((c_int=getc(fp)) != '\n') {
		first_string[i] = c_int;
		i++;
		if(i > 100) i = 100;
	}
	sscanf(first_string, "%d", &ngray);
		/*	fprintf(stderr, "\tNumber of gray levels:\t%d\n", ngray); */
	*t_ngray = ngray;

/*
	*	Read in graylevel data
*/

	*dbuf = (double *) calloc(nx*ny, sizeof(double));
	if(dbuf == NULL){
		logErr("Buffer allocation error!");
		return EXIT_FAILURE;
	}

	ik = 0;
	for (i = 1; i <= nx; i += 1){
		for (j = 1; j <= ny; j += 1){
			if(bin_ind) {
				if(fread (&b_in, sizeof(unsigned char), 1, fp) != 1){
					logErr("Error reading portable bitmap!");
					return EXIT_FAILURE;
				}
				fi = (double) b_in;
			} else {
				if ((fscanf(fp,"%le",&fi)) == EOF){
					logErr("End of input file reached!");
					return EXIT_FAILURE;
				}
			}
			*(*dbuf + ik) = fi;
			ik ++;
		}  
	}

	fclose(fp);
	return EXIT_SUCCESS;
	
}

