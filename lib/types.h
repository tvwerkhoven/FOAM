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
 @file libfoam.h
 @brief This file is the main library for FOAM
 @author Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)
 @date 2008-07-15 18:06
 
 This header file contains all functions used by the framework of FOAM.
 In addition to that, it contains a lot of structs to hold data used in 
 FOAM. These include things like the state of the AO system (\c control_t), 
 as well as some structs to track network connections to the CS.
 */

#ifndef __TYPES_H__
#define __TYPES_H__

// INCLUDES //
/************/
#include <gsl/gsl_linalg.h> 		// this is for SVD / matrix datatype
#include <pthread.h>
#include "autoconfig.h"

// these were used in ITIFG
//#include <sys/select.h> //?
//#include <limits.h> 				// LINE_MAX
//#include <sys/uio.h> //?
//#include <inttypes.h> //?
//#include <xlocale.h> //?


// GLOBAL VARIABLES //
/********************/

#define LOG_SOMETIMES 1
#define LOG_NOFORMAT 2

#define FILENAMELEN 64				//!< maximum length for logfile names (no need to touch)
#define COMMANDLEN 1024				//!< maximum length for commands we read over the socket (no need to touch)

#define MAX_CLIENTS 8				//!< maximum number of clients that can connect
#define MAX_THREADS 4				//!< number of threads besides the main thread that can be created (unused atm)
#define MAX_FILTERS 8				//!< maximum number of filters one filterwheel can have

// STRUCTS AND TYPES //
/*********************/

/*!
 @brief We use this to define integer 2-vectors (resolutions etc)
 */
typedef struct {
	int x;			//!< x coordinate
	int y;			//!< y coordinate
} coord_t;

/*!
 @brief We use this to define floating point 2-vectors (displacements etc)
 */
typedef struct {
	float x;		//!< x coordinate
	float y;		//!< y coordinate
} fcoord_t;

/*!
 @brief We use this to store gain information for WFC's
 */
typedef struct {
	float p;		//!< proportional gain
	float i;		//!< integral gain
	float d;		//!< differential gain
} gain_t;

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
	CAL_LINTEST,		//!< linearity test for WFCs
	CAL_DARK,
	CAL_DARKGAIN,
	CAL_SUBAPSEL,
	CAL_FLAT
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
typedef enum { // filter_t
	FILT_PINHOLE,	//!< Pinhole used for pinhole calibration
	FILT_OPEN,		//!< Open position, don't filter
	FILT_CLOSED		//!< Closed, don't let light through
} filter_t;


/*!
 @brief Stores the mode of the AO system.
 */
typedef enum { // aomode_t
	AO_MODE_OPEN=0,	//!< Open loop mode
	AO_MODE_CLOSED, //!< Closed loop mode
	AO_MODE_CAL, 	//!< Calibration mode (in conjunction with calmode_t)
	AO_MODE_LISTEN,	//!< Listen mode (idle)
	AO_MODE_SHUTDOWN //!< Set to this mode for the worker thread to finish
} aomode_t;

/*! 
 @brief This enum is used to distinguish between various datatypes for processing.
 
 Instead of using bpp or something else, this more general datatype identification
 also allows identification of foreign datatypes like a GSL matrix or non-integer
 datatypes (which is hard to distinguish between if only using bpp) like floats.
 
 It is used by functions that accept multiple datatypes, or will be accepting this
 in later versions. This way, routines can work on uint8_t data as well as on
 uint16_t data. The problem arises from the fact that cameras give different
 bitdepth outputs, meaning that routines working on this output need to be able
 to cope with different datatypes.
*/
typedef enum {
	DATA_UINT8,			//!< ID for uint8_t
	DATA_UINT16,		//!< ID for uint16_t
	DATA_FL,			  //!< ID for float
	DATA_GSL_M_F,		//!< ID for gsl_matrix_float
	DATA_GSL_V_F		//!< ID for gsl_vector_float
} foam_datat_t;

/*!
 @brief AO scanning mode enum
 
 This is used to distinguish between different AO modes. Typically, AO
 corrects both in X and Y direction, but in certain cases it might be
 useful to work only in one of the two, where only contrast in one
 direction is available (i.e. solar limb) as opposed to both directions
 (i.e. sunspot or planet).
 */
typedef enum { // axes_t
	AO_AXES_XY=0,		//!< Scan in X and Y direction
	AO_AXES_X,		//!< Scan X direction only
	AO_AXES_Y		//!< Scan Y direction only
} axes_t;

/*!
 @brief Struct for filter wheel identification (used by control_t)
 
 This datatype must be used by the user to configure the AO system.
 To do anything useful, FOAM must know what filterwheels you are using,
 and therefore you must fill in the (user) fields at the beginning.
 */
typedef struct {
	char *name;			//!< (user) Filterwheel name
	int nfilts;			//!< (user) Number of filters present in this wheel
	filter_t curfilt;	//!< (foam) Current filter in place
	filter_t filters[MAX_FILTERS];	//!< (user) All filters present in this wheel
    int delay;          //!< (user) The time in seconds the filterwheel needs to adjust, which is used in a sleep() call to give the filterwheel time
    int id;             //!< (user) a unique ID to identify the filterwheel
} filtwheel_t;

/*!
 @brief Store WFC information. Used by type \c control_t.
 
 This datatype must be used by the user to configure the AO system.
 To do anything useful, FOAM must know what WFSs and WFCs you are using,
 and therefore you must fill in the (user) fields at the beginning.
 See dummy prime module for details.
 
 ctrl holds the control signals for the actuators, which are normalized to
 [-1,1] and must be converted to true signals by a driver module. Since a WFC
 might have a bigger correction range than that can be measured by the wavefront
 sensor (SH), calrange can be used to limit this range during calibration. I.e.
 if you have a tip-tilt mirror which can correct more than the size of the image,
 calibration will not work if jolting it from -1 to 1. Instead, calrange[0] and
 calrange[1] are used to move the actuators.
 
 See dummy prime module for details. 
 */
typedef struct { // wfc_t
	char *name;					//!< (user) name for the specific WFC
	int nact;					//!< (user) number of actuators in this WFC
	gsl_vector_float *ctrl;		//!< (foam) pointer to array of controls for the WFC, must be [-1,1] (i.e. `voltages')
	gain_t gain;				//!< (user) gain used in calculating the new controls
	wfctype_t type;				//!< (user) type of WFC we're dealing with (0 = TT, 1 = DM)
	float calrange[2];			//!< (user) the range over which the calibration should be done (should be same datatype as ctrl)
    int id;                     //!< (user) a unique ID to identify the actuator
} wfc_t;

/*!
 @brief Stores WFS information. Used by type \c control_t.
 
 This datatype must be used by the user to configure the AO system.
 To do anything useful, FOAM must know what WFSs and WFCs you are using,
 and therefore you must fill in the (user) fields at the beginning.
 
 This datatype provides information on the WFSs used. It provides memory
 for some general information (name, resolution, bpp), as well as
 calibration images (dark, flat, sky, etc) and the scanning direction.
 
 The calibration files are used as follows: the sky-, dark- and flatfields
 are stored in formats on disk that provide higher precision than the sensor
 itself. This is logical as the darkfield is an average of many frames, and
 if the sensor is 8 bit, the darkfield can be determined much more accurately
 than this 8 bit. Therefore the fields are stored in float format, specifically
 gsl_matrix_float as this dataformat provides easy matrix handling and
 file I/O.
 
 Once the fields are taken and stored to disk, during AO operations,
 they are copied to *dark, *gain and *corr in a fast datatype, usually the sensor's.
 If the sensor for example is 8 bit, is makes sense to store the dark, gain and corr
 also in an integer type, since this prevents type conversion between floats and ints.
 To circumvent the precision problem one might have, instead of storing the actual
 darkfield, it is multiplied by for example 255 to provide better precision
 
 To further speed up the dark/flat-fielding process, some pre-calculation can be
 done. When doing a typical (raw-dark)/(flat-dark) calculation, flat-dark is
 always the same. Therefore, gain can hold 1/(flat-dark) such that this 
 is already precalculated. Corr is an additional 'field' that can be used to scale
 the incoming sensor image.

 See dummy prime module for details.
 */
typedef struct { // wfs_t
	char *name;						//!< (user) name of the specific WFS
	coord_t res;					//!< (user) x,y-resolution of this WFS
	int bpp;						//!< (user) bits per pixel to use when reading the sensor (only 8 or 16 atm)
    
	void *image;					//!< (foam) pointer to the WFS output
	gsl_matrix_float *darkim;		//!< (foam) darkfield for the CCD, in floats for better precision
	gsl_matrix_float *flatim;		//!< (foam) flatfield for the CCD (actually: flat-dark, as we never use flat directly), in floats
	gsl_matrix_float *skyim;		//!< (foam) skyfield for the CCD, in floats
	gsl_matrix_float *corrim;		//!< (foam) corrected image to be processed, in floats
	void *dark;				//!< (foam) dark field actually used in calculations (actually for SH)
	void *gain;				//!< (foam) gain used (1/(flat-dark)) in calculations (actually for SH)
	void *corr;				//!< (foam) this is used to store the corrected image if we're doing closed loop (we only dark/flat the subapts we're using here and we do it in fast ASM code)
	
	char *darkfile;					//!< (user) filename for the darkfield calibration
	char *flatfile;					//!< (user) filename for the flatfield calibration
	char *skyfile;		 			//!< (user) filename for the flatfield calibration
	int fieldframes;				//!< (user) take this many frames when dark or flatfielding.
    
	axes_t scandir; 				//!< (user) scanning direction(s) used
    int id;                         //!< (user) a unique ID to identify the wfs
} wfs_t;


/*! 
 @brief Stores the state of the AO system
 
 This struct is used to store several variables indicating the state of the AO system 
 which are shared between the different threads. The thread interfacing with user(s)
 can then read these variables and report them to the user, or change them to influence
 the behaviour.
 
 The struct should be configured by the user in the prime module c-file for useful operation.
 (user) fields must be configured by the user, (foam) fields should be left untouched,
 although it is generally safe to read these fields.
 
 The logfrac field is used to stop superfluous logging. See logInfo() and logDebug()
 documentation for details. Errors and warnings are always logged/displayed, as these
 shouldn't occur, and supressing these are generally unwanted.
 
 The datalog* variables can be used to do miscellaneous logging to, in addition to general
 operational details that are logged to the debug, info and error logfiles. Logging 
 to this file must be taken care of by the prime module, 
 
 Also take a look at wfs_t, wfc_t and filtwheel_t.
 */
typedef struct { // control_t
	aomode_t mode;		//!< (user) defines the mode the AO system is in, default AO_MODE_LISTEN
	calmode_t calmode;	//!< (user) defines the possible calibration modes, default CAL_PINHOLE
	time_t starttime;	//!< (foam) stores the starting time of the system
	time_t lasttime;	//!< (foam) use this to track the instantaneous framerate
    
	long frames;		//!< (foam) store the number of frames parsed
	long capped;			//!< (foam) stores the number of frames captured earlier (i.e. what files already exist)
	unsigned long saveimg; //!< (user) if this var is set to non-zero, the next 'saveimg' frames are stored to disk
	float fps;		//!< (foam) stores the currect FPS
    
	int logfrac;        //!< (user) fraction to log for certain info and debug (1 is always, 50 is 1/50 times), default 1000

    // WFS variables
	int wfs_count;		//!< (user) number of WFSs in the system, default 0
	wfs_t *wfs;			//!< (user) pointer to a number of wfs_t structs, default NULL
	
    // WFC variables
	int wfc_count;		//!< (user) number of WFCs in the system, default 0
	wfc_t *wfc;			//!< (user) pointer to a number of wfc_t structs, default NULL
	
    // Filterwheel variables
	int fw_count;		//!< (user) number of fwheels in the system, default 0
	filtwheel_t *filter; //!< (user) stores the filterwheel, default NULL
	
} control_t;


/*!
 @brief Loglevel, one of LOGNONE, LOGERR, LOGINFO or LOGDEBUG
 
 This specifies the verbosity of logging, going from LOGNONE, LOGERR, LOGINFO
 and to LOGDEBUG in increasing amount of logging details. For production systems,
 LOGINFO should suffice, while developers might want to use LOGDEBUG.
 */
typedef enum { // level_t
	LOGNONE, 	//!< Log nothing
	LOGERR, 	//!< Log only errors
	LOGINFO, 	//!< Log info and errors
	LOGDEBUG	//!< Log debug messages, info and errors
} level_t;

/*!
 @brief Struct to configuration data in.
 
 This struct stores things like the IP and port it should be listening on, the 
 files to log error, info and debug messages to and whether or not to use
 syslog. (user) fields can be modified by the user in the prime module c-file
 (see dummy prime module for example). These fields are initialized with some
 default values. (foam) fields should never be touched by a user (although reading
 should be ok).
 */
typedef struct { // config_t
	char *listenip;				//!< (user) IP to listen on, default "0.0.0.0"
	char *listenport;				//!< (user) port to listen on, default 1010
	
	char *infofile;				//!< (user) file to log info messages to, default none
	FILE *infofd;				//!< (foam) associated filepointer
	char *errfile;				//!< (user) file to log error messages to, default none
	FILE *errfd;				//!< (foam) associated filepointer
	char *debugfile;			//!< (user) file to log debug messages to, default none
	FILE *debugfd;				//!< (foam) associated filepointer
	
	bool use_syslog; 			//!< (user) syslog usage flag, default no
	char *syslog_prepend;		//!< (user) string to prepend to syslogs, default "foam"
	bool use_stdout; 			//!< (user) stdout usage flag, default no
	
	level_t loglevel;			//!< (user) level to log, default LOG_DEBUG
	
	pthread_t threads[MAX_THREADS]; //!< (foam) this stores the thread ids of all threads created
	int nthreads;				//!< (foam) stores the number of threads in use
} config_t;

/* 
 @brief This holds information on one particular client connection . Used by \c conntrack_t
 */
typedef struct {
	int fd; 						//!< FD for the client 
	int connid;						//!< ID used in conntrack_t
	struct bufferevent *buf_ev;		//!< The buffered event for the connected client
} client_t;

/* 
 @brief This keeps track of clients connected.
 
 Maximum amount of connections is defined by \c MAX_CLIENTS, in foam_cs_config.h, 
 which can be changed if necessary.
 */
typedef struct {
	int nconn;							//!< Amount of connections used
	client_t *connlist[MAX_CLIENTS];	//!< List of connected clients (max MAX_CLIENTS)
} conntrack_t;


#endif /* __TYPES_H__ */
