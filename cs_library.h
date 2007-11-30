/*
	Control Software Library header file
*/
#ifndef CS_LIBRARY
#define CS_LIBRARY

// INCLUDES //
/************/

#include "ao_library.h"


// DEFINES //
/***********/

#define DEBUG_SLEEP 1000000 		// usleep time for debugmode (typically about 1s = 1000000 usec)
#define FILENAMELEN 32				// maximum length for logfile names

#define FOAM_NAME "FOAM"
#define FOAM_VERSION "v0.1 Nov"
#define FOAM_AUTHOR "Tim van Werkhoven"


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
@brief logErr() prints out error messages to the appropriate streams.

This function assumes there was an error as it checks errno and appends 
this to the error message. See documentation on logInfo() for more information.

@param [in] msg The string to be passed on to vfprintf.
*/
void logErr(const char *msg, ...);

/*!
@brief logDebug() prints out debug messages to the appropriate streams.

See logInfo() for more details.

@param [in] msg The string to be passed on to vfprintf.
*/
void logDebug(const char *msg, ...);


/*!
@brief Parse a \a var = \a value configuration pair.

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
in particular \a ptc.
*/
void modeOpen();

/*! 
@brief Runs the AO closed-loop mode.

This function runs the AO system in closed loop mode and drives all 
components that the user wants to use. Communication again done via
global variables, in particular \a ptc.
*/
void modeClosed();

/*! 
@brief Listens to the user and decides what to do next.

This function runs continously every XXX microseconds and 
checks the global variables shared with the UI thread (\a ptc)
and decides what to do next (e.g. what mode to run).
*/
void modeListen();

/*! 
@brief Runs the AO calibration-loop mode.

This calibrates some components of the AO system.
*/
void modeCal();

/*! 
@brief Listens on a socket for connections

Listen on a socket for connections by any client, for example 
the UI also provided in this package. Uses the global \a ptc
struct to provide data to the connected clients or to change
the behaviour of the CS as dictated by the client.
@return \c EXIT_SUCCESS if it ran succesfully, \c EXIT_FAILURE otherwise.
*/
int sockListen();

/*! 
@brief Accept new client connection.

Accept a new connection pending on \a sock and add the
socket to the set of active sockets
@param [in] sock Socket with pending connection
@param [in,out] *lfd_set A pointer to the set of FD's to add the sockets to
@return Socket descriptor if succesfull, \c EXIT_FAILURE otherwise.
*/
int sockAccept(int sock, fd_set *lfd_set);

/*! 
@brief Initializes a listening TCP socket.

Creates a TCP streaming socket to listen on. Use \c cs_config.listenip
and \c cs_config.listenport for IP:port combination to listen on.
@param [out] *lfd_set A pointer to the set of FD's to insert the socket in
@return Socket descriptor if succefull, \c EXIT_FAILURE otherwise.
*/
int initSockL(fd_set *lfd_set);

/*! 
@brief Get socket id which needs I/O.

Loop over all possible sockets and see which needs attention
@param [in] *lfd_set A pointer to the set of FD's to scan
@return Socket descriptor if succesfull, \c EXIT_FAILURE otherwise.
*/
int sockGetActive(fd_set *lfd_set);

/*! 
@brief Process the command given by the user.

@param [in] *msg the char array (max length 1024)
@param [in] len the actual length of msg
@return \c EXIT_SUCCESS upon success, or \c EXIT_FAILURE otherwise.
*/
int parseCmd(char *msg, int len, int asock, fd_set *lfd_set);

/*!
@brief Pop off a word from a space-seperated (" \t\n") string.
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
*/
int initLogFiles();

/*!
@brief Save the configuration currently being used to \a file.

TODO: not complete yet.

@param [in] *file the file to store the data in
@return EXIT_SUCCESS on successful save, EXIT_FAILURE otherwise.
*/
int saveConfig(char *file);

/*!
@brief Give information on FOAM CS over the socket.
*/
int showHelp(int sock, char *subhelp);

int sendMsg(int sock, char *buf);

// STRUCTS AND TYPES //
/*********************/

/*!
@brief Helper struct to store WFC variables in \a ptc. Used by type \c control_t.
*/
typedef struct { // wfc_t
	char *name;			//!< name for the specific WFC
	int nact;			//!< number of actuators in this WFC
	double *ctrl;		//!< pointer to array of controls for the WFS
} wfc_t;

/*!
@brief Helper struct to store the WFS image(s). Used by type \c control_t.
*/
typedef struct { // wfs_t
	char *name;			//!< name of the specific WFS
	int resx;			//!< x-resolution of this WFS
	int resy;			//!< y-resolution of this WFS
	int cellsx;			//!< number of x-cells in this WFS (SH only)
	int cellsy;			//!< number of y-cells in this WFS (SH only)
	char *image;		//!< pointer to the WFS output
	char *dark;			//!< darkfield (byte image)
	char *flat;			//!< flatfield (byte image)
	char *darkfile;		//!< filename for the darkfield calibration
	char *flatfile;		//!< filename for the flatfield calibration

} wfs_t;

/*!
@brief Helper enum for ao mode operation.
*/
typedef enum { // aomode_t
	AO_MODE_OPEN,
	AO_MODE_CLOSED,
	AO_MODE_CAL
} aomode_t;

/*! 
@brief Stores the state of the AO system

This struct is used to store several variables indicating the state of the AO system 
which are shared between the different CS threads. The thread interfacing with user(s)
can then read these variables and report them to the user, or change them to influence
the CS behaviour.\n
Parts of it are read at initialisation
\n
This struct is globally available. 
*/
typedef struct { // control_t
	aomode_t mode;	//!< defines the mode the AO system is in (see \c AO_MODE_* definitions)
	
					// WFS variables
	int wfs_count;	//!< number of WFSs
	wfs_t *wfs;		//!< pointer to a number of \c wfs_t structs
	
					// WFC variables
	int wfc_count;	//!< number of WFCs
	wfc_t *wfc;		//!< pointer to a number of \c wfc_t structs
	
} control_t;

/*!
@brief Struct to store the listening socket information in.
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

// GLOBAL VARIABLES //
/********************/

#endif /* CS_LIBRARY */
