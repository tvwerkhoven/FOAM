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

#define OKODM_PORT "/dev/port"
#define OKODM_NCHAN 38
#define OKODM_MAXVOLT 255

// PCI bus is 32 bit oriented, hence each 4th addres represents valid
// 8bit wide channel 
#define OKODM_PCI_OFFSET 4

// These are the base addresses for the different boards. In our
// case we have 2 boards, boards 3 and 4 are not used
#define OKODM_BASE1 0xc000
#define OKODM_BASE2 0xc400
#define OKODM_BASE3 0xFFFF
#define OKODM_BASE4 0xFFFF


int Okodminit = 0;
int Okofd;
int Okoaddr[38];

static void okoOpen() {
	Okofd = open(OKODM_PORT, O_RDWR);
	if (Okofd < 0) {
//		logErr("Could not open port (%s) for Okotech DM: %s", OKODM_PORT, strerror(errno));
		printf("Could not open port (%s) for Okotech DM: %s", OKODM_PORT, strerror(errno));
		exit(-1);
	}
}

static void okoSetAddr() {
	int i, b1, b2;

	i = OKODM_PCI_OFFSET;
	b1 = OKODM_BASE1;
	b2 = OKODM_BASE2;

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

static void okoClose() {
	if (close(Okofd) < 0) {
		// logErr("Could not close port (%s) for Okotech DM: %s", OKODM_PORT, strerror(errno));
		printf("Could not close port (%s) for Okotech DM: %s", OKODM_PORT, strerror(errno));
		exit(-1);
	}
}

static void okoWrite(int addr, int voltage) {
	off_t offset;
	ssize_t w_out;
	
	// make sure we NEVER exceed the maximum voltage:
	voltage = voltage & OKODM_MAXVOLT;
	
	offset = lseek(Okofd, addr, SEEK_SET);
	if (offset < 0) {
		printf("Could not seek port %s: %s", OKODM_PORT, strerror(errno));
		exit(-1);
		// logErr("Could not seek port %s: %s", OKODM_PORT, strerror(errno));
	}
	w_out = write(Okofd, &voltage, 1);
	if (w_out != 1) {
		printf("Could not write to port %s: %s", OKODM_PORT, strerror(errno));
		exit(-1);
		// logErr("Could not write to port %s: %s", OKODM_PORT, strerror(errno));
	}
}

int drvSetOkoDM(gsl_vector_float *ctrl) {
	int i, volt;
	if (Okodminit == 0) {
		Okodminit = 1;
		okoOpen();
	}
	
	for (i=0; i< (int) ctrl->size; i++) {
		// this maps [-1,1] to [0,255^2] and takes the square root of that range (linear -> quadratic)
		volt = (int) round(sqrt(65025*(gsl_vector_float_get(ctrl, i)+1)*0.5 )); //65025 = 255^2
		okoWrite(Okoaddr[i], volt);
	}
	
	return EXIT_SUCCESS;
}

int main () {
	int i;
	gsl_vector_float *ctrl;
	ctrl = gsl_vector_float_calloc(OKODM_NCHAN-1);
	
	// set everything to zero:
	drvSetOkoDM(ctrl);
	
	// now manually read stuff:
    off_t offset;
    int w_out, dat;
	
	for (i=1; i<OKODM_NCHAN; i++) {
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
