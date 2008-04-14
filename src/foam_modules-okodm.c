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
	
	\li drvInitOkoDM() - Initialize the Okotech DM (call this first!)
	\li drvSetOkoDM() - Sets the Okotech 37ch DM to a certain voltage set.
	\li drvRstOkoDM() - Resets the Okotech DM to FOAM_MODOKODM_RSTVOLT
	\li drvCloseOkoDM() - Calls drvRstOkoDM, then closes the Okotech DM (call this at the end!)
 
	\section Configuration
 
	There are several things that can be configured about this module. The following defines are used:
	\li \b FOAM_MODOKODM_MAXVOLT (255), the maximum voltage allowed (all voltages are logically AND'd with this value)
	\li \b FOAM_MODOKODM_ALONE (*undef*), ifdef, this module will compile on it's own, imlies FOAM_MODOKODM_DEBUG
	\li \b FOAM_MODOKODM_DEBUG (*undef*), ifdef, this module will give lowlevel debugs through printf
 
	\section History
 
	\li 2008-04-14: api update, defines deprecated, replaced by struct
	\li 2008-04-02: init

*/

// headers

#include <gsl/gsl_vector.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <math.h>

// defines

#define FOAM_MODOKODM_MAXVOLT 255		//!< Maximum voltage to set to the PCI card

#ifdef FOAM_MODOKODM_ALONE
#define FOAM_MODOKODM_DEBUG 1			//!< set to 1 for debugging, in that case this module compiles on its own
#endif

// PCI bus is 32 bit oriented, hence each 4th addres represents valid
// 8bit wide channel 
//#define FOAM_MODOKODM_PCI_OFFSET 4		//!< This should be 4, perhaps 8 sometimes

// These are the base addresses for the different boards. In our
// case we have 2 boards, boards 3 and 4 are not used
//#define FOAM_MODOKODM_BASE1 0xc000		//!< Get these addresses from lspci -v and look for the PROTO-3 cards
//#define FOAM_MODOKODM_BASE2 0xc400		//!< Get these addresses from lspci -v and look for the PROTO-3 cards
//#define FOAM_MODOKODM_BASE3 0xFFFF		//!< Get these addresses from lspci -v and look for the PROTO-3 cards
//#define FOAM_MODOKODM_BASE4 0xFFFF		//!< Get these addresses from lspci -v and look for the PROTO-3 cards

// Local global variables for tracking configuration (obsoleted)
//static int Okodminit = 0;				//!< This variable is set to 1 if the DM is initialized
//static int Okofd;						//!< This stores the FD to the DM
//static int Okoaddr[38];					//!< This stores the addresses for all actuators

// datatypes

/*!
 @brief Metadata on DM operations, typically an Okotech DM using a PCI board
 
 This struct holds metadata on DM operations. The fields denoted '(user)' must
 be supplied in advance by the user, while '(mod)' will be filled in during
 operation.
 
 Note: To set maxvolt above 255, modify the FOAM_MODOKODM_MAXVOLT define. This define
 overrides maxvolt if it's greater than 255 for your safety. 
 */
typedef struct {
	int minvolt;		//!< (user) minimum voltage that can be applied to the mirror
	int midvolt;		//!< (user) mid voltage (i.e. 'flat')
	int maxvolt;		//!< (user) maximum voltage. note: this value is overridden by FOAM_MODOKODM_MAXVOLT if greater than 255!
	int nchan;			//!< (user) number of channels, including substrate (i.e. 38)
	int *addr;			//!< (mod) pointer to array storing hardware addresses
	int fd;				//!< (mod) fd used with a mirror (to open the port)
	char port[64];		//!< (user) port to use (i.e. "/dev/port")
	int pcioffset;		//!< (user) pci offset to use (4 on 32 bit systems)
	int pcibase[4];		//!< (user) max 4 PCI base addresses
} mod_okodm_t;


// local function prototypes

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
static int okoWrite(int addr, int voltage);

// public function prototypes

/*!
 @brief This function sets the DM to the values given by *ctrl to the DM at *dm
 
 The vector/array *ctrl should hold control values for the mirror which
 range from -1 to 1, the domain being linear (i.e. from 0.5 to 1 gives twice
 the stroke). Since the mirror is actually only linear in voltage^2, this
 routine maps [-1,1] to [0,255] appropriately.
 
 @param [in] *ctrl Holds the controls to send to the mirror in the -1 to 1 range
 @param [in] *dm DM configuration information, filled by drvInitOkoDM()
 @return EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
 */
int drvSetOkoDM(gsl_vector_float *ctrl, mod_okodm_t *dm);

/*!
 @brief Resets all actuators on the DM to dm->midvolt
 
 @param [in] *dm DM configuration information, filled by drvInitOkoDM()
 @return EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
 */
int drvRstOkoDM(mod_okodm_t *dm);

/*!
 @brief Initialize the module (software and hardware)
 
 You need to call this function *before* any other function, otherwise it will
 not work. The struct pointed to by *dm must hold some information to 
 init the DM with, see mod_okodm_t.
 
 @param [in] *dm DM configuration information
 @return EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
 */
int drvInitOkoDM(mod_okodm_t *dm);

/*!
 @brief Close the module (software and hardware)
 
 You need to call this function *after* the last DM command. This
 resets the mirror and closes the file descriptor.
 
 @param [in] *dm DM configuration information, filled by drvInitOkoDM()
 @return EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
 */
int drvCloseOkoDM(mod_okodm_t *dm);

static int okoOpen(mod_okodm_t *dm) {
	dm->fd = open(dm->port, O_RDWR);
	if (dm->fd < 0) {	
#ifdef FOAM_MODOKODM_DEBUG
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
	if (dm->nact != 38) {
#ifdef FOAM_MODOKODM_DEBUG
		printf("Warning, number of actuators greater not equal to 38 (is %d). This will not work!\n", dm->nact);
#else
		logWarn("Warning, number of actuators greater not equal to 38 (is %d). This will not work!", dm->nact);
#endif
		return EXIT_FAILURE;		
	}

	dm->addr = malloc((size_t) dm->nact * sizeof(dm->addr));
	if (dm->addr == NULL) {
#ifdef FOAM_MODOKODM_DEBUG
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
#ifdef FOAM_MODOKODM_DEBUG
		printf("Could not seek DM port: %s\n", strerror(errno));
#else
		logWarn("Could not seek DM port: %s", strerror(errno));
#endif
		return EXIT_FAILURE;
	}
	w_out = write(dmfd, &volt8, 1);
	if (w_out != 1) {
#ifdef FOAM_MODOKODM_DEBUG
		printf("Could not write to DM port: %s\n", strerror(errno));
#else
		logWarn("Could not write to DM port: %s", strerror(errno));
#endif
		return EXIT_FAILURE;
	}
			   
	return EXIT_SUCCESS;
}

int drvSetOkoDM(gsl_vector_float *ctrl, mod_okodm_t *dm) {
	int i, volt;
	float voltf;

// !!!:tim:20080414 removed, not really necessary, blame the user :P
//	if (Okodminit != 1) {
//#ifdef FOAM_MODOKODM_DEBUG
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

#ifdef FOAM_MODOKODM_DEBUG
		//printf("(%d) ", volt);
#endif		
	}
#ifdef FOAM_MODOKODM_DEBUG
	//printf("\n");
#endif
	
	return EXIT_SUCCESS;
}

int drvRstOkoDM(mod_okodm_t *dm) {
	int i;

	// !!!:tim:20080414 removed, not really necessary, blame the user :P
//	if (Okodminit != 1) {
//#ifdef FOAM_MODOKODM_DEBUG
//		printf("Mirror not initialized yet, please do that first\n");
//#else
//		logWarn("Mirror not initialized yet, please do that first");
//#endif
//		return EXIT_FAILURE;
//	}
	
	for (i=1; i< dm->nchan; i++) {
		if (okoWrite(dm->fd, dm->addr[i], dm->midvolt) == EXIT_FAILURE)
			return EXIT_FAILURE;
		
	}
	
	return EXIT_SUCCESS;
}

int drvInitOkoDM(mod_okodm_t *dm) {
	// Set the global list of addresses for the various actuators:
	okoSetAddr(dm);
	
	// Open access to the pci card using the global FD Okofd
	if (okoOpen(dm) == EXIT_FAILURE)
		return EXIT_FAILURE;
	
	return EXIT_SUCCESS;
}

int drvCloseOkoDM(mod_okodm_t *dm) {
	// reset the mirror
	if (drvRstOkoDM(dm) == EXIT_FAILURE) {
#ifdef FOAM_MODOKODM_DEBUG
		printf("Could not reset the DM to voltage %d\n", dm->midvolt);
#else
		logWarn("Could not reset the DM to voltage %d", dm->midvolt);
#endif		
	};
	
	// Close access to the pci card
	if (!(dm->fd >= 0)) {
#ifdef FOAM_MODOKODM_DEBUG
		printf("DM fd not valid, this cannot be an open FD (fd is: %d)\n", dm->fd);
#else
		logWarn("DM fd not valid, this cannot be an open FD (fd is: %d)\n", dm->fd);
#endif
		return EXIT_FAILURE;
	}
	
	if (close(dm->fd) < 0) {
#ifdef FOAM_MODOKODM_DEBUG
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
	
	if (drvInitOkoDM(&defmir) == EXIT_FAILURE) {
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
	
	if (drvSetOkoDM(ctrl, &defmir) != EXIT_SUCCESS) {
		printf("Could not set voltages\n");
		return EXIT_FAILURE;
	}

//	reading back from a PCI card is not always possible,
//	and apparantly it does not work here :P
//
//	// now manually read stuff:
//    off_t offset;
//    int w_out;
//    int dat=0;
//	
//	printf("Data set on %s, now reading back to see if it worked:\n",FOAM_MODOKODM_PORT);
//	
//	for (i=1; i<FOAM_MODOKODM_NCHAN; i++) {
//		if ((offset=lseek(Okofd, dm->addr[i], SEEK_SET)) < 0) {
//			fprintf(stderr,"Can not lseek %s\n", FOAM_MODOKODM_PORT);
//			return EXIT_FAILURE;
//		}
//		printf("(s: %d), ", (int) offset);
//
//		if ((w_out=read(Okofd, &dat,1)) != 1) {
//			fprintf(stderr,"Can not read %s\n", FOAM_MODOKODM_PORT);   
//			return EXIT_FAILURE;
//		}
//		printf("(%d, %#x) ", i, dat);
////		sleeping between read calls is not really necessary, pci is fast enough
////		usleep(1000000);
//	}
//	printf("\n");

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
		if (drvSetOkoDM(ctrl, &defmir) == EXIT_FAILURE) {
			printf("Could not set voltages!\n");
			return EXIT_FAILURE;
		}
		usleep(250000);
	}
	printf("done\n");
	

	
	printf("Settings actuators to low (0) and high (%d) volts repeatedly:...\n", FOAM_MODOKODM_MAXVOLT);
	for (i=0; i<20; i++) {
		// set all to -1
		printf("lo..");
		gsl_vector_float_set_all(ctrl, -1.0);
		
		if (drvSetOkoDM(ctrl, &defmir) != EXIT_SUCCESS) {
			printf("FAILED");
			return EXIT_FAILURE;
		}
		
		sleep(1);
		printf("hi..");

		gsl_vector_float_set_all(ctrl, 1.0);
		
		if (drvSetOkoDM(ctrl, &defmir) != EXIT_SUCCESS) {
			printf("FAILED");
			return EXIT_FAILURE;
		}
		
		sleep(1);
	}
	printf("done, cleaning up\n");
	
	if (drvCloseOkoDM(&defmir) == EXIT_FAILURE)
		return EXIT_FAILURE;
	
	printf("exit.\n");
	return 0;
}
#endif
