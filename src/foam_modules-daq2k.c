/*! 
	@file foam_modules-daq2k.c
	@author @authortim
	@date 2008-04-01 15:59

	@brief This file contains routines to drive a DaqBoard 2000 PCI board

	\section Info

	The IOtech DaqBoard/2000 series are PCI cards which have several digital and analog I/O ports, which 
	can be used to aqcuire data from various sources. This board can also be used to drive tip-tilt
	mirrors, telescope (using analog outputs) and filterwheels (using several digital output ports)
	though, making it a good choice for a general purpose IO board in AO setups.
 
	This module supports multiple daqboards with various DAC channels, and supports the 8225 digital IO chips 
	providing a total of 3 8-bit ports, of which the last port is split into two 4 bit ports.
 
	It does not support 'banks'.
 
	More information can be found at the manufacturers website: 
	 http://www.iotech.com/catalog/daq/dbseries2.html
 
	Especially take a look at the programmer's manual which has a useful function reference at the end.
 
	This module can compile on its own:
		gcc foam_modules-daq2k.c -lc -lm -ldaqx -Wall -I../../../../misc/daqboard_iotech220_portedto26/include/ -L../../../../misc/daqboard_iotech220_portedto26/lib

	\section Functions
	
	\li drvInitDaq2k() - Initialize the Daqboard 2000 (call this first!)
	\li drvCloseDaq2k() - Close the Daqboard 2000 (call this at the end!)
	\li drvDaqSetDAC() - Write analog output to specific ports (ranges from 0 to 65535 (16b))
	\li drvDaqSetDAC() - Write analog output to all (ranges from 0 to 65535 (16b))
	\li drvDaqSetP2() - Write digital output to specific ports
 
	\section History
	
*/

#include <stdio.h>
#include <stdlib.h>
#define DLINUX 1			// necessary for next header file
#include "daqx_linuxuser.h"
#include <string.h>			// for strerror
#include <errno.h>			// for errno
#include <unistd.h>			// for usleep

#define FOAM_MODDAQ2K_DEBUG 1		//!< set to 1 for debugging, in that case this module compiles on its own
#define FOAM_MODDAQ2K_NBOARDS 1		//!< number of total boards in the system
#define FOAM_MODDAQ2K_BOARDS {"daqBoard2k0"}	//!< array initialier of FOAM_MODDAQ2K_NBOARDS long with the device names for the daqboards
#define FOAM_MODDAQ2K_NCHANS {4}	//!< Number of DAC channels per board used
#define FOAM_MODDAQ2K_MINVOLT {-10.}	//!< Minimum voltage for boards, should be floats
#define FOAM_MODDAQ2K_MAXVOLT {10.}		//!< Maximum voltage for boards, should be floats
#define FOAM_MODDAQ2K_IOP2CONF {12}		//!< Port configuration for 8225 chip on boards, bit 1 is for portA, 2 for portB, 3 for high portC, 4 for low portC. 12 = 8 + 4  = bit 4 + bit 3, so portA=0, portB=0, portChigh=1, portClow=1

static int Daqdacinit = 0;		//!< used to indicate partial DAC initialisation success
static int Daqiop2init = 0;		//!< used to indicate partial digital IO initialisation success

// several static global variables holding configuration for the boards
// const if possible, so we don't accidentally change stuff
static DaqHandleT Daqfds[FOAM_MODDAQ2K_NBOARDS];
static char *Daqnames[] = FOAM_MODDAQ2K_BOARDS;
static const int Daqchancount[] = FOAM_MODDAQ2K_NCHANS;
static const float Daqminvolts[] = FOAM_MODDAQ2K_MINVOLT;
static const float Daqmaxvolts[] = FOAM_MODDAQ2K_MAXVOLT;
static const int Daqiop2conf[] = FOAM_MODDAQ2K_IOP2CONF;


static int initDaqDac(int board) {
	// FD not open? then just return
	if (Daqfds[board] == -1)
		return EXIT_SUCCESS;
	
	int chan;
	DaqError err;	
	CHAR errmsg[512];
	
	printf("Opening %d DAC channels on board %d, channel...", Daqchancount[board], board);
	for (chan=0; chan<Daqchancount[board]; chan++) {
		// configure output mode on this channel to be DdomVoltage (i.e. a constant DC)
		daqDacSetOutputMode(Daqfds[board], DddtLocal, chan, DdomVoltage);
		// set the initial voltage to 0, does the least harm in any situation ;)
		err = daqDacWt(Daqfds[board], DddtLocal, chan, (WORD) 0);
		// oops, we got an error! return immediately, and do not use Daqboard DAC routines anymore
		if (err != DerrNoError) {
			daqFormatError(err, (PCHAR) errmsg);
#ifdef FOAM_MODDAQ2K_DEBUG
			printf("Error writing voltage to DAC ports for board %d: %s\n", board, errmsg);
#elif
			logWarn("Error writing voltage to DAC ports for board %d: %s", board, errmsg);
#endif
			Daqdacinit = 0;			
			return EXIT_FAILURE;
		}
#ifdef FOAM_MODDAQ2K_DEBUG
		printf("%d...", chan);
#endif		
		
	}
	
#ifdef FOAM_MODDAQ2K_DEBUG
	printf("done!\n");
#endif
	
	return EXIT_SUCCESS;
}

static int initDaqIOP2(int board) {
	// FD not open? then just return
	if (Daqfds[board] == -1)
		return EXIT_SUCCESS;
	
	DaqError err;
	CHAR errmsg[512];	
	DWORD config;
	
	// set digital IO ports A, B as outputs, C as input
	// Create 8255 config number for these (0,0,1,1) settings
	//  first 0: A as output
	//  second 0: B as output
	// 3rd, 4th 1: C low and high nibble as inputs
#ifdef FOAM_MODDAQ2K_DEBUG
	printf("Setting up P2 on board %d as: (0x%x, 0x%x, 0x%x, 0x%x) ", board, \
		   Daqiop2conf[board] & 0x01, Daqiop2conf[board] & 0x02, \
		   Daqiop2conf[board] & 0x04, Daqiop2conf[board] & 0x08);
#endif
	
	err = daqIOGet8255Conf(Daqfds[board], (BOOL) Daqiop2conf[board] & 0x01, \
						   (BOOL) Daqiop2conf[board] & 0x02, \
						   (BOOL) Daqiop2conf[board] & 0x04, \
						   (BOOL) Daqiop2conf[board] & 0x08, &config);
	if (err != DerrNoError) {
		daqFormatError(err, (PCHAR) errmsg);
#ifdef FOAM_MODDAQ2K_DEBUG
		printf("Error configuring digital IO on 8255 for board %d: %s\n", board, errmsg);
#elif
		logWarn("Error configuring digital IO on 8255 for board %d: %s", board, errmsg);
#endif
		Daqiop2init = 0;			
		return EXIT_FAILURE;
	}
	
	
	// write settings and config number to internal register
	err = daqIOWrite(Daqfds[board], DiodtLocal8255, Diodp8255IR, 0, DioepP2, config);
	
	if (err != DerrNoError) {
		daqFormatError(err, (PCHAR) errmsg);
#ifdef FOAM_MODDAQ2K_DEBUG
		printf("Error configuring digital IO on 8255 for board %d: %s\n", board, errmsg);
#elif
		logWarn("Error configuring digital IO on 8255 for board %d: %s", board, errmsg);
#endif
		Daqiop2init = 0;
		return EXIT_FAILURE;
	}
	
	// init IO ports to 0 (off) no error checking because we don't really care here
	daqIOWrite(Daqfds[board], DiodtLocal8255, Diodp8255A, 0, DioepP2, 1);
	daqIOWrite(Daqfds[board], DiodtLocal8255, Diodp8255B, 0, DioepP2, 1);
	daqIOWrite(Daqfds[board], DiodtLocal8255, Diodp8255C, 0, DioepP2, 1);
	
#ifdef FOAM_MODDAQ2K_DEBUG
	printf("Successfully set up P2!\n");
#endif
	
	return EXIT_SUCCESS;
}

int drvInitDaq2k() {
	int board;
		
	// init DAC part of the Daqboard, assume success and set to 0 on error
	Daqdacinit = 1;
	// init digital IO channels here, assume success and set to 0 on error
	Daqiop2init = 1;

	// open all daqboards
	for (board=0; board<FOAM_MODDAQ2K_NBOARDS; board++) {
		Daqfds[board] = daqOpen(Daqnames[board]);
		if (Daqfds[board] == -1) {
#ifdef FOAM_MODDAQ2K_DEBUG
			printf("Could not connect to board %d, %s: %s\n", board, Daqnames[board], strerror(errno));
#elif
			logWarn("Could not connect to board %d, %s: %s", board, Daqnames[board], strerror(errno));
#endif
		}

#ifdef FOAM_MODDAQ2K_DEBUG
		printf("Opened daqboard %d\n", board);
#endif
		
		// try to init the DAC circuits
		initDaqDac(board);
		
		// try to init the digital IO circuits
		initDaqIOP2(board);
	}

	
	if (Daqdacinit != 1 && Daqiop2init != 1) {
#ifdef FOAM_MODDAQ2K_DEBUG
		printf("Failed to set up Daqboards\n");
#elif
		logWarn("Failed to set up Daqboards");
#endif
		return EXIT_FAILURE;
	}

	
	if (Daqiop2init != 1)
#ifdef FOAM_MODDAQ2K_DEBUG
		printf("Failed to set IO ports on some Daqboards\n");
#elif
		logWarn("Failed to set IO ports on some Daqboards");
#endif
	
	if (Daqdacinit != 1)
#ifdef FOAM_MODDAQ2K_DEBUG
		printf("Failed to set up DAC units on some Daqboards\n");
#elif
		logWarn("Failed to set up DAC units on some Daqboards");
#endif
	
#ifdef FOAM_MODDAQ2K_DEBUG
	printf("Daqboards are now set up!\n");
#endif
	
	return EXIT_SUCCESS;		
}

void drvCloseDaq2k() {
	int board;
	
	// close all open daqboards (fd != -1)
	for (board=0; board<FOAM_MODDAQ2K_NBOARDS; board++) {
		if (Daqfds[board] != -1)
			daqClose(Daqfds[board]);

	}
}

int drvDaqSetP2(int board, int port, int bitpat) {
	// port must be either 0, 1, 2 or 3 for portA, portB, portC high and low
	// respectively
	if (Daqfds[board] == -1)
		return EXIT_SUCCESS;

	// init IO ports to 0 (off) no error checking because we don't really care here
	switch (port) {
		case 0:
			if ((Daqiop2conf[board] & 0x01) != 0) // port is not output, can't write
				return EXIT_FAILURE;

			daqIOWrite(Daqfds[board], DiodtLocal8255, Diodp8255A, 0, DioepP2, bitpat);
			break;
		case 1:
			if ((Daqiop2conf[board] & 0x02) != 0) // port is not output, can't write
				return EXIT_FAILURE;

			daqIOWrite(Daqfds[board], DiodtLocal8255, Diodp8255B, 0, DioepP2, bitpat);
			break;
		case 2:
			if ((Daqiop2conf[board] & 0x04) != 0) // port is not output, can't write
				return EXIT_FAILURE;

			daqIOWrite(Daqfds[board], DiodtLocal8255, Diodp8255CHigh, 0, DioepP2, bitpat);
			break ;
		case 3:
			if ((Daqiop2conf[board] & 0x08) != 0) // port is not output, can't write
				return EXIT_FAILURE;

			daqIOWrite(Daqfds[board], DiodtLocal8255, Diodp8255CLow, 0, DioepP2, bitpat);
			break;
	}
	
	return EXIT_SUCCESS;
}

void drvDaqSetDAC(int board, int chan, int val) {
	if (Daqfds[board] == -1)
		return;
	
	daqDacWt(Daqfds[board], DddtLocal, (DWORD) chan, (WORD) val);
}

void drvDaqSetDACs(int board, int val) {
	if (Daqfds[board] == -1)
		return;
	
	int i;
	for (i=0; i<Daqchancount[0]; i++)
		daqDacWt(Daqfds[board], DddtLocal, (DWORD) i, (WORD) val);
}

#ifdef FOAM_MODDAQ2K_DEBUG
int main() {
	int i;
	
	if (drvInitDaq2k() != EXIT_SUCCESS)
		exit(-1);
	
	printf("Opened DAQboard!\n");
	
	// set stdout to unbuffered, otherwise we won't see intermediate messages
	setvbuf(stdout, NULL, _IONBF, 0);
	
	// setting digital IO ports now //
	//////////////////////////////////
	printf("Trying to set some bit patterns values on P2:\n");
	printf("\n");
	printf("portA and portB (8b): ");
	for (i=1; i<256; i *= 2) { 
		printf("0x%u...", i);
		if (drvDaqSetP2(0, 0, i) != EXIT_SUCCESS || drvDaqSetP2(0, 1, i) != EXIT_SUCCESS) 
			printf("(failed), ");
		else 
			printf("(ok), ");
		sleep(1);
	}
	i=255;
	printf("0x%u...", i);
	if (drvDaqSetP2(0, 0, i) != EXIT_SUCCESS || drvDaqSetP2(0, 1, i) != EXIT_SUCCESS) 
		printf("(failed), ");
	else 
		printf("(ok), ");
	
	printf("\n");
	sleep(1);
	
	printf("\n");
	printf("portC low and high (4b): ");
	for (i=1; i<16; i *= 2) {
		printf("0x%u...", i);
		if (drvDaqSetP2(0, 2, i) != EXIT_SUCCESS || drvDaqSetP2(0, 3, i) != EXIT_SUCCESS) 
			printf("(failed), ");
		else 
			printf("(ok), ");
		sleep(1);
	}
	
	i=15;
	printf("0x%u...", i);
	if (drvDaqSetP2(0, 2, i) != EXIT_SUCCESS || drvDaqSetP2(0, 3, i) != EXIT_SUCCESS) 
		printf("(failed), ");
	else 
		printf("(ok), ");
	
	printf("\n");
	sleep(1);
	printf("\n");
	
	// setting digital io now, do FW //
	///////////////////////////////////
	
	printf("Will now drive filterwheel connected to port A, sending values 0 through 7 by using the first three bits\n");
	for (i=0; i<8; i++) {
		printf("0x%u...", i);
		drvDaqSetP2(0,0,i);
		sleep(1);
	}
	printf("done\n");
	sleep(1);
	
	// setting analog outputs  now //
	/////////////////////////////////
	printf("Setting some voltages on all %d channels of board 0 now:\n", Daqchancount[0]);
	printf("(going through the whole voltage range in 20 seconds)\n");
	for (i=0; i<=100; i++) {
		if (i % 10 == 0) printf("%d%%", i);
		else printf(".");
		drvDaqSetDACs(0, i*65536/100);
		usleep(200000);
	}
	printf("..done\n");
	printf("\n");
		
	drvCloseDaq2k();	
	printf("Closed DAQboard!\n");
	return EXIT_SUCCESS;
}
#endif
