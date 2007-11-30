/*! 
	@file proto_ao_cs.c
	@author Tim van Werkhoven
	@date November 14 2007

	@brief This is the main file for the FOAM Control Software.

	Prototyped / pseudocoded FOAM Control Software\n\n
	TODO tags are used for work in progress or things that are unclear.
*/

// HEADERS //
/***********/

#include "cs_library.h"
#include "foam_modules.h"


// GLOBAL VARIABLES //
/********************/	

// These are defined in cs_library.c
extern control_t ptc;
extern config_t cs_config;

// PROTOTYPES //
/**************/

char printUTC() {
	struct tm *utc;
	char timestr[26];
	time_t t;
	t = time(NULL);
	utc = gmtime(&t);
//	timestr = asctime(utc);
//	timestr[24] = '\0';
	return *timestr;
}

char printLocaltime() {
	char timestr[26];
	time_t t;
	t = time(NULL);
	strcpy(timestr, ctime(&t));
	timestr[24] = '\0';
	return *timestr;
}

	/*! 
	@brief Initialisation function.
	
	\c main() initializes necessary variables, threads, etc. and
	then runs the AO in open-loop mode, from where the user can decide
	what to do.
	@return \c EXIT_FAILURE on failure, \c EXIT_SUCESS on successful completion.
	*/
int main () {
	pthread_t thread;
	logInfo("Starting %s (%s) by %s",FOAM_NAME, FOAM_VERSION, FOAM_AUTHOR);
	
	ptc.wfs = malloc(ptc.wfs_count * sizeof(ptc.wfs));	// allocate memory
	ptc.wfs[0].resx = 192;
	ptc.wfs[0].resy = 192;

	if (loadConfig("ao_config.cfg") != EXIT_SUCCESS) {
		logErr("Loading configuration failed, aborting");
		exit(EXIT_FAILURE);
	}

	logInfo("Configuration successfully loaded...");	
	
	// Create thread which listens to clients on a socket	
	
	if ((pthread_create(&thread,
		NULL,
		(void *) sockListen, // we don't need (void *) here? either ISO C error or GCC error :(
		NULL)
		) != 0)
		//TODO: rework I/O (where do we print errors?)
		perror("Error in socket create: ");

	modeListen(); 			// After initialization, start in open mode

	return EXIT_SUCCESS;
}

int parseConfig(char *var, char *value) {
	int tmp;

	if (strcmp(var, "WFS_COUNT") == 0) {
		ptc.wfs_count = (int) strtol(value, NULL, 10);
		ptc.wfs = malloc(ptc.wfs_count * sizeof(ptc.wfs));	// allocate memory
		if (ptc.wfs == NULL) return EXIT_FAILURE;		
		
		logDebug("WFS_COUNT initialized: %d", ptc.wfs_count);
	}
	else if (strcmp(var, "WFC_COUNT") == 0) {
		ptc.wfc_count = (int) strtol(value, NULL, 10);
		ptc.wfc = malloc(ptc.wfc_count * sizeof(ptc.wfc));	// allocate memory
		if (ptc.wfc == NULL) return EXIT_FAILURE;

		logDebug("WFC_COUNT initialized: %d", ptc.wfc_count);
	}
	else if (strstr(var, "WFC_NAME") != NULL){
		if (ptc.wfc == NULL) {
			logErr("Cannot initialize WFC_NAME before initializing WFC_COUNT");
			return EXIT_FAILURE;
		}
		tmp = strtol(strstr(var,"[")+1, NULL, 10);
		ptc.wfc[tmp].name = value;
		
		logDebug("WFC_NAME initialized for WFC %d: %s", tmp, ptc.wfc[tmp].name);
	}
	else if (strstr(var, "WFC_NACT") != NULL) {
		if (ptc.wfc == NULL) {
			logErr("Cannot initialize WFC_NACT before initializing WFC_COUNT");
			return EXIT_FAILURE;
		}
		// Get the number of actuators for which WFC?
		tmp = strtol(strstr(var,"[")+1, NULL, 10);
		ptc.wfc[tmp].nact = strtol(value, NULL, 10);
		ptc.wfc[tmp].ctrl = malloc(ptc.wfc[tmp].nact * sizeof(ptc.wfc[tmp].ctrl));
		if (ptc.wfc[tmp].ctrl == NULL) return EXIT_FAILURE;
		
		logDebug("WFS_NACT initialized for WFS %d: %d", tmp, ptc.wfc[tmp].nact);
	}
	else if (strstr(var, "WFS_DF") != NULL) {
		if (ptc.wfs == NULL) {
			logErr("Cannot initialize WFS_DF before initializing WFS_COUNT");
			return EXIT_FAILURE;
		}
		// Get the DF of actuators for which WFC?
		tmp = strtol(strstr(var,"[")+1, NULL, 10);
		ptc.wfs[tmp].darkfile = value;
		
		logDebug("WFS_DF initialized for WFS %d: %s", tmp, ptc.wfs[tmp].darkfile);
	}
	else if (strstr(var, "WFS_FF") != NULL) {
		if (ptc.wfs == NULL) {
			logErr("Cannot initialize WFS_FF before initializing WFS_COUNT");
			return EXIT_FAILURE;
		}
		tmp = strtol(strstr(var,"[")+1, NULL, 10);
		ptc.wfs[tmp].flatfile = value;
		
		logDebug("WFS_FF initialized for WFS %d: %s", tmp, ptc.wfs[tmp].flatfile);
	}
	else if (strstr(var, "WFS_NAME") != NULL){
		if (ptc.wfs == NULL) {
			logErr("Cannot initialize WFS_NAME before initializing WFS_COUNT");
			return EXIT_FAILURE;
		}
		tmp = strtol(strstr(var,"[")+1, NULL, 10);
		ptc.wfs[tmp].name = value;
		
		logDebug("WFS_NAME initialized for WFS %d: %s", tmp, ptc.wfs[tmp].name);
	}
	else if (strstr(var, "WFS_CELLS") != NULL){
		if (ptc.wfs == NULL) {
			logErr("Cannot initialize WFS_CELLS before initializing WFS_COUNT");
			return EXIT_FAILURE;
		}
		tmp = strtol(strstr(var,"[")+1, NULL, 10);
		
		if (strstr(value,"{") == NULL || strstr(value,"}") == NULL || strstr(value,",") == NULL) 
			return EXIT_FAILURE;

		ptc.wfs[tmp].cellsx = strtol(strtok(value,"{,}"), NULL, 10);
		ptc.wfs[tmp].cellsy = strtol(strtok(NULL ,"{,}"), NULL, 10);

		logDebug("WFS_CELLS initialized for WFS %d: %d x %d", tmp, ptc.wfs[tmp].cellsx, ptc.wfs[tmp].cellsy);
	}
	else if (strstr(var, "WFS_RES") != NULL){
		if (ptc.wfs == NULL) {
			logErr("Cannot initialize WFS_RES before initializing WFS_COUNT");
			return EXIT_FAILURE;
		}
		tmp = strtol(strstr(var,"[")+1, NULL, 10);
		
		if (strstr(value,"{") == NULL || strstr(value,"}") == NULL || strstr(value,",") == NULL) 
			return EXIT_FAILURE;

		ptc.wfs[tmp].resx = strtol(strtok(value,"{,}"), NULL, 10);
		ptc.wfs[tmp].resy = strtol(strtok(NULL ,"{,}"), NULL, 10);

		logDebug("WFS_RES initialized for WFS %d: %d x %d", tmp, ptc.wfs[tmp].resx, ptc.wfs[tmp].resy);
	}
	else if (strcmp(var, "CS_LISTEN_IP") == 0) {
		strncpy(cs_config.listenip, value, 16);
		
		logDebug("CS_LISTEN_IP initialized: %s", cs_config.listenip);
	}
	else if (strcmp(var, "CS_LISTEN_PORT") == 0) {
		cs_config.listenport = (int) strtol(value, NULL, 10);
		
		logDebug("CS_LISTEN_PORT initialized: %d", cs_config.listenport);
	}
	else if (strcmp(var, "CS_USE_SYSLOG") == 0) {
		cs_config.use_syslog = ((int) strtol(value, NULL, 10) == 1) ? true : false;
		
		logDebug("CS_USE_SYSLOG initialized: %d", cs_config.use_syslog);
	}
	else if (strcmp(var, "CS_USE_STDERR") == 0) {
		cs_config.use_stderr = ((int) strtol(value, NULL, 10) == 1) ? true : false;
		
		logDebug("CS_USE_STDERR initialized: %d", cs_config.use_stderr);
	}
	else if (strcmp(var, "CS_INFOFILE") == 0) {
		strncpy(cs_config.infofile,value, FILENAMELEN);
		
		logDebug("CS_INFOFILE initialized: %s", cs_config.infofile);
	}
	else if (strcmp(var, "CS_ERRFILE") == 0) {
		strncpy(cs_config.errfile,value, FILENAMELEN);
		
		logDebug("CS_ERRFILE initialized: %s", cs_config.errfile);
	}
	else if (strcmp(var, "CS_DEBUGFILE") == 0) {
		strncpy(cs_config.debugfile,value, FILENAMELEN);
		
		logDebug("CS_DEBUGFILE initialized: %s", cs_config.debugfile);
	}

	return EXIT_SUCCESS;
}

int loadConfig(char *file) {
	logDebug("Reading configuration from file: %s",file);
	FILE *fp;
	char line[LINE_MAX];
	char var[LINE_MAX], value[LINE_MAX];
	
	fp = fopen(file,"r");
	if (fp == NULL)		// We can't open the file?
		return EXIT_FAILURE;
	
	while (fgets(line, LINE_MAX, fp) != NULL) {	// Read while there is no error
		if (*line == ' ' || *line == '\t' || *line == '\n' || *line == '#')
			continue;	// Skip bogus lines
		
		if (sscanf(line,"%s = %s", var, value) != 2)
			continue;	// Skip lines which do not adhere to 'var = value'-syntax
		
		logDebug("Parsing '%s' '%s' settings pair.", var, value);
		
		if (parseConfig(var,value) == EXIT_FAILURE)	// pass the pair on to be parsed and inserted in ptc
			return EXIT_FAILURE;
	}
	
	if (!feof(fp)) 		// Oops, not everything was read?
		return EXIT_FAILURE;
	
	// Check the info, error and debug files that we possibly have to log to
	initLogFiles();
	
	// Init syslog
	if (cs_config.use_syslog == true) {
		openlog(cs_config.syslog_prepend, LOG_PID, LOG_USER);	
		logInfo("Syslog successfully initialized.");
	}
	return EXIT_SUCCESS;
}

int initLogFiles() {
	if (strlen(cs_config.infofile) > 0) {
		if ((cs_config.infofd = fopen(cs_config.infofile,"a")) == NULL) {
			logErr("Unable to open file %s for info-logging! Not using this logmethod!", cs_config.infofile);
			cs_config.infofile[0] = '\0';
		}	
		else logDebug("Info logfile '%s' successfully opened.", cs_config.infofile);
	}
	if (strlen(cs_config.errfile) > 0) {
		if (strcmp(cs_config.errfile, cs_config.infofile) == 0) {	// If the errorfile is the same as the infofile, use the same FD
			cs_config.errfd = cs_config.infofd;
			logDebug("Using the same file '%s' for info- and error- logging.", cs_config.errfile);
		}
		else if ((cs_config.errfd = fopen(cs_config.errfile,"a")) == NULL) {
			logErr("Unable to open file %s for error-logging! Not using this logmethod!", cs_config.errfile);
			cs_config.errfile[0] = '\0';
		}
		else logDebug("Error logfile '%s' successfully opened.", cs_config.errfile);
	}
	if (strlen(cs_config.debugfile) > 0) {
		if (strcmp(cs_config.debugfile,cs_config.infofile) == 0) {
			cs_config.debugfd = cs_config.infofd;	
			logDebug("Using the same file '%s' for debug- and info- logging.", cs_config.debugfile);
		}
		else if (strcmp(cs_config.debugfile,cs_config.errfile) == 0) {
			cs_config.debugfd = cs_config.errfd;	
			logDebug("Using the same file '%s' for debug- and error- logging.", cs_config.debugfile);
		}
		else if ((cs_config.debugfd = fopen(cs_config.debugfile,"a")) == NULL) {
			logErr("Unable to open file %s for debug-logging! Not using this logmethod!", cs_config.debugfile);
			cs_config.debugfile[0] = '\0';
		}
		else logDebug("Debug logfile '%s' successfully opened.", cs_config.debugfile);
	}

	return EXIT_SUCCESS;
}

int saveConfig(char *file) {
	FILE *fp;
		
	fp = fopen(file,"w+");
	if (fp == NULL)		// We can't open the file?
		return EXIT_FAILURE;

	fputs("# Automatically created config file\n", fp);
	fputs("WFS_COUNT = 1\n", fp);
	
	fputs("WFC_COUNT = 2\n", fp);
	fputs("WFC_NACT[0] = 2\n", fp);
	fputs("WFC_NACT[1] = 37\n", fp);
	fputs("# EOF\n", fp);

	fclose(fp);

	return EXIT_SUCCESS;
}

void modeOpen() {
	logInfo("Entering open loop.");
		
	while (ptc.mode == AO_MODE_OPEN) {
		drvReadSensor();			// read the sensor output into ptc.image
	
		modParseSH();			// process SH sensor output, get displacements
	
		logInfo("Operating in open loop"); // TODO
		
		usleep(DEBUG_SLEEP);
	}
	
	modeListen();		// mode is not open anymore, decide what to to next
}

void modeClosed() {	
	logInfo("entering closed loop");
	while (ptc.mode == AO_MODE_CLOSED) {

		logInfo("Operating in closed loop"); // TODO, make faster for fast loop
		usleep(DEBUG_SLEEP);
	}
	
	modeListen();		// mode is not closed anymore, decide what to do
}

void modeListen() {
	logInfo("Entering listen mode");
	usleep(DEBUG_SLEEP);
		
	switch (ptc.mode) {
		case AO_MODE_OPEN:
		modeOpen();
		break;
		case AO_MODE_CLOSED:
		modeClosed();
		break;
		case AO_MODE_CAL:
		modeCal();
		break;
		default:
		modeListen(); // re-run if nothing was found
	}	
}

void modeCal() {
	logInfo("Entering calibration loop");
	
	logDebug("Calibration loop done, switching to open loop (was %d).", ptc.mode);
	ptc.mode = AO_MODE_OPEN;
	logDebug("mode now is %d", ptc.mode);
	usleep(DEBUG_SLEEP);
	
	modeListen();
}

int sockListen() {
	int sock, nsock, asock;
	fd_set read_fd_set, active_fd_set;
	int nbytes;
	char msg[LINE_MAX];
//	struct event sockevent;
	
	logDebug("Starting socklisten.");
	// Initialize the listening socket
	if ((sock = initSockL(&active_fd_set)) == EXIT_FAILURE)
		return EXIT_FAILURE;
		
	nsock = 1;
	
	logDebug("Successfully initialized socket.");
	
	//TODO: rewrite with libevent (portable and scalable)
	//event_init(); // init libevent
	/*event_set(&sockevent, sock, EV_READ, sockAccept2, &sockevent); // add to struct
	event_add(&sockevent, NULL); // set correctly
	
	// start looping
	event_dispatch();
	// END TODO
	
	exit(EXIT_SUCCESS);*/
	
	// Use select to poll the socket and multiplex I/O (actually only I)
	while (1) { // Listening for connections begin
		
		logInfo("Info: listening for connections (%d possible, %d used)",FD_SETSIZE,nsock);
		read_fd_set = active_fd_set;
		if (select(FD_SETSIZE, &read_fd_set, NULL, NULL, NULL) < 0) {	// Check for acitivity using select()
			// TODO: make this nicer (dicer) :)
			perror("error in select");
			exit(0);
			return EXIT_FAILURE;
		}
		
		asock = sockGetActive(&read_fd_set);			// Get the active FD to tend to
		
		if (asock == sock) {							// If the active FD == sock, accept a new connection
			if (sockAccept(asock, &active_fd_set) == EXIT_FAILURE) {
				// TODO fix exit
				
				perror("BAD!");
				return EXIT_FAILURE;
			}
			nsock++;
		}
		else {											// If the active FD != sock, read data (or close connection)
			nbytes = sockRead(asock, msg, sizeof(msg), &active_fd_set);
			
			if (nbytes == 0) nsock--;					// Socket closed
			else if (nbytes == EXIT_FAILURE) exit(0);	// Error on socket
			else {										// We got data
				logDebug("%d bytes received on the socket: '%s'", nbytes, msg);
				parseCmd(msg, nbytes, asock, &active_fd_set);	// Process the command
			}
		}
	}
		
	return EXIT_SUCCESS;
}

/*
void sockAccept2(int sock, short event, void *arg) {
	char buf[255];
	int len;
	struct event *ev = arg;

	// Reschedule this event
	event_add(ev, NULL);

	fprintf(stderr, "sockAccept2 called with sock: %d, event: %d, arg: %p\n",
		sock, event, arg);
	
	int newsock;					// New socket ID
	struct sockaddr_in cli_addr;	// Store the client address here
	socklen_t cli_len;
	
	cli_len = sizeof(cli_addr);
	newsock = accept(sock,
		(struct sockaddr *) &cli_addr,
		&cli_len);
		
	if (newsock < 0) { 				// on error, quit
		logErr("accepting socket failed.");
//		perror("error in accept");
//		exit(0);
		return EXIT_FAILURE;
	}
	
//	FD_SET(newsock, lfd_set); 		// Add socket to the set of sockets

	event_set(&evfifo, socket, EV_READ, fifo_read, &evfifo);
	return newsock;
		
	len = read(fd, buf, sizeof(buf) - 1);

	if (len == -1) {
		perror("read");
		return;
	} else if (len == 0) {
		fprintf(stderr, "Connection closed\n");
		return;
	}

	buf[len] = '\0';

	fprintf(stdout, "Read: %s\n", buf);
}*/

/*!
@brief Takes a string seperated by spaces as input and pops off the first word.
*/
int popword(char **msg, char *cmd) {
	size_t begin, end;
	
	// remove initial whitespace
	begin = strspn(*msg, " \t\n");
	if (begin > 0) {
		logDebug("Trimmin string begin %d chars", begin);
		*msg = *msg+begin;
	}
	
	// get first next position of a space
	end = strcspn(*msg, " \t\n");
	if (end == 0) {// No characters? return 0 to the calling function
		cmd[0] = '\0';
		return 0;
	}
		
	strncpy(cmd, *msg, end);
	cmd[end] = '\0'; // terminate string (TODO: strncpy does not do this, solution?)
	*msg = *msg+end+1;
	return strlen(cmd);
}

int parseCmd(char *msg, int len, int asock, fd_set *lfd_set) {
	char tmp[len+1];	// reserve space for the command (TODO: can be shorter using strchr. Can it? wordlength can vary...)	
	tmp[0] = '\0';
	
	logDebug("Command was: '%s'",msg);
	popword(&msg, tmp);
	logDebug("Command is '%s' and '%s'", msg, tmp);
	
	if (strcmp(tmp,"help") == 0) {
		// Show the help command
		if (popword(&msg, tmp) > 0) // Does the user want help on a specific command?
			showHelp(asock, tmp);
		else
			showHelp(asock, NULL);
			
		logInfo("Got help command & sent it! (subhelp %s)", tmp);
	}
	else if (strcmp(tmp,"mode") == 0) {
		if (popword(&msg, tmp) > 0) {
			if (strcmp(tmp,"closed") == 0)
				ptc.mode = AO_MODE_CLOSED;
			else if (strcmp(tmp,"open") == 0)
				ptc.mode = AO_MODE_OPEN;
			else if (strcmp(tmp,"cal") == 0)
				ptc.mode = AO_MODE_CAL;
		}
		else {
			showHelp(asock, "mode");
			logInfo("showing help...");
		}

		logInfo("subcommand: '%s'", tmp);
	}
	
	return EXIT_SUCCESS;
}

int showHelp(int sock, char *subhelp) {
	if (subhelp == NULL) {
		char help[] = "200 OK HELP\n\
help [command]: help (on a certain command, if available).\n\
mode <open|closed>: close or open the loop.\n\
simulate: toggle simulation mode.\n";

		return sendMsg(sock, help); 
	}
	else if (strcmp(subhelp, "mode") == 0) {
		char help[] = "200 OK HELP MODE\n\
mode <open|closed>: close or open the loop.\n\
mode open: opens the loop and only records what's happening with the AO system\n\
and does not actually drive anything.\n\
mode closed: closes the loop and starts the feedbackloop, correcting the wavefront as fast\n\
as possible.\n";

		return sendMsg(sock, help);
	}
	
	return EXIT_FAILURE;
}

int sendMsg(int sock, char *buf) {
	return write(sock, buf, strlen(buf)); // TODO non blocking maken	
}

int sockAccept(int sock, fd_set *lfd_set) {
	int newsock;					// New socket ID
	struct sockaddr_in cli_addr;	// Store the client address here
	socklen_t cli_len;
	
	cli_len = sizeof(cli_addr);
	newsock = accept(sock,
		(struct sockaddr *) &cli_addr,
		&cli_len);
		
	if (newsock < 0) { 				// on error, quit
		perror("error in accept");
		exit(0);
		newsock = EXIT_FAILURE;
	}
	
	FD_SET(newsock, lfd_set); 		// Add socket to the set of sockets
	
	return newsock;
}

int initSockL(fd_set *lfd_set) {
	int sock, optval=1; 			// Socket id
	struct sockaddr_in serv_addr;	// Store the server address here

	logInfo("Starting socket.");
	
	// Initialize the internet socket. We want streaming and we want TCP
	if ((sock = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
		// TODO: nicer error
		logErr("Thread: socket error! %s",strerror(errno));
		return EXIT_FAILURE;
	}
	logDebug("Socket created.");
	
	// Set reusable and nosigpipe flags so we don't get into trouble later on.
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval) != 0)
		logErr("Could not set socket flag SO_REUSEADDR, continuing.");
	if (setsockopt(sock, SOL_SOCKET, SO_NOSIGPIPE, &optval, sizeof optval) != 0)
		logErr("Could not set socket flag SO_NOSIGPIPE, continuing.");
		
	// Get the address fixed:
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(cs_config.listenip);
	serv_addr.sin_port = htons(cs_config.listenport);
	memset(serv_addr.sin_zero, '\0', sizeof(serv_addr.sin_zero)); // TODO: we need to set this padding to zero?
	
	logDebug("Mem set to zero for serv_addr.");
	
	// We now actually bind the socket to something
	if (bind(sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) == -1) {
		logErr("Binding socket failed: %s", strerror(errno));
		return EXIT_FAILURE;
	}
	
	if (listen (sock, 1) < 0) {
		logErr("listen failed: ", strerror(errno));
		return EXIT_FAILURE;	
	}
	
	FD_ZERO(lfd_set);		// init lfd_set
	FD_SET(sock, lfd_set); 	// add sock to the set

	return sock;
}

int sockGetActive(fd_set *lfd_set) {
	int i;

	for (i = 0; i < FD_SETSIZE; ++i) 
		if (FD_ISSET(i, lfd_set)) 
			return i;

	return EXIT_FAILURE; // This shouldn't happen (as with all errors)
}


// DOXYGEN GENERAL DOCUMENTATION //
/*********************************/

/*! \mainpage FOAM 
	
	\section aboutdoc About this document
	
	This is the (developer) documentation for FOAM, the Modular Adaptive 
	Optics Framework (yes, FOAM is backwards). It is intended to clarify the
	code and give some help to people re-using the code for their specific needs.
	
	\section aboutfoam About FOAM
	
	FOAM, the Modular Adaptive Optics Framework, is intended to be a modular
	framework which works independent of hardware and should provide such
	flexibility that the framework can be implemented on any AO system.
	
	A short list of the features of FOAM follows:
	\li Portable - It will run on many (Unix) systems and is hardware independent
	\li Scalable - It  will scale easily and handle multiple cores/CPU's
	\li Usable - It will be controllable over a network by multiple clients simultaneously
	\li Tracking - It will be able to track an object as it moves over the sky
	\li Distortion correction - It will correct wavefront distortions by anything in front of the corrector device
	\li MCAO - It will support multiple correctors and sensors simultaneously
	\li Offloading - It will be able to offload/distribute wavefront correction over several correctors
	\li Stepping - It will allow for stepping over an object
	\li Simulation - It will be able to simulate the whole AO system

	\section install_sec Installation
	
	FOAM requires \a libevent to be installed on the target machine, which is released under 
	a 3-clause BSD license and is avalaible here:
	 http://www.monkey.org/~provos/libevent/
	
	Furthermore FOAM requires basic things like a hosted compilation environment, pthreads, etc.

	\subsection drivers Write drivers
	
	You'll have to write your own drivers for all hardware components that 
	adhere to the interfaces described below. The functions will have to be
	defined in the config file, see next section.
	
	\subsection config Configure FOAM
	
	Configure FOAM, especially cs_config.h.
	
*/
