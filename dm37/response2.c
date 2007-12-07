// ---------------------------------------------------------------------------
// file: response2.c
//
// Calculates the shape of an electrostatic bulk membrane mirror for
// a given set of electrodes and voltages
//
// Based on the file response.c that had the folloing copyright notice:
/*      (C) Gleb Vdovin 1997                                    */
/*      Send bug reports to gleb@okotech.com                    */
/*                                                              */
/*      Modified and cleaned in 1998                            */
/*      by Prof. Oskar von der Luehe                            */
/*      ovdluhe@kis.uni-freiburg.de                             */
/*                                                              */
// To run the example: response2 apert15.pgm dm37.pgm volt.txt > response2.dat
//
// This version takes a *.pgm input file that describes the electrode
// pattern where pixels belonging to electrode 'n' have the value 'n'.
// A text file contains the 'voltages' that should be applied
// to the electrodes. The 'voltages' range from 0 to 255 (integer only').
//
// The output on stdout contains the wavefront in um. The code also
// writes a response2.pgm file that contains the wavefront in 
// graphical form (PGM file format).
//
// Last modified by ckeller@noao.edu on December 19, 2002
// ---------------------------------------------------------------------------


// include files
    
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> 


// constants

#define	SOR_LIM	  (1.e-8)  // limit value for SOR iteration
#define NACT      37       // number of actuators


// function prototypes



// ---------------------------------------------------------------------------

int main(int argc, char *argv[]){

  void	 error_print();
  FILE	 *fp;
  int	 i, j, stat, nx, ny, ngray1, ngray2, niter;
  long	 ii, i_1j, i1j, ij_1, ij1;
  int    voltage[NACT]; // voltage settings for electrodes (in digital units)
  double *act, *boundary, *resp;
  double pi, rho, omega = 1.0, sum, sdif, update;
  long   ik;

#if defined(M_PI)
  pi = M_PI;
#else
  pi = 3.141592654;
#endif


  // check number of arguments

  if ((argc < 4) || (argc > 5)){ 
    error_print(argv[0]);
    exit(1);
  }


  // read boundary mask file 

  if ((stat = read_pgm(argv[1],&boundary,&nx,&ny,&ngray1)) < 0) {
    fprintf(stderr,"Cannot read boundary mask\n");
    exit(1);
  }
  
  for (ik = 0; ik < ny*nx; ik++){
    if (*(boundary + ik) > 0) {
      *(boundary + ik) = 1.;
    } else {
      *(boundary + ik) = 0.;
    }
  }


  // read actuator pattern file

  if ((stat = read_pgm(argv[2],&act,&nx,&ny,&ngray2)) < 0) {
    fprintf(stderr,"Cannot read actuator pattern file\n");
    exit(1);
  }


  // read actuator voltages

  fp = fopen(argv[3],"r");
  if (fp == NULL) {
    fprintf(stderr,"Cannot read voltages file\n");
    exit(1);
  }
  for (i=0;i<NACT;i++) {// loop over all voltages, one per line
    fscanf(fp,"%d\n",&voltage[i]);
  }
  fclose(fp);
 

  // set actuator voltages on electrodes
  
  for (ik = 0; ik < nx*ny; ik++){
    i = (int) act[ik];
    if(i>0) {
      act[ik] = pow( voltage[i-1] / 255.0, 2.0) / 75.7856;
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
  
  if (argc == 5) {
    sscanf(argv[3], "%d", &niter);
  } else {
    niter = (int)(2.0*sqrt((double)nx*ny));
  }
  // fprintf(stderr,"Number of iterations:\t%d\n", niter);


  // Calculation of the response. This is a Poisson PDE problem
  // where the actuator pattern represents the source term and and
  // mirror boundary a Dirichlet boundary condition. Because of the
  // arbitrary nature of the latter we use a relaxation algorithm.
  // This is a Simultaneous Over-Relaxation (SOR) algorithm described
  // in Press et al., "Numerical Recipes", Section 17.

  resp = (double *) calloc(nx*ny, sizeof(double));
  if(resp == NULL){
    fprintf(stderr,"Allocation error, exiting\n");
    exit(1);
  }
  
  /*  fprintf(stderr,"Iterating:\n"); */
  
  /* loop over all iterations */
  for (ii = 1; ii <= niter; ii++){
    sum = 0.0;
    sdif = 0.0;
    
    /* loops over 2-D aperture */
    for (i = 2; i <= nx-1; i += 1){
      for (j = 2; j <= ny-1; j += 1){
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
	} else {
	  resp[ik] = 0.0;
	}
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
      printf("%e\n",resp[ik]);
      ik ++;
    } 
    printf("\n");
  }
  
  stat = write_pgm("response2.pgm",resp,nx,ny,255);
  
  free(act);
  free(boundary);
  free(resp);

  return 0; // successful completion
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

int read_pgm(char *fname, double **dbuf, int *t_nx, int *t_ny, int *t_ngray)

{
	char		c_int, first_string[110];
	unsigned char	b_in;
	int		i, j, bin_ind, nx, ny, ngray;
	long		ik;
	double		fi;
	FILE		*fp;

	if((fp = fopen(fname,"r"))==NULL){
	    fprintf(stderr,"\t*** Error opening file %s!", fname);
	    return -1;
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
	    fprintf(stderr,
		    "\tPortable binary graymap aperture mask detected \n"); 
	    bin_ind = 1;
	} else {
	    fprintf(stderr,"\tUnknown magic %s!\n",first_string);
	    fprintf(stderr,"\tValid are magic numbers P2 and P5.\n");
	    return -1;
	}
	
/*
 *	Skip comments which start with a "#" and read the picture dimensions
 */

l1:
	i = 0;
  	while((c_int = getc(fp)) != '\n') {
	    first_string[i] = c_int;
	    i++;
	    if(i > 100) i = 100;
	}
	if(first_string[0] == '#') {
	    goto l1;
  	} else {
	    sscanf(first_string, "%d %d", &nx, &ny);
	}
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
	    fprintf(stderr,"\tBuffer allocation error!\n");
	    return -1;
	}

   	ik = 0;
    	for (i = 1; i <= nx; i += 1){
            for (j = 1; j <= ny; j += 1){
		if(bin_ind) {
		    if(fread (&b_in, sizeof(unsigned char), 1, fp) != 1){
			fprintf(stderr,"\tError reading portable bitmap!\n");
			return -1;
		    }
	   	    fi = (double) b_in;
	  	} else {
		    if ((fscanf(fp,"%le",&fi)) == EOF){
		    	fprintf(stderr,"\tEnd of input file reached!\n");
	      		return -1;
		    }
		}
		*(*dbuf + ik) = fi;
		ik ++;
	    }  
	}
   
   	fclose(fp);
	return 0;
	
}


/*
 *============================================================================
 *
 *	write_pgm:	write a portable gray map from double buffer pointed
 *			to by dbuf, given dimensions and no. of graylevels,
 *			to file "fname".
 *
 *	return value:	0 	normal return
 *			-1	error return
 *
 *============================================================================
 */

int write_pgm(char *fname, double *dbuf, int nx, int ny, int ngray)

{
	int		i;
	long		ik;
	double		dmax, dmin, drng;
	FILE		*fp;

	if((fp = fopen(fname,"w"))==NULL){
	    fprintf(stderr,"\t*** Error opening file %s!", fname);
	    return -1;
	}

/*
 *============================================================================
 *	Write file header
 *============================================================================
 */
 
	fprintf(fp,"P2\n");
	fprintf(fp,"#Creator: Response (1998)\n");
	fprintf(fp,"%d %d\n",nx,ny);
	fprintf(fp,"%d\n",ngray);
	
/*
 *============================================================================
 *	Determine data range and write data
 *============================================================================
 */
 
 	dmin = *dbuf; dmax = *dbuf;
	for (ik = 1; ik < nx*ny; ik++ ) {
	    if (*(dbuf + ik) < dmin)
	    	dmin = *(dbuf + ik);
	    if (*(dbuf + ik) > dmax)
	    	dmax = *(dbuf + ik);
	}
	drng = dmax - dmin;
	if (drng == (double)(0.0)) {
	    fprintf(stderr,"*** Output data range is zero!\n");
	    drng = 1.0;
	}

	for (ik = 0; ik < nx*ny; ik++ ) {
	    i = 0;
	    fprintf(fp,"%d ",(int)(ngray*(*(dbuf + ik) - dmin)/drng));
	    if (i >= 40) {
	    	fprintf(fp,"\n");
		i = 0;
	    }
	}
	fprintf(fp,"\n");
	
	fclose(fp);

	return 0;
}



void error_print(char *arr)
{
  fprintf(stderr,"%s calculates the reponse function of a membrane "
	  "mirror.\n",arr);
  fprintf(stderr,"\nUSAGE: %s F1 F2 F3\n"
	  "F1 is the mask PNM file to define the mirror aperture \n"
	  "F2 is the PNM file defining the actuator shape \n"
	  "F3 is a text file containing the integer voltages\n",arr);
}
