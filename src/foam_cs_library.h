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
#include <sys/time.h>
#include <stdio.h>
#include <math.h>					// we need math
#include <sys/socket.h>				// networking
#include <arpa/inet.h>				// networking
#include <sys/types.h>
#include <sys/errno.h>
#include <stdlib.h>
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

#include "foam_cs_config.h"

//#include <sys/select.h> //?
//#include <limits.h> 				// LINE_MAX
//#include <sys/uio.h> //?
//#include <inttypes.h> //?
//#include <xlocale.h> //?


// GLOBAL VARIABLES //
/********************/

char logmessage[COMMANDLEN];
#define LOG_SOMETIMES 1
#define LOG_NOFORMAT 2

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
@brief Helper enum for ao mode operation.
*/
typedef enum { // aomode_t
	AO_MODE_OPEN,	//!< Open loop mode
	AO_MODE_CLOSED, //!< Closed loop mode
	AO_MODE_CAL, 	//!< Calibration mode (in conjunction with calmode_t)
	AO_MODE_LISTEN	//!< Listen mode (idle)
} aomode_t;

/*!
@brief Helper enum for filter wheel identification
*/
typedef enum {
	FILT_PINHOLE,	//!< Pinhole filter in place, used for calibration
	FILT_NORMAL		//!< Normal' filter in place
} fwheel_t;

/*!
@brief Helper enum for ao calibration mode operation.
*/
typedef enum { // calmode_t
	CAL_PINHOLE,	//!< determine reference shifts after inserting a pinhole
	CAL_INFL,		//!< determine the influence functions for each WFS-WFC pair
	CAL_LINTEST		//!< linearity test for WFCs
} calmode_t;

/*!
@brief Helper enum for ao scanning mode
*/
typedef enum { // axes_t
	AO_AXES_XY,		//!< Scan in X and Y direction
	AO_AXES_X,		//!< Scan X direction only
	AO_AXES_Y		//!< Scan Y direction only
} axes_t;

/*!
@brief Helper enum for WFC types
*/
typedef enum { // axes_t
	WFC_TT=0,		//!< WFC Type for tip-tilt mirrors
	WFC_DM=1		//!< WFC type for deformable mirrors
} wfctype_t;

/*!
@brief Helper struct to store WFC variables in \a ptc. Used by type \c control_t.
*/
typedef struct { // wfc_t
	char name[FILENAMELEN];		//!< name for the specific WFC
	int nact;					//!< number of actuators in this WFC
	gsl_vector_float *ctrl;		//!< pointer to array of controls for the WFC (i.e. `voltages')
	float gain;					//!< gain used in calculating the new controls
	wfctype_t type;				//!< type of WFC we're dealing with (0 = TT, 1 = DM)
} wfc_t;

/*!
@brief Helper struct to store the WFS image(s). Used by type \c control_t.
*/
typedef struct { // wfs_t
	char name[FILENAMELEN];			//!< name of the specific WFS
	coord_t res;					//!< x,y-resolution of this WFS

	float *image;					//!< pointer to the WFS output, stored in row-major format
	float *darkim;					//!< darkfield (byte image), stored in row-major format \b per \b subapt
	float *flatim;					//!< flatfield (byte image), stored in row-major format \b per \b subapt
	gsl_matrix_float *corrim;		//!< corrected image, stored in row-major format \b per \b subapt
	
	char darkfile[FILENAMELEN];		//!< filename for the darkfield calibration
	char flatfile[FILENAMELEN];		//!< filename for the flatfield calibration
	char skyfile[FILENAMELEN];		//!< filename for the flatfield calibration

	axes_t scandir; 				//!< scanning direction(s) used (see axes_t type)
	
	// TODO: the below members SHOULD be exported to some different structure (these are SH specific)	
	
	float *refim;					//!< reference image for correlation tracking (unused now)
	
	gsl_vector_float *singular;		//!< stores singular values from SVD (nact big)
	gsl_matrix_float *dmmodes;		//!< stores dmmodes from SVD (nact*nact big)
	gsl_matrix_float *wfsmodes;		//!< stores wfsmodes from SVD (nact*nsubap*2 big)
	
	int cells[2];					//!< number of cells in this WFS (SH only)
	int shsize[2];					//!< cells/res, resolution per cell (redundant, but easy -> phase this out)
	coord_t track;					//!< tracker window resolution in pixels (i.e. 1/2 of shsize)
	
	int (*subc)[2];					//!< this will hold the coordinates of each subapt
	int (*gridc)[2];				//!< this will hold the grid origina for a certain subaperture
//	float (*refc)[2];				//!< reference displacements
//	float (*disp)[2];				//!< measured displacements (compare with refence for actual shift)
	gsl_vector_float *refc;			//!< reference displacements
	gsl_vector_float *disp;			//!< measured displacements (compare with refence for actual shift)
	fcoord_t stepc;					//!< add this to the reference displacement during correction
	
	char pinhole[FILENAMELEN];		//!< filename to store the pinhole calibration (in *(refc))
	char influence[FILENAMELEN];	//!< filename to store the influence matrix
	
	int nsubap;						//!< amount of subapertures used (coordinates stored in subc)
	
} wfs_t;



/*! 
@brief Stores the state of the AO system

This struct is used to store several variables indicating the state of the AO system 
which are shared between the different CS threads. The thread interfacing with user(s)
can then read these variables and report them to the user, or change them to influence
the CS behaviour.\n
Parts of it are read at initialisation from some configuration file, other parts are
hardcoded and yet others are assigned dynamically.
\n
This struct is globally available.
*/
typedef struct { // control_t
	aomode_t mode;		//!< defines the mode the AO system is in (see aomode_t type)
	calmode_t calmode;	//!< defines the possible calibration modes (see calmode_t type)
	time_t starttime;	//!< stores the starting time of the system
	long frames;		//!< store the number of frames parsed
	
						// WFS variables
	int wfs_count;		//!< number of WFSs in the system
	wfs_t *wfs;			//!< pointer to a number of wfs_t structs
	
						// WFC variables
	int wfc_count;		//!< number of WFCs in the system
	wfc_t *wfc;			//!< pointer to a number of wfc_t structs
	
	fwheel_t filter;	//!< stores the filterwheel currently in place
	
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
syslog.
*/
typedef struct { // config_t
	char listenip[16];			//!< IP to listen on, like "0.0.0.0"
	int listenport;				//!< port to listen on, like 10000
	char infofile[FILENAMELEN]; //!< file to log info messages to
	FILE *infofd;				//!< associated filepointer
	char errfile[FILENAMELEN];	//!< file to log error messages to
	FILE *errfd;				//!< associated filepointer
	char debugfile[FILENAMELEN];	//!< file to log debug messages to
	FILE *debugfd;				//!< associated filepointer
	bool use_syslog; 			//!< syslog usage flag
	char syslog_prepend[32]; 	//!< string to prepend to syslogs
	bool use_stdout; 			//!< stdout usage flag (do we want to log to stdout/stderr or not?)
	level_t loglevel;			//!< level to log (see \c level_t)
	int logfrac;				//!< fraction to log for info and debug (1 is always, 50 is 1/50 times)
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
int modInitModule(control_t *ptc);

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
@brief Parse a \a var = \a value configuration pair stored in a config file.

@param [in] *var the name of the variable.
@param [in] *value the value of the variable.
@return \c EXIT_SUCCESS upon successful parsing of the configuration, \c EXIT_FAILURE otherwise.
*/
int parseConfig(char *var, char *value);

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
@brief Pop off a word from a space-seperated (" \t\n") string. Used to parse commands (see parseCmd()).
*/
int popword(char **msg, char *cmd);

/*! 
@brief Loads the config from \a file.

Loads the configuration (especially system characterisation, like number of WFCs, 
number of WFSs, etc) from a file and stores it in the global struct \a ptc.
@param [in] file the filename to read the configuration from.
@return \c EXIT_SUCCESS if the load succeeds, \c EXIT_FAILURE otherwise.
*/
int loadConfig(char *file);

/*
@brief Open a stream to the appropriate logfiles, as defined in the configuration file.

This function opens the error-, info- and debug-log files IF they
are defined in the global struct cs_config (see config_t). If some filenames are equal, the
logging is linked. This can be useful if users want to log everything to the same
file. No filename means that no stream will be opened.
@return EXIT_SUCCESS if the load succeeds, EXIT_FAILURE otherwise.
*/
int initLogFiles();

/*!
@brief Save the configuration currently being used to \a file.

Save the configuration to a file such that it can be read by loadConfig().
Currently this function is unfinished.

@param [in] *file the file to store the data in
@return EXIT_SUCCESS on successful save, EXIT_FAILURE otherwise.
*/
int saveConfig(char *file);

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
@brief Checks whether ptc.wfc is already allocated, displays a warning otherwise
@return EXIT_SUCCESS if ptc.wfc is already allocated, EXIT_FAILURE otherwise
*/
int issetWFC(char *var);

/*!
@brief Checks whether ptc.wfs is already allocated, displays a warning otherwise
@return EXIT_SUCCESS if ptc.wfs is already allocated, EXIT_FAILURE otherwise
*/
int issetWFS(char *var);

/*!
@brief Checks whether the var=value pair in \c var is a valid WFC.

This function takes a variable as input in the form of var = value, taken
from the configuration file, and checks whether the variable is for a
valid WFC. I.e.: WFC_NAME[1] = 'anyname' is invalid if WFC_COUNT is only 1,
and then this function would warn the user.
@return EXIT_SUCCESS if \c var holds a valid WFC configuration, EXIT_FAILURE otherwise
*/
int validWFC(char *var);

/*!
@brief Checks whether the var=value pair in \c var is a valid WFS. See validWFC();
@return EXIT_SUCCESS if \c var holds a valid WFS configuration, EXIT_FAILURE otherwise
*/
int validWFS(char *var);

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


#endif /* FOAM_CS_LIBRARY */
