/*! 
	@file foam_modules-dm.c
	@author @authortim
	@date December 7, 2007

	@brief This file contains routines to simulate an n-actuator DM.

	To run, see help at simDM()
	
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

#include "foam_cs_library.h"


// constants

#define	SOR_LIM	  (1.e-8)  // limit value for SOR iteration
//#define NACT      37       // number of actuators

// local prototypes
/*!
@brief Reads a pgm file from disk into memory

@param [in] *fname the filename to read
@param [out] **dbuf the buffer that the image will be read to (will be allocated dynamically)
@param [out] *t_nx will hold the x resolution of the image
@param [out] *t_ny will hold the y resolution of the image
@param [out] *t_ngray will hold the number of different graylevels in the image
*/
int read_pgm(char *fname, double **dbuf, int *t_nx, int *t_ny, int *t_ngray);

/*!
@brief Simulates the DM shape as function of input voltages.

This routine, based on response2.c by C.U. Keller, takes a boundarymask,
an actuatorpattern for the DM being simulated, the number of actuators
and the voltages as input. It then calculates the shape of the mirror
in the output variable image. Additionally, niter can be set to limit the 
amount of iterations (0 is auto).

@param [in] *boundarymask The pgm-file containing the boundary mask (aperture)
@param [in] *actuatorpat The pgm-file containing the DM-actuator pattern
@param [in] nact The number of actuators, must be the same as used in \a *actuatorpat
@param [in] *ctrl The control commands array, \a nact long
@param [in] niter The number of iterations, pass 0 for automatic choice
@param [out] *image The DM wavefront correction in um, 2d array.
@return EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
*/
int simDM(char *boundarymask, char *actuatorpat, int nact, float *ctrl, float *image, int niter);

// FUNCTIONS BEGIN //
/*******************/

int simDM(char *boundarymask, char *actuatorpat, int nact, float *ctrl, float *image, int niter) {
	int	 i, j, status, nx, ny, ngray1, ngray2;
	long	 ii, i_1j, i1j, ij_1, ij1;
	int    voltage[nact]; // voltage settings for electrodes (in digital units)
	double *act, *boundary, *resp;
	double pi, rho, omega = 1.0, sum, sdif, update;
	long   ik;

	pi = 4.0*atan(1);

// read boundary mask file 

	if ((status = read_pgm(boundarymask,&boundary,&nx,&ny,&ngray1)) != EXIT_SUCCESS) {
		logErr("Cannot read boundary mask");
		return EXIT_FAILURE;
	}

	logDebug("Simulating DM with voltages:");
	for(ik = 0; ik < nact; ik++) {
		voltage[ik] = (int) round(ctrl[ik]);
		printf("%d ", voltage[ik]); // TODO: we don't want printf here
	}
	printf("\n");

	for (ik = 0; ik < ny*nx; ik++) {
		if (*(boundary + ik) > 0) {
			*(boundary + ik) = 1.;
		} else {
			*(boundary + ik) = 0.;
		}
	}

// read actuator pattern file

	if ((status = read_pgm(actuatorpat,&act,&nx,&ny,&ngray2)) != EXIT_SUCCESS) {
		logErr("Cannot read actuator pattern file");
		return EXIT_FAILURE;
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

	resp = (double *) calloc(nx*ny, sizeof(double));
	if(resp == NULL) {
		logErr("Allocation error, exiting");
		return EXIT_FAILURE;
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

