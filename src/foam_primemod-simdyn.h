/*! 
	@file foam_primemod-simdyn.h
	@author @authortim
	@date 2008-07-03

	@brief Header file for the dynamical simulation prime module
*/

#ifndef FOAM_PRIME_SIMDYN
#define FOAM_PRIME_SIMDYN

// GENERAL //
/***********/

/*
 * These must be defined. Choose values as you like, but
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
	CAL_LINTEST,	//!< linearity test for WFCs
	CAL_SUBAPSEL,	//!< For subaperture selection
	CAL_DARK,		//!< dark fielding
	CAL_FLAT,		//!< flat fielding
	CAL_DARKGAIN	//!< generate dark and gain only for the subapertures for fast dark/flat fielding
} calmode_t;

/*!
 @brief Enum for WFC types
 
 A list of all WFC's used in the system
 */
typedef enum { // axes_t
	WFC_TT=0,		//!< WFC Type for tip-tilt mirrors
	WFC_DM=1		//!< WFC type for deformable mirrors
} wfctype_t;

/*!
 @brief Enum for filterwheel types

 This enum includes all filterwheels present in the hardware system. 
 */
typedef enum { //filter_t
	FILT_PINHOLE,	//!< Pinhole used for subaperture selection (pinhole in front of SH lenslet array, after TT/DM)
    FILT_OPEN,      //!< Normal operations filter position
	FILT_CLOSED,	//!< Closed filter position
    FILT_TARGET     //!< A target for doing stuff
} filter_t;

// We always use config.h
#include "config.h"
// We always use the main library for datatypes etc
#include "foam_cs_library.h"

// ROUTINE PROTOTYPES //
/**********************/

// These *must* be defined in a prime module
int drvSetupHardware(control_t *ptc, aomode_t aomode, calmode_t calmode);
int drvSetActuator(control_t *ptc, int wfc);
int drvGetImg(control_t *ptc, int wfs);

// LIBRARIES //
/*************/

#ifdef FOAM_SIMDYN_DISPLAY
// for displaying stuff (SDL)
#include "foam_modules-dispcommon.h"
#endif

// for image file I/O
#include "foam_modules-img.h"
// for calibrating the image lateron
#include "foam_modules-calib.h"
// for simulation of DM, TT, wavefront propagation etc
#include "foam_modules-sim.h"
// for sh wfs tracking
#include "foam_modules-sh.h"


// These are simstatic specific (for the time being)
int MMAvgFramesByte(control_t *ptc, gsl_matrix_float *output, wfs_t *wfs, int rounds);
int MMDarkFlatFullByte(wfs_t *wfs, mod_sh_track_t *shtrack);
int MMDarkFlatSubapByte(wfs_t *wfs, mod_sh_track_t *shtrack);


#endif // #ifndef FOAM_PRIME_SIMSTATIC

