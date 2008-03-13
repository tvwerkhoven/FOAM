/*! 
	@file foam_primemod-simstatic.h
	@author @authortim
	@date 2008-03-12

	@brief This header file links the prime module `simstatic' to various modules.
*/

#ifndef FOAM_MODULES_SIMSTATIC
#define FOAM_MODULES_SIMSTATIC

#include "foam_cs_library.h"		// we link to the main program here (i.e. we use common (log) functions)
#include "foam_modules-display.h"	// we need the display module to show debug output
#include "foam_modules-sh.h"		// we want the SH subroutines so we can track targets
#include "foam_modules-pgm.h"		// we need PGM routines
//#include "foam_modules-calib.h"		// we want the calibration

// These are defined in foam_cs_library.c
extern control_t ptc;
extern config_t cs_config;

// PROTOTYPES //
/**************/

/*!
@brief Simulates sensor(s) read-out and outputs to ptc.wfs[n].image.

During simulation, this function takes care of simulating the atmosphere, 
the telescope, the tip-tilt mirror, the DM and the (SH) sensor itself, since
these all occur in sequence. The sensor output will be stored in ptc.wfs[n].image,
which must be globally available and allocated.

@return EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
*/
int drvReadSensor();

/*!
@brief This sets the various actuators (WFCs).

This simulates setting the actuators to the right voltages etc. 

@return EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
*/
int drvSetActuator();

/*!
@brief This drives the filterwheel.

This simulates setting the filterwheel. The actual simulation is not done here,
but in drvReadSensor() which checks which filterwheel is being used and 
acts accordingly. With a real filterwheel, this would set some hardware address.

@return EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
*/
int drvFilterWheel(control_t *ptc, fwheel_t mode);

/*!
@brief Fake Control vector calculation, only simulate the computational load.

@return EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
*/
int modCalcCtrlFake(control_t *ptc, const int wfs, int nmodes);

#endif /* FOAM_MODULES_SIMSTATIC */

