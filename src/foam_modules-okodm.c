/*! 
	@file foam_modules-okodm.c
	@author @authortim
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
	
	\li drvSetOkoDM() - Sets the Okotech 37ch DM to a certain voltage set.
	\li drvInitOkoDM() - Initialize the Okotech DM (call this first!)
	\li drvCloseOkoDM() - Calls drvRstOkoDM, then closes the Okotech DM (call this at the end!)
	\li drvRstOkoDM() - Resets the Okotech DM to FOAM_MODOKODM_RSTVOLT
 
	\section Configuration
 
	There are several things that can be configured about this module. The following defines are used:
	\li \b FOAM_MODOKODM_PORT ("/dev/port"), the port used throughout the module
	\li \b FOAM_MODOKODM_NCHAN (38), the number of channels on the board
	\li \b FOAM_MODOKODM_MAXVOLT (255), the maximum voltage allowed (all voltages are logically AND'd with this value)
	\li \b FOAM_MODOKODM_RSTVOLT (180), the voltage used to reset the mirror
	\li \b FOAM_MODOKODM_ALONE (*undef*), ifdef, this module will compile on it's own, imlies FOAM_MODOKODM_DEBUG
	\li \b FOAM_MODOKODM_DEBUG (*undef*), ifdef, this module will give lowlevel debugs through printf
	\li \b FOAM_MODOKODM_BASE1 (0xc000), first base address of the PCI card, can be found with lspci -v, look for PROTO-3
	\li \b FOAM_MODOKODM_BASE2 (0xc400), second base address
	\li \b FOAM_MODOKODM_BASE3 (0xFFFF), third base address
	\li \b FOAM_MODOKODM_BASE4 (0xFFFF), fourth base address
 
	\section History
 
	\li 2008-04-02: init

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

#define FOAM_MODOKODM_PORT "/dev/port"	//!< The port for PCI card IO
#define FOAM_MODOKODM_NCHAN 38			//!< Number of channels (actuators + substrate)
#define FOAM_MODOKODM_MAXVOLT 255		//!< Maximum voltage to set to the PCI car

#ifndef FOAM_MODOKODM_RSTVOLT
#define FOAM_MODOKODM_RSTVOLT 180		//!< Use this voltage when resetting the mirror
#endif

#ifdef FOAM_MODOKODM_ALONE
#define FOAM_MODOKODM_DEBUG 1			//!< set to 1 for debugging, in that case this module compiles on its own
#endif

// PCI bus is 32 bit oriented, hence each 4th addres represents valid
// 8bit wide channel 
#define FOAM_MODOKODM_PCI_OFFSET 4		//!< This should be 4, perhaps 8 sometimes

// These are the base addresses for the different boards. In our
// case we have 2 boards, boards 3 and 4 are not used
#define FOAM_MODOKODM_BASE1 0xc000		//!< Get these addresses from lspci -v and look for the PROTO-3 cards
#define FOAM_MODOKODM_BASE2 0xc400		//!< Get these addresses from lspci -v and look for the PROTO-3 cards
#define FOAM_MODOKODM_BASE3 0xFFFF		//!< Get these addresses from lspci -v and look for the PROTO-3 cards
#define FOAM_MODOKODM_BASE4 0xFFFF		//!< Get these addresses from lspci -v and look for the PROTO-3 cards

// Local global variables for tracking configuration
static int Okodminit = 0;				//!< This variable is set to 1 if the DM is initialized
static int Okofd;						//!< This stores the FD to the DM
static int Okoaddr[38];					//!< This stores the addresses for all actuators

// Function prototypes

// local functions
/*!
 @brief This local function opens the mirror and stores the FD in Okofd
 
 @return EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
 */
static int okoOpen();
/*!
 @brief This local function fills the Okoaddr array with the actuator addresses.
 */
static void okoSetAddr();
/*!
 @brief This local function writes the first byte of an integer to the address given by addr
 
 @return EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
 */

static int okoWrite(int addr, int voltage);
// public functions
/*!
 @brief This function sets the DM to the values given by *ctrl
 
 The vector/array *ctrl should hold control values for the mirror which
 range from -1 to 1, the domain being linear (i.e. from 0.5 to 1 gives twice
 the stroke). Since the mirror is actually only linear in voltage^2, this
 routine maps [-1,1] to [0,255] appropriately.
 
 @param [in] *ctrl Holds the controls to send to the mirror in the -1 to 1 range
 @return EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
 */
int drvSetOkoDM(gsl_vector_float *ctrl);
/*!
 @brief Resets all actuators on the DM to FOAM_MODOKODM_RSTVOLT
 
 @return EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
 */
int drvRstOkoDM();
/*!
 @brief Initialize the module (software and hardware)
 
 You need to call this function *before* any other function, otherwise it will
 not work.
 
 @return EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
 */
int drvInitOkoDM();
/*!
 @brief Close the module (software and hardware)
 
 You need to call this function *after* the last DM command. This
 resets the mirror and closes the file descriptor.
 
 @return EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
 */
int drvCloseOkoDM();

static int okoOpen() {
	Okofd = open(FOAM_MODOKODM_PORT, O_RDWR);
	if (Okofd < 0) {	
#ifdef FOAM_MODOKODM_DEBUG
		printf("Could not open port (%s) for Okotech DM: %s\n", FOAM_MODOKODM_PORT, strerror(errno));
#else
		logWarn("Could not open port (%s) for Okotech DM: %s", FOAM_MODOKODM_PORT, strerror(errno));
#endif
		return EXIT_FAILURE;
	}
	
	return EXIT_SUCCESS;
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

static int okoWrite(int addr, int voltage) {
	off_t offset;
	ssize_t w_out;
	
	// make sure we NEVER exceed the maximum voltage
	// this is a cheap way to guarantee that (albeit inaccurate)
	// however, if voltage > 255, things are bad anyway
	voltage = voltage & FOAM_MODOKODM_MAXVOLT;
	
	offset = lseek(Okofd, addr, SEEK_SET);
	if (offset < 0) {
#ifdef FOAM_MODOKODM_DEBUG
		printf("Could not seek port %s: %s\n", FOAM_MODOKODM_PORT, strerror(errno));
#else
		logWarn("Could not seek port %s: %s", FOAM_MODOKODM_PORT, strerror(errno));
#endif
		return EXIT_FAILURE;
	}
	w_out = write(Okofd, &voltage, 1);
	if (w_out != 1) {
#ifdef FOAM_MODOKODM_DEBUG
		printf("Could not write to port %s: %s\n", FOAM_MODOKODM_PORT, strerror(errno));
#else
		logWarn("Could not write to port %s: %s", FOAM_MODOKODM_PORT, strerror(errno));
#endif
		return EXIT_FAILURE;
	}
	
#ifdef FOAM_MODOKODM_DEBUG
	printf("(w: %d), ", (int) offset);
#endif
		   
	return EXIT_SUCCESS;
}

int drvSetOkoDM(gsl_vector_float *ctrl) {
	int i, volt;
	float voltf;
	
	if (Okodminit != 1) {
#ifdef FOAM_MODOKODM_DEBUG
		printf("Mirror not initialized yet, please do that first\n");
#else
		logWarn("Mirror not initialized yet, please do that first");
#endif
		return EXIT_FAILURE;
	}
	
	for (i=1; i< (int) ctrl->size; i++) {
		// this maps [-1,1] to [0,255^2] and takes the square root of that range (linear -> quadratic)
		voltf = round(sqrt(65025*(gsl_vector_float_get(ctrl, i)+1)*0.5 )); //65025 = 255^2
		volt = (int) voltf;
		if (okoWrite(Okoaddr[i], volt) != EXIT_SUCCESS)
			return EXIT_FAILURE;

#ifdef FOAM_MODOKODM_DEBUG
		printf("(%d) ", volt);
#endif		
	}
#ifdef FOAM_MODOKODM_DEBUG
	//printf("\n");
#endif
	
	return EXIT_SUCCESS;
}

int drvRstOkoDM() {
	int i;
	
	if (Okodminit != 1) {
#ifdef FOAM_MODOKODM_DEBUG
		printf("Mirror not initialized yet, please do that first\n");
#else
		logWarn("Mirror not initialized yet, please do that first");
#endif
		return EXIT_FAILURE;
	}
	
	for (i=1; i< FOAM_MODOKODM_NCHAN; i++) {
		if (okoWrite(Okoaddr[i], FOAM_MODOKODM_RSTVOLT) == EXIT_FAILURE)
			return EXIT_FAILURE;
		
	}
	
	return EXIT_SUCCESS;
}

int drvInitOkoDM() {
	// Set the global list of addresses for the various actuators:
	okoSetAddr();
	
	// Open access to the pci card using the global FD Okofd
	if (okoOpen() == EXIT_FAILURE)
		return EXIT_FAILURE;
	
	// Indicate success for initialisation
	Okodminit = 1;
		
	return EXIT_SUCCESS;
}

int drvCloseOkoDM() {
	// reset the mirror
	if (drvRstOkoDM() == EXIT_FAILURE) {
#ifdef FOAM_MODOKODM_DEBUG
		printf("Could not reset the DM to voltage %d\n", FOAM_MODOKODM_RSTVOLT);
#else
		logWarn("Could not reset the DM to voltage %d", FOAM_MODOKODM_RSTVOLT);
#endif		
	};
	
	// Close access to the pci card
	if (Okodminit != 1)
#ifdef FOAM_MODOKODM_DEBUG
		printf("DM was never initialized! Trying to close FD anyone in case it's open, might cause an error\n");
#else
		logWarn("DM was never initialized! Trying to close FD anyone in case it's open, might cause an error");
#endif
	
	// we're done
	Okodminit = 0;

	if (close(Okofd) < 0) {
#ifdef FOAM_MODOKODM_DEBUG
		printf("Could not close port (%s) for Okotech DM: %s\n", FOAM_MODOKODM_PORT, strerror(errno));
#else
		logWarn("Could not close port (%s) for Okotech DM: %s", FOAM_MODOKODM_PORT, strerror(errno));
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
	ctrl = gsl_vector_float_calloc(FOAM_MODOKODM_NCHAN-1);
	
	if (drvInitOkoDM() == EXIT_FAILURE) {
		printf("Failed to init the mirror\n");
		return EXIT_FAILURE;
	}

	// set everything to zero:
	printf("Setting mirror with control vector (values between -1 and 1):\n");
	for (i=0; i<ctrl->size; i++)  {
		volt = ((float) i/ctrl->size)*2-1;
		printf("(%d, %.2f) ", i, volt);
		gsl_vector_float_set(ctrl, i, volt);
	}
	printf("\n");

	printf("Which corresponds to voltages:\n");
	for (i=0; i<ctrl->size; i++)  {
		volt = ((float) i/ctrl->size)*2-1;
		printf("(%d, %d) ", i, (int) round(sqrt(65025*(volt+1)*0.5 )));
	}
	printf("\n");
	
	if (drvSetOkoDM(ctrl) != EXIT_SUCCESS) {
		printf("Could not set voltages\n");
		return EXIT_FAILURE;
	}
		
	// now manually read stuff:
    off_t offset;
    int w_out;
    int dat=0;
	
	printf("Data set on %s, now reading back to see if it worked:\n",FOAM_MODOKODM_PORT);
	
	for (i=1; i<FOAM_MODOKODM_NCHAN; i++) {
		if ((offset=lseek(Okofd, Okoaddr[i], SEEK_SET)) < 0) {
			fprintf(stderr,"Can not lseek %s\n", FOAM_MODOKODM_PORT);
			return EXIT_FAILURE;
		}
		printf("(s: %d), ", (int) offset);

		if ((w_out=read(Okofd, &dat,1)) != 1) {
			fprintf(stderr,"Can not read %s\n", FOAM_MODOKODM_PORT);   
			return EXIT_FAILURE;
		}
		printf("(%d, %#x) ", i, dat);
//		sleeping between read calls is not really necessary, pci is fast enough
//		usleep(1000000);
	}
	printf("\n");

	printf("Mirror does not give errors (good), now setting actuators one by one\n(skipping 0 because it is the substrate)\n");
	printf("Settings acts with 0.25 second delay:...\n");
	
	for (i=0; i<FOAM_MODOKODM_NCHAN-1; i++) {
		// set all to zero
		gsl_vector_float_set_zero(ctrl);
		
		// set one to 1 (max)
		gsl_vector_float_set(ctrl, i, 1);
		
		printf("%d...", i);
		// because we don't add a \n, we need fflush
		fflush(stdout);
		if (drvSetOkoDM(ctrl) == EXIT_FAILURE) {
			printf("Could not set voltages!\n");
			return EXIT_FAILURE;
		}
		usleep(250000);
	}
	printf("done\n");
	
	printf("Settings single actuators randomly over the DM a 1000 times without delay:...");
	for (i=0; i<1000; i++) {
		// set all to zero
		gsl_vector_float_set_zero(ctrl);
		
		// set one to 1 (max)
		gsl_vector_float_set(ctrl, (int) drand48()*(FOAM_MODOKODM_NCHAN-1), 1);
		
		printf(".");
		fflush(stdout);
		if (drvSetOkoDM(ctrl) == EXIT_FAILURE) {
			printf("Could not set voltages!\n");
			return EXIT_FAILURE;
		}
	}
	printf("done, cleaning up\n");
	
	if (drvCloseOkoDM() == EXIT_FAILURE)
		return EXIT_FAILURE;
	
	return 0;
}
#endif
