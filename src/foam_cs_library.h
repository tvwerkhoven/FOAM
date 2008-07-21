/*
 Copyright (C) 2008 Tim van Werkhoven (tvwerkhoven@xs4all.nl)
 
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
 */
/*! 
 @file foam_cs_library.h
 @brief This file is the main library for @name
 @author @authortim
 @date 2008-07-15 18:06
 
 This header file contains all functions used by the framework of @name.
 In addition to that, it contains a lot of structs to hold data used in 
 @name. These include things like the state of the AO system (\c control_t), 
 as well as some structs to track network connections to the CS.
 */

#ifndef FOAM_CS_LIBRARY
#define FOAM_CS_LIBRARY

// INCLUDES //
/************/

#include <string.h>
#include <unistd.h>
#ifndef FOAM_MODITIFG_ALONE
// We must not include sys/time.h if we're only building the itifg module
// This is a little ugly but somehow itifg decided to redefine timeval
#include <sys/time.h>
#endif
#ifndef _GNU_SOURCE				
#define _GNU_SOURCE				// for vasprintf / asprintf
#endif
#include <stdio.h>
#include <math.h>					// we need math
#include <sys/socket.h>				// networking
#include <arpa/inet.h>				// networking
#ifndef FOAM_MODITIFG_ALONE
// Same as above, itifg redefines u_int32_t
#include <sys/types.h>
#endif
#include <sys/errno.h>
#ifndef FOAM_MODITIFG_ALONE
// Same as above, itifg redefines u_int32_t
#include <stdlib.h>
#endif
#include <syslog.h> 				// used for syslogging
#include <stdarg.h>
#include <pthread.h> 				// threads
#include <stdbool.h> 				// true/false
#include <signal.h> 				// signal handlers
#include <time.h> 					// needed by libevent/event.h
typedef unsigned char u_char;
#include <event.h> 					// include AFTER stdlib.h (which defined u_char needed by event.h)
#include <fcntl.h>
#include <gsl/gsl_linalg.h> 		// this is for SVD / matrix datatype
#include <gsl/gsl_blas.h> 			// this is for SVD

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

// ERROR CHECKING //
/******************/

#ifndef FILENAMELEN
#error "FILENAMELEN undefined, please define in prime module header file"
#endif

#ifndef COMMANDLEN
#error "COMMANDLEN undefined, please define in prime module header file"
#endif

#ifndef MAX_CLIENTS
#error "MAX_CLIENTS undefined, please define in prime module header file"
#endif

#ifndef MAX_THREADS 
#error "MAX_THREADS undefined, please define in prime module header file"
#endif

#ifndef MAX_FILTERS
#error "MAX_FILTERS undefined, please define in prime module header file"
#endif

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
	DATA_FL,			//!< ID for float
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
	int listenport;				//!< (user) port to listen on, default 10000
	
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


// PROTOTYPES //
/**************/

// THESE MUST BE DEFINED IN SOME PRIME MODULE!!!!

/*!
 @brief This routine is run at the very beginning of the @name program, after configuration has been read.
 
 modInitModule() is one of the cornerstones of the modular design of @name. This routine is not defined in
 the framework itself, but must be provided by the prime modules compiled with @name. This `hook' provides
 a standardized means to initialize the module before anything has been done, like allocate memory, read in 
 some configuration files, start cameras or anything else.
 
 @param [in] *ptc A control_t struct that has been configured in a prime module
 @param [in] *cs_config A config_t struct that has been configured in a prime module
 @return EXIT_SUCCESS or EXIT_FAILURE depening on success or not.
 */
int modInitModule(control_t *ptc, config_t *cs_config);

/*!
 @brief This routine is run right after the program has split into two threads.
 
 This routine can be used to initialize things that are not thread safe,
 such as OpenGL. See modInitModule for more details.
 
 @param [in] *ptc A control_t struct that has been configured in a prime module
 @param [in] *cs_config A config_t struct that has been configured in a prime module
 @return EXIT_SUCCESS or EXIT_FAILURE depening on success or not. 
 */
int modPostInitModule(control_t *ptc, config_t *cs_config);

/*!
 @brief This routine is run at the very end of the @name program.
 
 modStopModule() can be used to wrap up things related to the module, like stop cameras, set
 filterwheels back or anything else. If this module fails, @name *will* exit anyway.
 
 @param [in] *ptc A control_t struct that has been configured in a prime module
 */
void modStopModule(control_t *ptc);

/*! 
 @brief This routine is run during closed loop.
 
 modClosedLoop() should be provided by a module which does the necessary things in closed loop. Before this
 routine is called in a loop, modClosedInit() is first called once to initialize things related to closed loop
 operation.
 
 @param [in] *ptc A control_t struct that has been configured in a prime module
 @return EXIT_SUCCESS or EXIT_FAILURE depening on success or not. 
 */
int modClosedLoop(control_t *ptc);

/*! 
 @brief This routine is run once before entering closed loop.
 
 modClosedInit() should be provided by a module which does the necessary things just before closed loop.
 @param [in] *ptc A control_t struct that has been configured in a prime module 
 @return EXIT_SUCCESS or EXIT_FAILURE depening on success or not. 
 */
int modClosedInit(control_t *ptc);

/*! 
 @brief This routine is run after closed loop.
 
 modClosedFinish() can be used to shut down camera's temporarily, i.e.
 to stop grabbing frames or something similar.
 @param [in] *ptc A control_t struct that has been configured in a prime module 
 @return EXIT_SUCCESS or EXIT_FAILURE depening on success or not. 
 */
int modClosedFinish(control_t *ptc);

/*! 
 @brief This routine is run during open loop.
 
 modOpenLoop() should be provided by a module which does the necessary things in open loop.
 @param [in] *ptc A control_t struct that has been configured in a prime module 
 @return EXIT_SUCCESS or EXIT_FAILURE depening on success or not. 
 */
int modOpenLoop(control_t *ptc);

/*! 
 @brief This routine is run once before entering open loop.
 
 modOpenInit() should be provided by a module which does the necessary things just before open loop.
 @param [in] *ptc A control_t struct that has been configured in a prime module 
 @return EXIT_SUCCESS or EXIT_FAILURE depening on success or not.  
 */
int modOpenInit(control_t *ptc);

/*! 
 @brief This routine is run after open loop.
 
 modOpenFinish() can be used to shut down camera's temporarily, i.e.
 to stop grabbing frames or something similar.
 
 @param [in] *ptc A control_t struct that has been configured in a prime module 
 @return EXIT_SUCCESS or EXIT_FAILURE depening on success or not. 
 */
int modOpenFinish(control_t *ptc);

/*! 
 @brief This routine is run in calibration mode.
 
 Slightly different from open and closed mode is the calibration mode. This mode does not have
 a loop which runs forever, but only calls modCalibrate once. It is left to the programmer to decide 
 what to do in this mode. control_t provides a flag (.calmode) to distinguish between different calibration modes.
 
 @param [in] *ptc A control_t struct that has been configured in a prime module 
 @return EXIT_SUCCESS or EXIT_FAILURE depening on success or not.  
 */
int modCalibrate(control_t *ptc);

/*!
 @brief This routine is run if a client sends a message over the socket
 
 If the networking thread of the control software receives data, it will 
 split the string received in space-seperate words. After that, the
 routine will handle some default commands itself, like 'help', 'quit',
 'shutdown' and others (see parseCmd()). If a command is not recognized, it 
 is passed to this routine, which must then do something, or return 0 to
 indicate an unknown command. parseCmd() itself will then warn the user.\n
 \n
 Besides parsing commands, this routine must also provide help informtion
 about which commands are available in the first place. FOAM itself
 already provides some help on the basic functions, and after sending this
 to the user, modMessage is called with list[0] == 'help' such that this 
 routine can add its own help info.\n
 \n
 If a client sends 'help <topic>', this is also passed to modMessage(),
 with list[0] == help and list[1] == <topic>. It should then give
 information on that topic and return 1, or return 0 if it does not
 'know' that topic.\n
 \n
 For an example of modMessage(), see foam_primemod-dummy.c.
 @param [in] *ptc A control_t struct that has been configured in a prime module 
 @param [in] *client Information on the client that sent data
 @param [in] *list The list of words received over the network
 @param [in] count The amount of words received over the network
 */
int modMessage(control_t *ptc, const client_t *client, char *list[], const int count);

// FOAM (LIBRARY) ROUTINES BEGIN HERE //
/**************************************/

/*!
 @brief This is the routine that is run immediately after threading, and should run modeListen after initializing the prime module.

 */
void startThread();

/*!
 @brief logInfo() prints out info messages to the appropriate streams.
 
 This function accepts a variable amount of arguments (like vfprintf) and passes
 them on to vfprintf more or less unchanged, except for the appended newline character
 and some prefix. This function returns immediately if the loglevel is too low to log
 info messages (see the level_t type for available levels).
 
 The parameter 'flag' can be set to a XOR of LOG_SOMETIMES and LOG_NOFORMAT.
 The first option makes sure that logging only happens every \a config.logfrac
 frames, which can be useful if logging during adaptive optics operations when
 logging too much can cause performance problems. LOG_NOFORMAT can be used to
 specify that logging should be done without formatting anything (i.e. no prefix
 nor a newline at the end of the log message).
 
 logInfo() first attempts to write to the file descriptor provided by \a cs_config.infofd
 if this is not \c NULL. After that the boolean variable \a cs_config.use_stderr
 is checked to see if the user requested output to stderr. Finally, 
 \a cs_config.use_syslog is checked to see if output to syslog is desired.
 This function does not report any problems at all.
 
 @param [in] flag Some options on how to log the data
 @param [in] msg The string to be passed on to vfprintf.
 */
void logInfo(const int flag, const char *msg, ...);

/*!
 @brief logErr() prints out error messages to the appropriate streams, and exits.
 
 This function assumes there was an error as it checks errno and appends 
 this to the error message. This function should be used for fatal errors, because
 this function exits after printing. See documentation on logInfo() for more information.
 
 @param [in] msg The string to be passed on to vfprintf.
 */
void logErr(const char *msg, ...);

/*!
 @brief logWarn() prints out error messages to the appropriate streams.
 
 This function assumes there was an error as it checks errno and appends 
 this to the error message. This function does not exit, and should be used
 for non-fatal errors. See documentation on logInfo() for more information.
 
 @param [in] msg The string to be passed on to vfprintf.
 */
void logWarn(const char *msg, ...);

/*!
 @brief logDebug() prints out debug messages to the appropriate streams.
 
 This function is used for debug logging. See documentation on logInfo() for more information.
 
 @param [in] flag Some options on how to log the data
 @param [in] msg The string to be passed on to vfprintf.
 */
void logDebug(const int flag, const char *msg, ...);

/*! 
 @brief Runs @name in open-loop mode.
 
 Runs the AO system in open loop mode, which is basically a skeleton which 
 calls modOpenInit() once, then enters a loop which runs as long as ptc.mode is 
 set to \c AO_MODE_OPEN and (see aomode_t) calls modOpenLoop() each run. Additionally,
 this function increments ptc.frames each loop. If ptc.mode is unequal to 
 \c AO_MODE_OPEN, this function calls modOpenFinish() and returns to modeListen().
 */
void modeOpen();

/*! 
 @brief Runs the AO closed-loop mode.
 
 Runs the AO system in closed loop mode, which does the same as modeOpen() except
 it calls modClosedInit() first, then modClosedLoop() and finally 
 modClosedFinish(). See modeOpen() for details.
 */
void modeClosed();

/*! 
 @brief Listens to the user and decides what to do next.
 
 This function is called when the system is not in calibration, open loop or
 closed loop-mode. If so, it locks using a pthread condition and waits until
 the networking thread gets a command to do something. When a command is sent
 over the network, the networking thread notices the worker thread which
 is running modeListen() and checks if something needs to be done.
 */
void modeListen();

/*! 
 @brief Runs the AO calibration-loop mode.
 
 This skeleton simply calls modCalibrate() which must be defined in a 
 prime module.
 */
void modeCal();

/*! 
 @brief Listens on a socket for connections
 
 Listen on a socket for connections by any client, for example telnet. Uses 
 the global \a ptc (see control_t) struct to provide data to the connected 
 clients or to change the behaviour of @name as dictated by the client.
 
 This function initializes the listening socket, and calls sockAccept() when
 a client connects to it. This event handling is done by libevent, which also
 multiplexes the other socket interactions.
 
 @return \c EXIT_SUCCESS if it ran succesfully, \c EXIT_FAILURE otherwise.
 */
int sockListen();

/*! 
 @brief Accept new client connection.
 
 Accept a new connection pending on \a sock and add the
 socket to the set of active sockets (see conntrack_t and client_t).
 This function is called if there is an event on the main socket, which means 
 someone wants to connect. It spawns a new bufferent event which keeps an eye 
 on activity on the new socket, in which case there is data to be read. 
 
 The event handling for this socket (such as processing incoming user data or
 sending data back to the client) is handled by libevent.
 
 @param [in] sock Socket with pending connection
 @param [in] event The way this function was called
 @param [in] *arg Necessary additional argument to use as caller function for libevent
 */
void sockAccept(int sock, short event, void *arg);

/*!
 @brief Sets a socket to non-blocking mode.
 
 Taken from \c http://unx.ca/log/libevent_echosrv_bufferedc/
 
 @return \c EXIT_SUCCESS if it ran succesfully, \c EXIT_FAILURE otherwise. 
 */
int setNonBlock(int fd);

/*! 
 @brief Process the command given by the user.
 
 This function is called by sockOnRead if there is data on a socket. The data
 is then passed on to this function which interprets it and
 takes action if necessary. Currently this function can only
 read 1 kb in one time maximum (which should be enough).
 
 'msg' contains the string received from the client, which is passed to
 explode() which chops it up in several words.
 
 The first word in 'msg' is compared against a few commands known by parseCmd()
 and if one is found, apropriate actions are taken if necessary. If the command
 is unknown, it is passed to modMessage(), which should be provided by the prime
 module. This way, the commands @name accepts can be expanded by the user.
 
 One special case is the 'help' command. If simply 'help' is given with nothing
 after that, parseCmd() calls showHelp() to display general usage information
 for the @name framework. After showing this information, modMessage() is also
 called, such that specific usage information for the prime module can also 
 be dispalyed.
 
 If 'help' is followed by another word which is the topic a user requests
 help on, it is passed to showHelp() and modMessage() as well. If these routines
 both return 0, the topic is unknown and an error is given to the user.
  
 @param [in] *msg the char array (max length \c COMMANDLEN)
 @param [in] len the actual length of msg
 @param [in] *client the client_t struct of the client we want to parse a command from
 @return \c EXIT_SUCCESS upon success, or \c EXIT_FAILURE otherwise.
 */
int parseCmd(char *msg, int len, client_t *client);

/*!
 @brief This function is called if there is an error on the socket.
 
 An error on the socket can mean that the client simply wants to disconnect,
 in which cas \c event is equal to \c EVBUFFER_EOF. If so, this is not a problem.
 If \c event is something different, a real error occurred. In both cases, the
 bufferevent is freed, the socket is closed, and the client struct is freed, in that order.
 */
void sockOnErr(struct bufferevent *bev, short event, void *arg);

/*!
 @brief This function is called if there is data to be read on the socket.
 
 Data waiting to be read usually means there is a command coming from one of the connected
 clients. This routine reads the data, checking if it does not exceed \c COMMANDLEN,
 and then passes the result on to parseCmd().
 */
void sockOnRead(struct bufferevent *bev, void *arg);

/*!
 @brief This function is called if the socket is ready for writing.
 
 This function is necessary because passing a NULL pointer to bufferevent_new for the write
 function causes it to crash. Therefore this placeholder is used, which does exactly nothing.
 */
void sockOnWrite(struct bufferevent *bev, void *arg);

/*!
 @brief Open a stream to the appropriate logfiles, as defined in the configuration file.
 
 This function opens the error-, info- and debug-log files *if* they
 are defined in the global struct cs_config (see config_t). If some filenames are equal, the
 logging is linked. This can be useful if users want to log everything to the same
 file. No filename means that no stream will be opened.
 @return EXIT_SUCCESS if the load succeeds, EXIT_FAILURE otherwise.
 */
int initLogFiles();

/*!
 @brief Check & initialize darkfield, flatfield and skyfield files.
 
 Check if the darkfield, flatfield and skyfield files are available, and if so,
 allocate memory for the various images in memory and load them into the
 newly allocated matrices. 
 
 If the files are not present, the FD's will be NULL. Allocate memory anyway in that
 case so we can directly store the frames in the specific memory later on.
 
 If the files are set to NULL, don't use dark/flat/sky field calibration at all.
 */
void checkFieldFiles(wfs_t *wfsinfo);

/*!
 @brief Check if the ptc struct has reasonable values
 
 Check if the values in ptc set by modInitModule() by the user are reasonable.
 If they are not reasonable, give a warning about it. If necessary, allocate
 some memory.
 */
void checkAOConfig(control_t *ptc);

/*!
 @brief Check if the cs_config struct has reasonable values
 
 Check if the values in cs_config set by modInitModule() by the user are reasonable.
 If they are not reasonable, give a warning about it. If necessary, allocate
 some memory.
 */
void checkFOAMConfig(config_t *conf);

/*!
 @brief Give usage information on @name to a client.
 
 This function gives help to the cliet which sent a HELP command over the socket.
 Also look at parseCmd() function.
 
 @param [in] *client the client that requested help
 @param [in] *subhelp the helpcode requested by the client (i.e. what topic)
 @return 1 everything went ok, or 0 if the user requested an unknown topic.
 */
int showHelp(const client_t *client, const char *subhelp);

/*!
 @brief Function which wraps up @name (free some memory, gives some stats)
 
 This routine does the following:
 \li Print info to user that @name is stopping using logInfo()
 \li Run modStopModule() such that modules can stop what they're doing (shutdown cameras and such)
 \li Destroy the mutexes for the threads
 \li Close files for info/error/debug logging.
 */
void stopFOAM();

/*!
 @brief Catches \c SIGINT signals and stops @name gracefully, calling \c stopFOAM().
 */
void catchSIGINT();

/*!
 @brief This sends a message 'msg' to all connected clients.
 
 @param [in] *msg The message to send to the clients
 @return EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
 */
int tellClients(char *msg, ...);

/*!
 @brief This sends a message 'msg' to a specific client.
 
 @param [in] *bufev The bufferevent associated with a client
 @param [in] *msg The message to send to the clients
 @return EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
 */
int tellClient(struct bufferevent *bufev, char *msg, ...);


#endif /* FOAM_CS_LIBRARY */
