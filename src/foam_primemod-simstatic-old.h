/*! 
	@file foam_primemod-simstatic.h
	@author @authortim
	@date 2008-03-12

	@brief This header file links the prime module `simstatic' to various modules.
*/

#ifndef FOAM_MODULES_SIMSTATIC
#define FOAM_MODULES_SIMSTATIC

// GENERAL //
/***********/

/*
 * These defines must be defined. Choose values as you like, but
 * don't touch them if you don't need to.
 */

#define FILENAMELEN 64				//!< maximum length for logfile names (no need to touch)
#define COMMANDLEN 1024				//!< maximum length for commands we read over the socket (no need to touch)

#define MAX_CLIENTS 8				//!< maximum number of clients that can connect
#define MAX_THREADS 4				//!< number of threads besides the main thread that can be created (unused atm)
#define MAX_FILTERS 8				//!< maximum number of filters one filterwheel can have

// DATATYPES //
/************/

/* 
 * These datatypes can be expanded as you like, but 
 * do not remove things that are already present!
 */

/*!
 @brief Helper enum for ao calibration mode operation.
 
 You can add your own calibration modes here which you can use to determine
 what kind of calibration a user wants.
 */
typedef enum { // calmode_t
	CAL_PINHOLE,	//!< determine reference shifts after inserting a pinhole
	CAL_INFL,		//!< determine the influence functions for each WFS-WFC pair
	CAL_LINTEST		//!< linearity test for WFCs
} calmode_t;

/*!
 @brief Helper enum for WFC types
 
 This should be enough for now, but can be expanded to include other
 WFCs as well.
 */
typedef enum { // axes_t
	WFC_TT=0,		//!< WFC Type for tip-tilt mirrors
	WFC_DM=1		//!< WFC type for deformable mirrors
} wfctype_t;

/*!
 @brief Helper enum for filterwheel types
 
 This should be enough for now, but can be expanded to include other
 filterwheels as well.
 */
typedef enum { //fwheel_t
	FILT_PINHOLE,	//!< Pinhole used for pinhole calibration
	FILT_OPEN,		//!< Open position, don't filter
	FILT_CLOSED		//!< Closed, don't let light through
} fwheel_t;

#include "config.h"
#include "foam_cs_library.h"		// we link to the main program here (i.e. we use common (log) functions)

#include "foam_modules-display.h"	// we need the display module to show debug output
#include "foam_modules-sh.h"		// we want the SH subroutines so we can track targets
#include "foam_modules-img.h"		// we need img routines
#include "foam_modules-calib.h"		// we want the dark/flat calibration

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

