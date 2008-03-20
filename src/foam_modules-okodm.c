/*! 
	@file foam_modules-okodm.c
	@author @authortim
	@date 2008-03-20 10:07

	@brief This file contains routines to drive a 37 actuator Okotech DM using PCI interface

	\section Info
	
	The Okotech 37ch DM has 38 actuators (one being the substrate) leaving 37 for AO. The mirror
	is controlled through a PCI board. This requires setting some hardware addresses, but not much
	more. See mirror.h and rotate.c supplied on CD with the Okotech mirror for examples.
	
	\section Functions
	
	\li drvSetOkoDM() - Sets the Okotech 37ch DM to a certain voltage set.
	\li drvInitOkoDM() - Initialize the Okotech DM
	\li drvCloseOkoDM() - Close the Okotech DM
	
	\section History
	
*/

#include <gsl/gsl_vector.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <math.h>

#define FOAM_MODOKODM_PORT "/dev/port"
#define FOAM_MODOKODM_NCHAN 38
#define FOAM_MODOKODM_MAXVOLT 255
#define FOAM_MODOKODM_DEBUG 1		//!< set to 1 for debugging, in that case this module compiles on its own

// PCI bus is 32 bit oriented, hence each 4th addres represents valid
// 8bit wide channel 
#define FOAM_MODOKODM_PCI_OFFSET 4

// These are the base addresses for the different boards. In our
// case we have 2 boards, boards 3 and 4 are not used
#define FOAM_MODOKODM_BASE1 0xc000
#define FOAM_MODOKODM_BASE2 0xc400
#define FOAM_MODOKODM_BASE2DM_BASE3 0xFFFF
#define FOAM_MODOKODM_BASE4 0xFFFF


static int Okodminit = 0;
static int Okofd;
static int Okoaddr[38];

static void okoOpen() {
	Okofd = open(FOAM_MODOKODM_PORT, O_RDWR);
	if (Okofd < 0) {	
#ifdef FOAM_MODOKODM_DEBUG
		printf("Could not open port (%s) for Okotech DM: %s", FOAM_MODOKODM_PORT, strerror(errno));
		exit(-1);
#elif
		logErr("Could not open port (%s) for Okotech DM: %s", FOAM_MODOKODM_PORT, strerror(errno));
#endif
	}
}

static void okoSetAddr() {
	int i, b1, b2;

	i = FOAM_MODOKODM_PCI_OFFSET;
	b1 = FOAM_MODOKODM_BASE1;
	b2 = FOAM_MODOKODM_BASE2;

	// board 1:
	Okoaddr[1]=b1+13*i; 
	Okoaddr[2]=b1+21*i; 
	Okoaddr[3]=b1+10*i; 
	Okoaddr[4]=b1+14*i;
	Okoaddr[5]=b1+2*i; 
	Okoaddr[6]=b1+1*i; 
	Okoaddr[7]=b1+9*i; 
	Okoaddr[8]=b1+20*i; 
	Okoaddr[9]=b1+22*i;
	Okoaddr[10]=b1+11*i; 
	Okoaddr[11]=b1+12*i;
	Okoaddr[12]=b1+7*i;
	Okoaddr[13]=b1+4*i;
	Okoaddr[14]=b1+5*i;
	Okoaddr[15]=b1+3*i;
	Okoaddr[16]=b1+0*i;
	Okoaddr[17]=b1+15*i;
	Okoaddr[18]=b1+8*i;
	Okoaddr[19]=b1+23*i;
	
	// board 2:
	Okoaddr[20]=b2+9*i;
	Okoaddr[21]=b2+23*i;
	Okoaddr[22]=b2+22*i;
	Okoaddr[23]=b2+21*i;
	Okoaddr[24]=b2+8*i;
	Okoaddr[25]=b2+4*i;
	Okoaddr[26]=b2+2*i;
	Okoaddr[27]=b2+7*i;
	Okoaddr[28]=b2+5*i;
	Okoaddr[29]=b2+3*i;
	Okoaddr[30]=b2+1*i;
	Okoaddr[31]=b2+0*i;
	Okoaddr[32]=b2+15*i;
	Okoaddr[33]=b2+14*i;
	Okoaddr[34]=b2+13*i;
	Okoaddr[35]=b2+12*i;
	Okoaddr[36]=b2+11*i;
	Okoaddr[37]=b2+10*i;
}

static void okoWrite(int addr, int voltage) {
	off_t offset;
	ssize_t w_out;
	
	// make sure we NEVER exceed the maximum voltage:
	voltage = voltage & FOAM_MODOKODM_MAXVOLT;
	
	offset = lseek(Okofd, addr, SEEK_SET);
	if (offset < 0) {
#ifdef FOAM_MODOKODM_DEBUG
		printf("Could not seek port %s: %s", FOAM_MODOKODM_PORT, strerror(errno));
		exit(-1);
#elif
		logErr("Could not seek port %s: %s", FOAM_MODOKODM_PORT, strerror(errno));
#endif
	}
	w_out = write(Okofd, &voltage, 1);
	if (w_out != 1) {
#ifdef FOAM_MODOKODM_DEBUG
		printf("Could not write to port %s: %s", FOAM_MODOKODM_PORT, strerror(errno));
		exit(-1);
#elif
		logErr("Could not write to port %s: %s", FOAM_MODOKODM_PORT, strerror(errno));
#endif
	}
}

int drvSetOkoDM(gsl_vector_float *ctrl) {
	int i, volt;
	float voltf;
	
	for (i=1; i< (int) ctrl->size; i++) {
		// this maps [-1,1] to [0,255^2] and takes the square root of that range (linear -> quadratic)
		voltf = round(sqrt(65025*(gsl_vector_float_get(ctrl, i)+1)*0.5 )); //65025 = 255^2
		volt = (int) voltf;
#ifdef FOAM_MODOKODM_DEBUG
		printf("(%.1f, %d -> %#x) ", voltf, volt, Okoaddr[i]);
#endif
		okoWrite(Okoaddr[i], volt);
	}
#ifdef FOAM_MODOKODM_DEBUG
	printf("\n");
#endif
	
	return EXIT_SUCCESS;
}

void drvInitOkoDM() {
	// Set the global list of addresses for the various actuators:
	okoSetAddr();
	
	// Open access to the pci card using the global FD Okofd
	okoOpen();
	
	// Indicate success for initialisation
	Okodminit = 1;
}

void drvCloseOkoDM() {
	// Close access to the pci card
	if (close(Okofd) < 0) {
#ifdef FOAM_MODOKODM_DEBUG
		printf("Could not close port (%s) for Okotech DM: %s", FOAM_MODOKODM_PORT, strerror(errno));
		exit(-1);
#elif
		logErr("Could not close port (%s) for Okotech DM: %s", FOAM_MODOKODM_PORT, strerror(errno));
#endif
	}
}


#ifdef FOAM_MODOKODM_DEBUG
int main () {
	int i;
	float volt;
	gsl_vector_float *ctrl;
	ctrl = gsl_vector_float_calloc(FOAM_MODOKODM_NCHAN-1);
	
	// set addr:
	okoSetAddr();

	// set everything to zero:
	printf("setting mirror:\n");
	for (i=0; i<ctrl->size; i++)  {
		volt = ((float) i/ctrl->size)*2-1;
		printf("%f ", volt);
		gsl_vector_float_set(ctrl, i, volt);
	}
	printf("\n");

	drvSetOkoDM(ctrl);
	
	// now manually read stuff:
    off_t offset;
    int w_out;
    unsigned char dat;
	
	for (i=1; i<FOAM_MODOKODM_NCHAN; i++) {
		if ((offset=lseek(Okofd, Okoaddr[i], SEEK_SET)) < 0) {
			fprintf(stderr,"Can not lseek  /dev/port\n");
			exit (-1);
		}

		if ((w_out=read(Okofd, &dat,1)) != 1) {
			fprintf(stderr,"Can not read  /dev/port\n");   
			exit (-1);
		}
		printf("(%d, %d) ", i, dat);
	}
	printf("\n");
	return 0;
}
#endif
