/*! 
	@file foam_cs_config.h
	@author @authortim
	@date 2008-03-12 09:51

	@brief This is the user config file for @name

	Hardware-dependent variables can be defined here to be used in
	te rest of @name.
*/

// USER CONFIG BEGIN //
/*********************/
#ifndef FOAM_CS_CONFIG
#define FOAM_CS_CONFIG

// General configuration
#define FILENAMELEN 64				//!< maximum length for logfile names (no need to touch)
#define COMMANDLEN 1024				//!< maximum length for commands we read over the socket (no need to touch)

#define FOAM_NAME "FOAM CS"			//!< some info about FOAM
#define FOAM_VERSION "v0.4.0-130 Mar"	//!< FOAM version
#define FOAM_AUTHOR "Tim van Werkhoven" //!< FOAM author

#define MAX_CLIENTS 8				//!< maximum number of clients that can connect
 									// (allows for easy implementation of connection tracking)

#define MAX_THREADS 4				//!< number of threads besides the main thread that can be created (not really used)

#endif /* FOAM_CS_CONFIG */
