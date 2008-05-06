/*! 
	@file foam_modules-okodm.c
	@author @authortim
	@date 2008-03-20 10:07

	@brief This file contains routines to drive a 37 actuator Okotech DM using PCI interface

*/

// headers

#include "foam_modules-okodm.h"

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
	if (dm->nchan != 38) {
#ifdef FOAM_MODOKODM_DEBUG
		printf("Warning, number of actuators greater not equal to 38 (is %d). This will not work!\n", dm->nchan);
#else
		logWarn("Warning, number of actuators greater not equal to 38 (is %d). This will not work!", dm->nchan);
#endif
		return EXIT_FAILURE;		
	}

	dm->addr = malloc((size_t) dm->nchan * sizeof(dm->addr));
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
	return drvSetAllOkoDM(dm, dm->minvolt);
}

int drvSetAllOkoDM(mod_okodm_t *dm, int volt) {
	int i;
	
	for (i=1; i< dm->nchan; i++) {
		if (okoWrite(dm->fd, dm->addr[i], volt) == EXIT_FAILURE)
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
	

	
	printf("Settings actuators to low (0) and high (%d) volts repeatedly (20 times):...\n", FOAM_MODOKODM_MAXVOLT);
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
