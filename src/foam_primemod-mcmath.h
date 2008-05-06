/*! 
	@file foam_primemod-mcmath.h
	@author @authortim
	@date 2008-04-18 12:55

	@brief This is the prime module header file for 'mcmath', which holds some the definition of some
	datatypes that can vary from setup to setup. There are also some define statement to control
	some aspects of the framework in general.
*/

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

// We always use config.h
#include "config.h"
// We always use the main library for datatypes etc
#include "foam_cs_library.h"
// These are specific for McMath

#ifdef FOAM_MCMATH_DISPLAY
// for displaying stuff (SDL)
#include "foam_modules-display.h"
#endif

// for image file I/O
#include "foam_modules-img.h"
// for okodm things
#include "foam_modules-okodm.h"
// for itifg stuff
#include "foam_modules-itifg.h"
// for daq2k board stuff
#include "foam_modules-daq2k.h"
// for serial port stuff
#include "foam_modules-serial.h"
// for calibrating the image lateron
#include "foam_modules-calib.h"
