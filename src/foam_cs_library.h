/*! 
@file foam_cs_library.h
@brief This file is the main library for the CS component of @name
@author @authortim

(was cs_library.h)

This header file contains allmost all functions used by the CS component of @name.
In addition to that, it contains specific libraries only used by CS and not by the UI
as well as a lot of structs to hold data used in the CS. These include things like the 
state of the AO system (control_t), as well as some structs to track network connections
to the CS. For UI headers, see @uilib.\n
Last: 2008-01-21
*/

#ifndef FOAM_CS_LIBRARY
#define FOAM_CS_LIBRARY

// INCLUDES //
/************/
#include <fcntl.h>
#include <fitsio.h> // we need this to read FITS files
#include "foam_library.h"


// DEFINES //
/***********/

#define DEBUG_SLEEP 1 				// sleep time (sec) for loops in debug mode
#define FILENAMELEN 32				// maximum length for logfile names

#define FOAM_NAME "FOAM CS"			// some info about FOAM
#define FOAM_VERSION "v0.2 Dec"
#define FOAM_AUTHOR "Tim van Werkhoven"

#define MAX_CLIENTS 16				// maximum number of clients that can connect
 									// (allows for easy implementation of connection tracking)

// GLOBAL VARIABLES //
/********************/

char logmessage[LINE_MAX];

// STRUCTS AND TYPES //
/*********************/

/*!
@brief Helper enum for ao mode operation. Modes include AO_MODE_OPEN, AO_MODE_CLOSED and AO_MODE_CAL.
*/
typedef enum { // aomode_t
	AO_MODE_OPEN,
	AO_MODE_CLOSED,
	AO_MODE_CAL,
	AO_MODE_NONE
} aomode_t;

/*!
@brief Helper enum for ao scanning mode (i.e. in X and/or Y direction).
*/
typedef enum { // axes_t
	AO_AXES_XY,
	AO_AXES_X,
	AO_AXES_Y
} axes_t;

/*!
@brief Helper struct to store WFC variables in \a ptc. Used by type \c control_t.
*/
typedef struct { // wfc_t
	char name[FILENAMELEN];			//!< name for the specific WFC
	int nact;			//!< number of actuators in this WFC
	float *ctrl;		//!< pointer to array of controls for the WFS (i.e. voltages)
} wfc_t;

/*!
@brief Helper struct to store the WFS image(s). Used by type \c control_t.
*/
typedef struct { // wfs_t
	char name[FILENAMELEN];			//!< name of the specific WFS
	int res[2];			//!< x,y-resolution of this WFS
	int cells[2];		//!< number of cells in this WFS (SH only)
	int shsize[2];		//!< cells/res, resolution per cell (redundant, but easy)
	int (*subc)[2];		//!< this will hold the coordinates of each subapt
						// TODO: how to make a pointer to an array which holds pairs of ints as elements?
						// e.g. pointer to: { {x1,y1}, {x2,y2} ... {xn,yn}}
						// where ptr[i] = {xi,yi} ? GUUS
	int nsubap;			//!< amount of subapertures used (coordinates stored in subc)
	axes_t scandir; 	//!< scanning direction(s) used (see \c axes_t type)
	float *image;		//!< pointer to the WFS output, stored in row-major format
	float *darkim;		//!< darkfield (byte image), stored in row-major format \b per \b subapt
	float *flatim;		//!< flatfield (byte image), stored in row-major format \b per \b subapt
	float *corrim;		//!< corrected image, stored in row-major format \b per \b subapt
	float *refim;		//!< reference image for correlation tracking
	char darkfile[FILENAMELEN];		//!< filename for the darkfield calibration
	char flatfile[FILENAMELEN];		//!< filename for the flatfield calibration
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
	aomode_t mode;	//!< defines the mode the AO system is in (see \c aomode_t type)
	time_t starttime;	//!< stores the starting time of the system
	long frames;	//!< store the number of frames parsed
	
					// WFS variables
	int wfs_count;	//!< number of WFSs in the system
	wfs_t *wfs;		//!< pointer to a number of \c wfs_t structs
	
					// WFC variables
	int wfc_count;	//!< number of WFCs in the system
	wfc_t *wfc;		//!< pointer to a number of \c wfc_t structs
	
} control_t;

/*!
@brief Struct to configuration data in.

This struct stores things like the IP and port it should be listening on, the 
files to log error, info and debug messages to and whether or not to use
syslog.
*/
typedef struct { // config_t
	char listenip[16];	//!< IP to listen on, like 0.0.0.0
	int listenport;	//!< port to listen on, like 10000
	char infofile[FILENAMELEN]; //!< file to log info messages to
	FILE *infofd;	//!< associated filepointer
	char errfile[FILENAMELEN];
	FILE *errfd;
	char debugfile[FILENAMELEN];
	FILE *debugfd;
	bool use_syslog; //!< syslog usage flag
	char syslog_prepend[32]; //!< string to prepend to syslogs
	bool use_stderr; //!< stderr usage flag (for everything)
	level_t loglevel;
} config_t;

/* 
@brief This holds information on one particular connection to the CS. Used by conntrack_t
*/
typedef struct {
	int fd; 						// FD for client 
	int connid;						// ID used in conntrack_t
	struct bufferevent *buf_ev;		// The event for the connected client
} client_t;

/* 
@brief This counts and stores the number of connections to the CS. Maximum amount of connections
is defined by MAX_CLIENTS (hardcoded) 
*/
typedef struct {
	int nconn;						// Amount of connections used
	client_t *connlist[MAX_CLIENTS];	// List of connected clients (max 16)
} conntrack_t;


// PROTOTYPES //
/**************/

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

@param [in] msg The string to be passed on to vfprintf.
*/
void logInfo(const char *msg, ...);

/*!
@brief This directly prints something to the log files and/or screen without formatting

Works the same as logInfo() in the sense that it uses the same files/streams etc.

@param [in] msg The string to be passed on to vfprintf.
*/
void logDirect(const char *msg, ...);

/*!
@brief logErr() prints out error messages to the appropriate streams.

This function assumes there was an error as it checks errno and appends 
this to the error message. See documentation on logInfo() for more information.

@param [in] msg The string to be passed on to vfprintf.
*/
void logErr(const char *msg, ...);

/*!
@brief logDebug() prints out debug messages to the appropriate streams.

This function is used for debug logging. See documentation on logInfo() for more information.

@param [in] msg The string to be passed on to vfprintf.
*/
void logDebug(const char *msg, ...);


/*!
@brief Parse a \a var = \a value configuration pair stored in a config file.

@param [in] *var the name of the variable.
@param [in] *value the value of the variable.
@return \c EXIT_SUCCESS upon successful parsing of the configuration, \c EXIT_FAILURE otherwise.
*/
int parseConfig(char *var, char *value);

/*! 
@brief Runs the AO open-loop mode.

Runs the AO system in open loop mode, it reads out 
the sensors, calculates stuff and displays this to the user, but 
it does not control anything. Communcation is done via global variables,
in particular \a ptc (also see control_t).
*/
void modeOpen();

/*! 
@brief Runs the AO closed-loop mode.

This function runs the AO system in closed loop mode and drives all 
components that the user wants to use. Communication is again done via
global variables, in particular \a ptc (also see control_t).
*/
void modeClosed();

/*! 
@brief Listens to the user and decides what to do next.

This function runs continously every XXX microseconds and 
checks the global variables shared with the UI thread (\a ptc, see control_t)
and decides what to do next (e.g. what mode to run).
*/
void modeListen();

/*! 
@brief Runs the AO calibration-loop mode.

This calibrates some components of the AO system, and then returns to open loop mode.
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

@param [in] *msg the char array (max length 1024)
@param [in] len the actual length of msg
@param [in] *client the client_t struct of the client we want to parse a command from
@return \c EXIT_SUCCESS upon success, or \c EXIT_FAILURE otherwise.
*/
int parseCmd(char *msg, int len, client_t *client);

/*!
@brief This function is called if there is an error on the socket.
*/
void sockOnErr(struct bufferevent *bev, short event, void *arg);

/*!
@brief This function is called if there is data to be read on the socket.
*/
void sockOnRead(struct bufferevent *bev, void *arg);

/*!
@brief This function is called if the socket is ready for writing (placeholder)
*/
void sockOnWrite(struct bufferevent *bev, void *arg);
	
/*!
@brief Pop off a word from a space-seperated (" \t\n") string. Used to parse commands (see parseCmd()).
*/
int popword(char **msg, char *cmd);

/*! 
@brief Loads the config from \a file.

Loads the configuration (especially system characterisation,
like number of WFCs, number of WFSs, etc) from a file and
stores it in the global struct \a ptc.
@param [in] file the file to read the configuration from.
@return EXIT_SUCCESS if the load succeeds, EXIT_FAILURE otherwise.
*/
int loadConfig(char *file);

/*
@brief Open a stream to the appropriate files.

This function opens the error-, info- and debug-log files IF they
are defined in the global struct \a cs_config.
@return EXIT_SUCCESS if the load succeeds, EXIT_FAILURE otherwise.
*/
int initLogFiles();

/*!
@brief Save the configuration currently being used to \a file.

Save the configuration to a file such that it can be read by loadConfig().

@param [in] *file the file to store the data in
@return EXIT_SUCCESS on successful save, EXIT_FAILURE otherwise.
*/
int saveConfig(char *file);

/*!
@brief Give information on @name CS over the socket.

This function gives help to the cliet which sent a HELP command over the socket.

@param [in] *client the client that requested help
@param [in] *subhelp the helpcode requested by the client (i.e. what topic)
*/
int showHelp(const client_t *client, const char *subhelp);

/*!
@brief Function which wraps up  @name  (gives some stats)
*/
void stopFOAM();

/*!
@brief Catches \c SIGINT signals and decides what to do with it.
*/
void catchSIGINT();


#endif /* FOAM_CS_LIBRARY */
