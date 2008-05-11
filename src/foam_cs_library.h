/*! 
 @file foam_cs_library.h
 @brief This file is the main library for the CS component of @name
 @author @authortim
 
 This header file contains allmost all functions used by the CS component of @name.
 In addition to that, it contains specific libraries only used by CS and not by the UI
 as well as a lot of structs to hold data used in the CS. These include things like the 
 state of the AO system (\c control_t), as well as some structs to track network connections
 to the CS. For UI headers, see @uilib.\n
 Last: 2008-01-21
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

// !!!:tim:20080415 deprecated, basic configuration now done in foam_cs_librabry.c
// !!!:tim:20080415 and the rest is done in the primemodule header- & c-file
//#include "foam_cs_config.h"

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
 @brief Helper enum for ao mode operation.
 */
typedef enum { // aomode_t
	AO_MODE_OPEN,	//!< Open loop mode
	AO_MODE_CLOSED, //!< Closed loop mode
	AO_MODE_CAL, 	//!< Calibration mode (in conjunction with calmode_t)
	AO_MODE_LISTEN,	//!< Listen mode (idle)
	AO_MODE_SHUTDOWN //!< Set to this mode for the worker thread to finish
} aomode_t;

/*!
 @brief Helper enum for ao scanning mode
 */
typedef enum { // axes_t
	AO_AXES_XY,		//!< Scan in X and Y direction
	AO_AXES_X,		//!< Scan X direction only
	AO_AXES_Y		//!< Scan Y direction only
} axes_t;

/*!
 @brief Helper enum for filter wheel identification (user by control_t)
 
 This datatype must be used by the user to configure the AO system.
 To do anything useful, FOAM must know what filterwheels you are using,
 and therefore you must fill in the (user) fields at the beginning.
 
 See dummy prime module for details.
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
 @brief Helper struct to store WFC variables in \a ptc. Used by type \c control_t.
 
 This datatype must be used by the user to configure the AO system.
 To do anything useful, FOAM must know what WFSs and WFCs you are using,
 and therefore you must fill in the (user) fields at the beginning.
 See dummy prime module for details.
 */
typedef struct { // wfc_t
	char *name;					//!< (user) name for the specific WFC
	int nact;					//!< (user) number of actuators in this WFC
	gsl_vector_float *ctrl;		//!< (foam) pointer to array of controls for the WFC (i.e. `voltages')
	gain_t gain;				//!< (user) gain used in calculating the new controls
	wfctype_t type;				//!< (user) type of WFC we're dealing with (0 = TT, 1 = DM)
    int id;                     //!< (user) a unique ID to identify the actuator
} wfc_t;

/*!
 @brief Helper struct to store the WFS image(s). Used by type \c control_t.
 
 This datatype must be used by the user to configure the AO system.
 To do anything useful, FOAM must know what WFSs and WFCs you are using,
 and therefore you must fill in the (user) fields at the beginning.
 See dummy prime module for details.
 */
typedef struct { // wfs_t
	char *name;						//!< (user) name of the specific WFS
	coord_t res;					//!< (user) x,y-resolution of this WFS
	int bpp;						//!< (user) bits per pixel to use when reading the sensor (only 8 or 16 atm)
    
	void *image;					//!< (foam) pointer to the WFS output
	gsl_matrix_float *darkim;		//!< (foam) darkfield for the CCD 
	gsl_matrix_float *flatim;		//!< (foam) flatfield for the CCD (actually: flat-dark, as we never use flat directly)
	gsl_matrix_float *skyim;		//!< (foam) skyfield for the CCD
	gsl_matrix_float *corrim;		//!< (foam) corrected image to be processed
	
	// gsl might not be as fast as I wanted :<
	void *darkim2;					//!< (foam) store darkfield here
	void *flatim2;					//!< (foam) store flatim here
	void *corrim2;					//!< (foam) store corrim here
	
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
 which are shared between the different CS threads. The thread interfacing with user(s)
 can then read these variables and report them to the user, or change them to influence
 the CS behaviour.
 
 The struct is initialized with some default values at runtime (hardcoded), but
 should be configured by the user in the prime module c-file for useful operation.
 (user) fields can be configured by the user, (foam) fields should be left untouched
 
 The logfrac field is used to stop superfluous logging. See logInfo() and logDebug()
 documentation for details. Errors and warnings are always logged/displayed, as these
 shouldn't occur.
 
 Also take a look at wfs_t, wfc_t and filtwheel_t.
 */
typedef struct { // control_t
	aomode_t mode;		//!< (user) defines the mode the AO system is in, default AO_MODE_LISTEN
	calmode_t calmode;	//!< (user) defines the possible calibration modes, default CAL_PINHOLE
	time_t starttime;	//!< (foam) stores the starting time of the system
	time_t lasttime;	//!< (foam) use this to track the framerate
    
	long frames;		//!< (foam) store the number of frames parsed
	long capped;			//!< (foam) stores the number of frames captured earlier (i.e. what files already exist)
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
 @brief Helper enum for loglevel. Can be LOGNONE, LOGERR, LOGINFO or LOGDEBUG.
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
 @brief This holds information on one particular connection to the CS. Used by \c conntrack_t
 */
typedef struct {
	int fd; 						//!< FD for the client 
	int connid;						//!< ID used in conntrack_t
	struct bufferevent *buf_ev;		//!< The buffered event for the connected client
} client_t;

/* 
 @brief This keeps track of connections to the CS. 
 
 Maximum amount of connections is defined by \c MAX_CLIENTS, in foam_cs_config.h
 */
typedef struct {
	int nconn;							//!< Amount of connections used
	client_t *connlist[MAX_CLIENTS];	//!< List of connected clients (max MAX_CLIENTS)
} conntrack_t;


// PROTOTYPES //
/**************/

// THESE MUST BE DEFINED IN SOME MODULE!!!!

/*!
 @brief This routine is run at the very beginning of the @name program, after configuration has been read.
 
 modInitModule() is one of the cornerstones of the modular design of @name. This routine is not defined in
 the framework itself, but must be provided by the modules compiled with @name (i.e. in simulation). This `hook' provides
 a standardized means to initialize the module before anything has been done, like allocate memory, read in 
 some configuration files, start cameras or anything else. It is called without arguments and should return
 EXIT_SUCCESS or EXIT_FAILURE depening on success or not.
 */
int modInitModule(control_t *ptc, config_t *cs_config);

/*!
 @brief This routine is run at the very end of the @name program.
 
 modStopModule() can be used to wrap up things related to the module, like stop cameras, set
 filterwheels back or anything else. If this module fails, @name *will* exit anyway.
 */
void modStopModule(control_t *ptc);

/*! 
 @brief This routine is run during closed loop.
 
 modClosedLoop() should be provided by a module which does the necessary things in closed loop. Before this
 routine is called in a loop, modClosedInit() is first called once to initialize things related to closed loop
 operation.
 */
int modClosedLoop(control_t *ptc);

/*! 
 @brief This routine is run once before entering closed loop.
 
 modClosedInit() should be provided by a module which does the necessary things just before closed loop.
 */
int modClosedInit(control_t *ptc);

/*! 
 @brief This routine is run during open loop.
 
 modOpenLoop() should be provided by a module which does the necessary things in open loop.
 */
int modOpenLoop(control_t *ptc);

/*! 
 @brief This routine is run once before entering open loop.
 
 modOpenInit() should be provided by a module which does the necessary things just before open loop.
 */
int modOpenInit(control_t *ptc);

/*! 
 @brief This routine is run in calibration mode.
 
 Slightly different from open and closed mode is the calibration mode. This mode does not have
 a loop which runs forever, but only calls modCalibrate once. It is left to the programmer to decide 
 what to do in this mode. control_t provides a flag (.calmode) to distinguish between different calibration modes.
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
 @param [in] *ptc The global control struct
 @param [in] *client Information on the client that sent data
 @param [in] *list The list of words received over the network
 @param [in] count The amount of words received over the network
 */
int modMessage(control_t *ptc, const client_t *client, char *list[], const int count);

/*!
 @brief logInfo() prints out info messages to the appropriate streams.
 
 This function accepts a variable amount of arguments (like vfprintf) and passes
 them on to vfprintf more or less unchanged, except for the appended newline character
 and some prefix. This function returns immediately if the loglevel is too low to log
 info messages (see the level_t type for available levels). \n
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
 @brief Runs the AO open-loop mode.
 
 Runs the AO system in open loop mode, which is basically a skeleton which 
 calls modOpenInit() once, then enters a loop which runs as long as ptc.mode is 
 set to \c AO_MODE_OPEN and (see aomode_t) calls modOpenLoop() each run. Additionally,
 this function increments ptc.frames each loop.
 */
void modeOpen();

/*! 
 @brief Runs the AO closed-loop mode.
 
 Runs the AO system in closed loop mode, which is basically a skeleton which 
 calls modClosedInit() once, then enters a loop which runs as long as ptc.mode is 
 set to \c AO_MODE_CLOSED (see aomode_t) and calls modClosedLoop() each run.
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
 
 This skeleton simply calls modCalibrate() which must be defined in some module.
 */
void modeCal();

/*! 
 @brief Listens on a socket for connections
 
 Listen on a socket for connections by any client, for example 
 the UI also provided in this package. Uses the global \a ptc (see control_t)
 struct to provide data to the connected clients or to change
 the behaviour of the CS as dictated by the client.
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
 
 @param [in] sock Socket with pending connection
 @param [in] event The way this function was called
 @param [in] *arg Necessary additional argument to use as caller function for libevent
 */
void sockAccept(int sock, short event, void *arg);

/*!
 @brief Sets a socket to non-blocking mode.
 
 Taken from \c http://unx.ca/log/libevent_echosrv_bufferedc/
 */
int setnonblock(int fd);

/*! 
 @brief Process the command given by the user.
 
 This function is called if there is data on a socket. The data
 is then passed on to this function which interprets it and
 takes action if necessary. Currently this function can only
 read 1 kb in one time maximum (which should be enough).
 
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
 clients.
 */
void sockOnRead(struct bufferevent *bev, void *arg);

/*!
 @brief This function is called if the socket is ready for writing (placeholder).
 
 This function is necessary because passing a NULL pointer to bufferevent_new for the write
 function causes it to crash. Therefore this placeholder is used, which does exactly nothing.
 */
void sockOnWrite(struct bufferevent *bev, void *arg);

/*!
 @brief Open a stream to the appropriate logfiles, as defined in the configuration file.
 
 This function opens the error-, info- and debug-log files IF they
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
 
 If the files are not present, set the images to NULL so that we know we have 
 to calibrate lateron. 
 
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
 @brief Give information on @name CS over the socket.
 
 This function gives help to the cliet which sent a HELP command over the socket.
 
 @param [in] *client the client that requested help
 @param [in] *subhelp the helpcode requested by the client (i.e. what topic)
 @return EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
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
