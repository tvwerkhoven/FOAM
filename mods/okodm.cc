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

 $Id$
 */
/*! 
	@file foam_modules-okodm.c
	@author Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)
	@date 2008-03-20 10:07

	@brief This file contains routines to drive a 37 actuator Okotech DM using PCI interface
 
 \section Info
 
 The Okotech 37ch DM has 38 actuators (one being the substrate) leaving 37 for AO. The mirror
 is controlled through a PCI board. This requires setting some hardware addresses, but not much
 more. See mirror.h and rotate.c supplied on CD with the Okotech mirror for examples.
 
 Manufacturers website:
 <tt>http://www.okotech.com/content/oko/pics/gallery/Typical%20PDM%2037%20passport_.pdf</tt>
 
 This module also compiles on its own like:\n
 <tt>gcc foam_modules-okodm.c -lm -lc -lgslcblas -lgsl -Wall -DFOAM_MODOKODM_ALONE=1 -std=c99</tt>
 
 \section Functions
 
 \li okoInitDM() - Initialize the Okotech DM
 \li okoSetDM() - Sets the Okotech 37ch DM to a certain voltage set.
 \li okoRstDM() - Resets the Okotech DM to FOAM_MODOKODM_RSTVOLT
 \li okoCloseDM() - Calls okoRstDM, then closes the Okotech DM (call this at the end!)
 
 \section Configuration
 
 There are several things that can be configured about this module. The following defines are used:
 \li \b FOAM_MODOKODM_MAXVOLT (255), the maximum voltage allowed (all voltages are logically AND'd with this value)
 \li \b FOAM_MODOKODM_ALONE (*undef*), ifdef, this module will compile on it's own, imlies FOAM_DEBUG
 \li \b FOAM_DEBUG (*undef*), ifdef, this module will give lowlevel debugs through printf
 
 \section Dependencies
 
 This module depends on GSL because it uses the gsl_vector* datatypes to store DM commands in. 
 This is done because this this format is suitable for doing fast calculations, and
 the control vector is usually the output of some matrix multiplication.
 
 \section History
 
 \li 2008-04-14: api update, defines deprecated, replaced by struct
 \li 2008-04-02: init
 
 
*/

// HEADERS //
/***********/

#include "okodm.h"

// LOCAL FUNCTIONS //
/*******************/

/*!
 @brief This local function opens the mirror and stores the FD
 
 @return EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
 */
static int okoOpen(mod_okodm_t *dm);

/*!
 @brief This local function fills an array with the actuator addresses.
 */
static int okoSetAddr(mod_okodm_t *dm);

/*!
 @brief This local function writes the first byte of an integer to the address given by addr
 
 @return EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
 */
static int okoWrite(int dmfd, int addr, int voltage);

// ROUTINES //
/************/

static int okoOpen(mod_okodm_t *dm) {
	dm->fd = open(dm->port, O_RDWR);
	if (dm->fd < 0) {	
#ifdef FOAM_DEBUG
		printf("Could not open port (%s) for Okotech DM: %s\n", dm->port, strerror(errno));
#else
		logWarn("Could not open port (%s) for Okotech DM: %s", dm->port, strerror(errno));
#endif
		return EXIT_FAILURE;
	}
	
	return EXIT_SUCCESS;
}

static int okoSetAddr(mod_okodm_t *dm) {
	int i, b1, b2;

	i = dm->pcioffset;
	b1 = dm->pcibase[0];
	b2 = dm->pcibase[1];
	
	// malloc address memory
	if (dm->nchan != 38) {
#ifdef FOAM_DEBUG
		printf("Warning, number of actuators greater not equal to 38 (is %d). This will not work!\n", dm->nchan);
#else
		logWarn("Warning, number of actuators greater not equal to 38 (is %d). This will not work!", dm->nchan);
#endif
		return EXIT_FAILURE;		
	}

	dm->addr = malloc((size_t) dm->nchan * sizeof(dm->addr));
	if (dm->addr == NULL) {
#ifdef FOAM_DEBUG
		printf("Could not allocate memory for DM hardware addresses!\n");
#else
		logWarn("Could not allocate memory for DM hardware addresses!\n");
#endif
		return EXIT_FAILURE;
	}

	// board 1:
	dm->addr[1]=b1+13*i; 
	dm->addr[2]=b1+21*i; 
	dm->addr[3]=b1+10*i; 
	dm->addr[4]=b1+14*i;
	dm->addr[5]=b1+2*i; 
	dm->addr[6]=b1+1*i; 
	dm->addr[7]=b1+9*i; 
	dm->addr[8]=b1+20*i; 
	dm->addr[9]=b1+22*i;
	dm->addr[10]=b1+11*i; 
	dm->addr[11]=b1+12*i;
	dm->addr[12]=b1+7*i;
	dm->addr[13]=b1+4*i;
	dm->addr[14]=b1+5*i;
	dm->addr[15]=b1+3*i;
	dm->addr[16]=b1+0*i;
	dm->addr[17]=b1+15*i;
	dm->addr[18]=b1+8*i;
	dm->addr[19]=b1+23*i;
	
	// board 2:
	dm->addr[20]=b2+9*i;
	dm->addr[21]=b2+23*i;
	dm->addr[22]=b2+22*i;
	dm->addr[23]=b2+21*i;
	dm->addr[24]=b2+8*i;
	dm->addr[25]=b2+4*i;
	dm->addr[26]=b2+2*i;
	dm->addr[27]=b2+7*i;
	dm->addr[28]=b2+5*i;
	dm->addr[29]=b2+3*i;
	dm->addr[30]=b2+1*i;
	dm->addr[31]=b2+0*i;
	dm->addr[32]=b2+15*i;
	dm->addr[33]=b2+14*i;
	dm->addr[34]=b2+13*i;
	dm->addr[35]=b2+12*i;
	dm->addr[36]=b2+11*i;
	dm->addr[37]=b2+10*i;
	
	return EXIT_SUCCESS;
}

static int okoWrite(int dmfd, int addr, int voltage) {
	off_t offset;
	ssize_t w_out;
	unsigned char volt8;
	
	// make sure we NEVER exceed the maximum voltage
	// this is a cheap way to guarantee that (albeit inaccurate)
	// however, if voltage > FOAM_MODOKODM_MAXVOLT, things are bad anyway
	voltage = voltage & FOAM_MODOKODM_MAXVOLT;
	
	// store data in 8bit char, as we only write one byte
	volt8 = (unsigned char) voltage;
	
	offset = lseek(dmfd, addr, SEEK_SET);
	if (offset == (off_t) -1) {
#ifdef FOAM_DEBUG
		printf("Could not seek DM port: %s\n", strerror(errno));
#else
		logWarn("Could not seek DM port: %s", strerror(errno));
#endif
		return EXIT_FAILURE;
	}
	w_out = write(dmfd, &volt8, 1);
	if (w_out != 1) {
#ifdef FOAM_DEBUG
		printf("Could not write to DM port: %s\n", strerror(errno));
#else
		logWarn("Could not write to DM port: %s", strerror(errno));
#endif
		return EXIT_FAILURE;
	}
			   
	return EXIT_SUCCESS;
}

int okoSetDM(gsl_vector_float *ctrl, mod_okodm_t *dm) {
	int i, volt;
	float voltf;

// !!!:tim:20080414 removed, not really necessary, blame the user :P
//	if (Okodminit != 1) {
//#ifdef FOAM_DEBUG
//		printf("Mirror not initialized yet, please do that first\n");
//#else
//		logWarn("Mirror not initialized yet, please do that first");
//#endif
//		return EXIT_FAILURE;
//	}
	
	for (i=1; i< (int) ctrl->size; i++) {
		// this maps [-1,1] to [0,255^2] and takes the square root of that range (linear -> quadratic)
		voltf = round(sqrt(65025*(gsl_vector_float_get(ctrl, i)+1)*0.5 )); //65025 = 255^2
		volt = (int) voltf;
		if (okoWrite(dm->fd, dm->addr[i], volt) != EXIT_SUCCESS)
			return EXIT_FAILURE;

#ifdef FOAM_DEBUG
		//printf("(%d) ", volt);
#endif		
	}
#ifdef FOAM_DEBUG
	//printf("\n");
#endif
	
	return EXIT_SUCCESS;
}

int okoRstDM(mod_okodm_t *dm) {
	int i;

	// !!!:tim:20080414 removed, not really necessary, blame the user :P
//	if (Okodminit != 1) {
//#ifdef FOAM_DEBUG
//		printf("Mirror not initialized yet, please do that first\n");
//#else
//		logWarn("Mirror not initialized yet, please do that first");
//#endif
//		return EXIT_FAILURE;
//	}
	return okoSetAllDM(dm, dm->minvolt);
}

int okoSetAllDM(mod_okodm_t *dm, int volt) {
	int i;
	
	for (i=1; i< dm->nchan; i++) {
		if (okoWrite(dm->fd, dm->addr[i], volt) == EXIT_FAILURE)
			return EXIT_FAILURE;
		
	}
	
	return EXIT_SUCCESS;
}


int okoInitDM(mod_okodm_t *dm) {
	// Set the global list of addresses for the various actuators:
	okoSetAddr(dm);
	
	// Open access to the pci card using the global FD Okofd
	if (okoOpen(dm) == EXIT_FAILURE)
		return EXIT_FAILURE;
	
	return EXIT_SUCCESS;
}

int okoCloseDM(mod_okodm_t *dm) {
	// reset the mirror
	if (okoRstDM(dm) == EXIT_FAILURE) {
#ifdef FOAM_DEBUG
		printf("Could not reset the DM to voltage %d\n", dm->midvolt);
#else
		logWarn("Could not reset the DM to voltage %d", dm->midvolt);
#endif		
	};
	
	// Close access to the pci card
	if (!(dm->fd >= 0)) {
#ifdef FOAM_DEBUG
		printf("DM fd not valid, this cannot be an open FD (fd is: %d)\n", dm->fd);
#else
		logWarn("DM fd not valid, this cannot be an open FD (fd is: %d)\n", dm->fd);
#endif
		return EXIT_FAILURE;
	}
	
	if (close(dm->fd) < 0) {
#ifdef FOAM_DEBUG
		printf("Could not close port (%s) for Okotech DM: %s\n", dm->port, strerror(errno));
#else
		logWarn("Could not close port (%s) for Okotech DM: %s", dm->port, strerror(errno));
#endif
		return EXIT_FAILURE;
	}
	
	return EXIT_SUCCESS;
}


#ifdef FOAM_MODOKODM_ALONE
int main () {
	int i;
	float volt;
	gsl_vector_float *ctrl;
	mod_okodm_t defmir = {
		.minvolt = 0,
		.midvolt = 180,
		.maxvolt = 255,
		.nchan = 38,
		.port = "/dev/port",
		.pcioffset = 4,
		.pcibase = {0xc000, 0xc400, 0xffff, 0xffff}
	};
	
	ctrl = gsl_vector_float_calloc(defmir.nchan-1);
	
	if (okoInitDM(&defmir) == EXIT_FAILURE) {
		printf("Failed to init the mirror\n");
		return EXIT_FAILURE;
	}

	// set everything to zero:
	printf("Setting mirror with control vector (values between -1 and 1):\n");
	for (i=0; i < (int) ctrl->size; i++)  {
		volt = ((float) i/ctrl->size)*2-1;
		printf("(%d, %.2f) ", i, volt);
		gsl_vector_float_set(ctrl, i, volt);
	}
	printf("\n");

	printf("Which corresponds to voltages:\n");
	for (i=0; i < (int) ctrl->size; i++)  {
		volt = ((float) i/ctrl->size)*2-1;
		printf("(%d, %d) ", i, (int) round(sqrt(65025*(volt+1)*0.5 )));
	}
	printf("\n");
	
	if (okoSetDM(ctrl, &defmir) != EXIT_SUCCESS) {
		printf("Could not set voltages\n");
		return EXIT_FAILURE;
	}


	printf("Mirror does not give errors (good), now setting actuators one by one\n(skipping 0 because it is the substrate)\n");
	printf("Settings acts with 0.25 second delay:...\n");

	// do unbuffered printing
	setvbuf(stdout, NULL, _IONBF, 0);
	
	for (i=0; i<defmir.nchan-1; i++) {
		// set all to zero
		gsl_vector_float_set_zero(ctrl);
		
		// set one to 1 (max)
		gsl_vector_float_set(ctrl, i, 1);
		
		printf("%d...", i);
		if (okoSetDM(ctrl, &defmir) == EXIT_FAILURE) {
			printf("Could not set voltages!\n");
			return EXIT_FAILURE;
		}
		usleep(250000);
	}
	printf("done\n");
	

	
	printf("Settings actuators to low (0) and high (%d) volts repeatedly (20 times):...\n", FOAM_MODOKODM_MAXVOLT);
	for (i=0; i<20; i++) {
		// set all to -1
		printf("lo..");
		gsl_vector_float_set_all(ctrl, -1.0);
		
		if (okoSetDM(ctrl, &defmir) != EXIT_SUCCESS) {
			printf("FAILED");
			return EXIT_FAILURE;
		}
		
		sleep(1);
		printf("hi..");

		gsl_vector_float_set_all(ctrl, 1.0);
		
		if (okoSetDM(ctrl, &defmir) != EXIT_SUCCESS) {
			printf("FAILED");
			return EXIT_FAILURE;
		}
		
		sleep(1);
	}
	printf("done, cleaning up\n");
	
	if (okoCloseDM(&defmir) == EXIT_FAILURE)
		return EXIT_FAILURE;
	
	printf("exit.\n");
	return 0;
}
#endif
