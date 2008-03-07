/*! 
	@file foam_cs.c
	@author @authortim
	@date November 14 2007

	@brief This is the main file for the @name Control Software.

	Partially functional alpha version of the @name Control Software\n\n
	TODO tags are used for work in progress or things that are unclear.
*/

// HEADERS //
/***********/

#include "foam_cs_library.h"

#define FOAM_CONFIG_FILE "../config/ao_config.cfg"

// GLOBAL VARIABLES //
/********************/	

// These are defined in foam_cs_library.c
extern control_t ptc;
extern config_t cs_config;
extern conntrack_t clientlist;

// PROTOTYPES //
/**************/	

// These come from modules, these MUST be defined there
extern void modStopModule(control_t *ptc);
extern int modInitModule(control_t *ptc);
extern int modOpenInit(control_t *ptc);
extern int modOpenLoop(control_t *ptc);
extern int modClosedInit(control_t *ptc);
extern int modClosedLoop(control_t *ptc);
extern int modCalibrate(control_t *ptc);

pthread_mutex_t mode_mutex;
pthread_cond_t mode_cond;

	/*! 
	@brief Initialisation function.
	
	\c main() initializes necessary variables, threads, etc. and
	then runs the AO in open-loop mode, from where the user can decide
	what to do.
	@return \c EXIT_FAILURE on failure, \c EXIT_SUCESS on successful completion.
	*/
int main(int argc, char *argv[]) {
	// INIT VARS // 
	/*************/
	
	pthread_t thread;
	if (pthread_mutex_init(&mode_mutex, NULL) != 0)
		logErr("pthread_mutex_init failed.");
	if (pthread_cond_init (&mode_cond, NULL) != 0)
		logErr("pthread_cond_init failed.");
	
	clientlist.nconn = 0;	// Init number of connections to zero

	char date[64];
	struct tm *loctime;

	// SIGNAL HANDLERS //
	/*******************/

	signal(SIGINT, catchSIGINT);
	signal(SIGPIPE, SIG_IGN);
	
	// BEGIN FOAM //
	/**************/
	
	logInfo("Starting %s (%s) by %s",FOAM_NAME, FOAM_VERSION, FOAM_AUTHOR);

	ptc.starttime = time (NULL);
	loctime = localtime (&ptc.starttime);
	strftime (date, 64, "%A, %B %d %H:%M:%S, %Y (%Z).", loctime);	
	logInfo("at %s", date);
		
	// BEGIN LOADING CONFIG
	if (loadConfig(FOAM_CONFIG_FILE) != EXIT_SUCCESS)
		logErr("Loading configuration failed");

	logInfo("Configuration successfully loaded...");	
	
	// INITIALIZE MODULES //
	/**********************/

	modInitModule(&ptc);
	
	// Create thread which listens to clients on a socket		
	if ((pthread_create(&thread,
		NULL,
		(void *) sockListen,
		NULL)
		) != 0)
		logErr("Error in pthread_create: %s.", strerror(errno));
		
	modeListen(); 			// After initialization, start in open mode

	return EXIT_SUCCESS;
}

void catchSIGINT() {
	// reset signal handler, as noted on http://www.cs.cf.ac.uk/Dave/C/node24.html
	signal(SIGINT, catchSIGINT);
	
	// stop the framework
	stopFOAM();
}

void stopFOAM() {
	char date[64];
	struct tm *loctime;
	time_t end;
	
	end = time (NULL);
	loctime = localtime (&end);
	strftime (date, 64, "%A, %B %d %H:%M:%S, %Y (%Z).", loctime);	
	
	logInfo("Trying to stop modules...");
	modStopModule(&ptc);
	
	logInfo("Stopping threads...");
	pthread_mutex_destroy(&mode_mutex);
	pthread_cond_destroy(&mode_cond);
//	pthread_exit(NULL);
	
	logInfo("Stopping FOAM at %s", date);
	logInfo("Ran for %ld seconds and parsed %ld frames (framerate: %f).", \
		end-ptc.starttime, ptc.frames, ptc.frames/(float) (end-ptc.starttime));

	// fclose(cs_config.infofd);
	// fclose(cs_config.errfd);
	// fclose(cs_config.debugfd);
	exit(EXIT_SUCCESS);
}

// tiny helper function
int issetWFC(char *var) {
	if (ptc.wfc == NULL) {
		logWarn("Cannot initialize %s before initializing WFC_COUNT", var);
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

int issetWFS(char *var) {
	if (ptc.wfs == NULL) {
		logWarn("Cannot initialize %s before initializing WFS_COUNT", var);
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

int validWFC(char *var) {
	int tmp;
	tmp = strtol(strstr(var,"[")+1, NULL, 10);
	if (tmp >= ptc.wfc_count) {
		logWarn("Corrupt configuration, found config for WFC %d (%s) while WFC count is only %d.", tmp, var, ptc.wfc_count);
		return -1; // TODO: we deviate from errorcodes here, acceptable?
	}
	return tmp;
}

int validWFS(char *var) {
	int tmp;
	tmp = strtol(strstr(var,"[")+1, NULL, 10);
	if (tmp >= ptc.wfs_count) {
		logWarn("Corrupt configuration, found config for WFS %d (%s) while WFS count is only %d.", tmp, var, ptc.wfs_count);
		return -1; // TODO: we deviate from errorcodes here, acceptable?
	}
	return tmp;
}


int parseConfig(char *var, char *value) {
	int tmp, i;

	if (strcmp(var, "WFS_COUNT") == 0) {
		ptc.wfs_count = (int) strtol(value, NULL, 10);
		ptc.wfs = calloc(ptc.wfs_count, sizeof(*ptc.wfs));	// allocate memory
		if (ptc.wfs == NULL) {
			logErr("Failed to allocate ptc.wfs");
			return EXIT_FAILURE;		
		}
		
		// initialize some things to zero
		for (i=0; i<ptc.wfs_count; i++) {
			ptc.wfs[i].singular = NULL;
			ptc.wfs[i].dmmodes = NULL;
			ptc.wfs[i].wfsmodes = NULL;
			ptc.wfs[i].stepc.x = 0;
			ptc.wfs[i].stepc.y = 0;
		}
		
		logInfo("WFS_COUNT initialized: %d", ptc.wfs_count);
	}
	else if (strcmp(var, "WFC_COUNT") == 0) {
		ptc.wfc_count = (int) strtol(value, NULL, 10);
		ptc.wfc = calloc(ptc.wfc_count, sizeof(*ptc.wfc));
		
		if (ptc.wfc == NULL) {
			logErr("Failed to allocate ptc.wfc");
			return EXIT_FAILURE;
		}

		logInfo("WFC_COUNT initialized: %d", ptc.wfc_count);
	}
	else if (strstr(var, "WFC_NAME") != NULL) {
		if (issetWFC(var) != EXIT_SUCCESS) return EXIT_FAILURE;
		if ((tmp = validWFC(var)) < 0) return EXIT_FAILURE;
		
		strncpy(ptc.wfc[tmp].name, value, (size_t) FILENAMELEN);
		ptc.wfc[tmp].name[FILENAMELEN-1] = '\0'; // TODO: This might not be necessary
		
		logInfo("WFC_NAME initialized for WFC %d: %s", tmp, ptc.wfc[tmp].name);
	}
    else if (strstr(var, "WFC_NACT") != NULL) {
		if (issetWFC(var) != EXIT_SUCCESS) return EXIT_FAILURE;
		if ((tmp = validWFC(var)) < 0) return EXIT_FAILURE;

		// Get the number of actuators for which WFC?
		// tmp = strtol(strstr(var,"[")+1, NULL, 10);
		ptc.wfc[tmp].nact = strtol(value, NULL, 10);
		ptc.wfc[tmp].ctrl = gsl_vector_float_calloc(ptc.wfc[tmp].nact);
		if (ptc.wfc[tmp].ctrl == NULL) return EXIT_FAILURE;

		logInfo("WFS_NACT initialized for WFS %d: %d", tmp, ptc.wfc[tmp].nact);
    }
    else if (strstr(var, "WFC_GAIN") != NULL) {
		if (issetWFC(var) != EXIT_SUCCESS) return EXIT_FAILURE;
		if ((tmp = validWFC(var)) < 0) return EXIT_FAILURE;
		
		ptc.wfc[tmp].gain = strtof(value, NULL);

		logInfo("WFC_GAIN initialized for WFS %d: %f", tmp, ptc.wfc[tmp].gain);
    }
	else if (strstr(var, "WFS_NAME") != NULL) {
		if (issetWFS(var) != EXIT_SUCCESS) return EXIT_FAILURE;
		if ((tmp = validWFS(var)) < 0) return EXIT_FAILURE;
		
		strncpy(ptc.wfs[tmp].name, value, (size_t) FILENAMELEN);
		ptc.wfs[tmp].name[FILENAMELEN-1] = '\0'; // This might not be necessary
				
		logInfo("WFS_NAME initialized for WFS %d: %s", tmp, ptc.wfs[tmp].name);
	}
	else if (strstr(var, "WFS_DF") != NULL) {
		if (issetWFS(var) != EXIT_SUCCESS) return EXIT_FAILURE;
		if ((tmp = validWFS(var)) < 0) return EXIT_FAILURE;

		strncpy(ptc.wfs[tmp].darkfile, value, (size_t) FILENAMELEN);
		ptc.wfs[tmp].darkfile[FILENAMELEN-1] = '\0'; // This might not be necessary
		
		logInfo("WFS_DF initialized for WFS %d: %s", tmp, ptc.wfs[tmp].darkfile);
	}
	else if (strstr(var, "WFS_SKY") != NULL) {
		if (issetWFS(var) != EXIT_SUCCESS) return EXIT_FAILURE;
		if ((tmp = validWFS(var)) < 0) return EXIT_FAILURE;

		strncpy(ptc.wfs[tmp].skyfile, value, (size_t) FILENAMELEN);
		ptc.wfs[tmp].skyfile[FILENAMELEN-1] = '\0'; // This might not be necessary
		
		logInfo("WFS_SKY initialized for WFS %d: %s", tmp, ptc.wfs[tmp].skyfile);
	}
	else if (strstr(var, "WFS_PINHOLE") != NULL) {
		if (issetWFS(var) != EXIT_SUCCESS) return EXIT_FAILURE;
		if ((tmp = validWFS(var)) < 0) return EXIT_FAILURE;

		strncpy(ptc.wfs[tmp].pinhole, value, (size_t) FILENAMELEN);
		ptc.wfs[tmp].pinhole[FILENAMELEN-1] = '\0'; // This might not be necessary
		
		logInfo("WFS_PINHOLE initialized for WFS %d: %s", tmp, ptc.wfs[tmp].pinhole);
	}
	else if (strstr(var, "WFS_INFL") != NULL) {
		if (issetWFS(var) != EXIT_SUCCESS) return EXIT_FAILURE;
		if ((tmp = validWFS(var)) < 0) return EXIT_FAILURE;

		strncpy(ptc.wfs[tmp].influence, value, (size_t) FILENAMELEN);
		ptc.wfs[tmp].influence[FILENAMELEN-1] = '\0'; // This might not be necessary
		
		logInfo("WFS_INFL initialized for WFS %d: %s", tmp, ptc.wfs[tmp].influence);
	}
	else if (strstr(var, "WFS_FF") != NULL) {
		if (issetWFS(var) != EXIT_SUCCESS) return EXIT_FAILURE;
		if ((tmp = validWFS(var)) < 0) return EXIT_FAILURE;

		strncpy(ptc.wfs[tmp].flatfile, value, (size_t) FILENAMELEN);
		ptc.wfs[tmp].flatfile[FILENAMELEN-1] = '\0'; // This might not be necessary

		logInfo("WFS_FF initialized for WFS %d: %s", tmp, ptc.wfs[tmp].flatfile);
	}
	else if (strstr(var, "WFS_CELLS") != NULL){
		if (issetWFS(var) != EXIT_SUCCESS) return EXIT_FAILURE;
		if ((tmp = validWFS(var)) < 0) return EXIT_FAILURE;
		
		if (strstr(value,"{") == NULL || strstr(value,"}") == NULL || strstr(value,",") == NULL) 
			return EXIT_FAILURE; // return if we don't have the {x,y} syntax

		ptc.wfs[tmp].cells[0] = strtol(strtok(value,"{,}"), NULL, 10);
		ptc.wfs[tmp].cells[1] = strtol(strtok(NULL ,"{,}"), NULL, 10);
		
		if (ptc.wfs[tmp].cells[0] % 2 != 0 || ptc.wfs[tmp].cells[1] % 2 != 0) {
			logErr("WFS %d has an odd cell-resolution (%dx%d), not supported. Please only use 2nx2n cells.", \
				tmp, ptc.wfs[tmp].cells[0], ptc.wfs[tmp].cells[1]);
			return EXIT_FAILURE;
		}
		
		ptc.wfs[tmp].subc = calloc(ptc.wfs[tmp].cells[0] * ptc.wfs[tmp].cells[1], sizeof(*ptc.wfs[tmp].subc));
		ptc.wfs[tmp].gridc = calloc(ptc.wfs[tmp].cells[0] * ptc.wfs[tmp].cells[1], sizeof(*ptc.wfs[tmp].gridc));
		
		ptc.wfs[tmp].refc = gsl_vector_float_calloc(ptc.wfs[tmp].cells[0] * ptc.wfs[tmp].cells[1] * 2);
		ptc.wfs[tmp].disp = gsl_vector_float_calloc(ptc.wfs[tmp].cells[0] * ptc.wfs[tmp].cells[1] * 2);
		if (ptc.wfs[tmp].subc == NULL || ptc.wfs[tmp].refc == NULL || ptc.wfs[tmp].disp == NULL) {
			logErr("Cannot allocate memory for tracker window coordinates, or other tracking vectors.");
			return EXIT_FAILURE;
		}
		
		if (ptc.wfs[tmp].res.x * ptc.wfs[tmp].res.y <= 0) {
			logErr("Cannot initialize WFS_CELLS before WFS_RES");
			return EXIT_FAILURE;
		}
		
		ptc.wfs[tmp].shsize[0] = (ptc.wfs[tmp].res.x / ptc.wfs[tmp].cells[0]);
		ptc.wfs[tmp].shsize[1] = (ptc.wfs[tmp].res.y / ptc.wfs[tmp].cells[1]);
		ptc.wfs[tmp].refim = calloc(ptc.wfs[tmp].shsize[0] * \
			ptc.wfs[tmp].shsize[1], sizeof(ptc.wfs[tmp].refim));
		if (ptc.wfs[tmp].refim == NULL) {
			logErr("Failed to allocate image memory for reference image.");
			return EXIT_FAILURE;
		}

		logInfo("WFS_CELLS initialized for WFS %d: (%dx%d). Subapt resolution is (%dx%d) pixels", \
			tmp, ptc.wfs[tmp].cells[0], ptc.wfs[tmp].cells[1], ptc.wfs[tmp].shsize[0], ptc.wfs[tmp].shsize[1]);
	}
	else if (strstr(var, "WFS_RES") != NULL){
		if (issetWFS(var) != EXIT_SUCCESS) return EXIT_FAILURE;
		if ((tmp = validWFS(var)) < 0) return EXIT_FAILURE;
		
		if (strstr(value,"{") == NULL || strstr(value,"}") == NULL || strstr(value,",") == NULL) 
			return EXIT_FAILURE;

		ptc.wfs[tmp].res.x = strtol(strtok(value,"{,}"), NULL, 10);
		ptc.wfs[tmp].res.y = strtol(strtok(NULL ,"{,}"), NULL, 10);
		
		if (ptc.wfs[tmp].res.x % 2 != 0 || ptc.wfs[tmp].res.y % 4 != 0) {
			logErr("WFS %d has an odd resolution (%dx%d), not supported. Please only use 2nx2n pixels.", \
				tmp, ptc.wfs[tmp].res.x, ptc.wfs[tmp].res.y);
			return EXIT_FAILURE;
		}		
		
		// Allocate memory for all images we need lateron
		ptc.wfs[tmp].image = calloc(ptc.wfs[tmp].res.x * ptc.wfs[tmp].res.y, sizeof(ptc.wfs[tmp].image));
		ptc.wfs[tmp].darkim = calloc(ptc.wfs[tmp].res.x * ptc.wfs[tmp].res.y, sizeof(ptc.wfs[tmp].darkim));
		ptc.wfs[tmp].flatim = calloc(ptc.wfs[tmp].res.x * ptc.wfs[tmp].res.y, sizeof(ptc.wfs[tmp].flatim));
		ptc.wfs[tmp].corrim = calloc(ptc.wfs[tmp].res.x * ptc.wfs[tmp].res.y, sizeof(ptc.wfs[tmp].corrim));
		
		// check if everything worked out ok
		if ((ptc.wfs[tmp].image == NULL) ||
				(ptc.wfs[tmp].darkim == NULL) ||
				(ptc.wfs[tmp].flatim == NULL) ||
				(ptc.wfs[tmp].corrim == NULL)) {
			logErr("Failed to allocate image memory (image, dark, flat, corrected).");
			return EXIT_FAILURE;
		}
		
		logInfo("WFS_RES initialized for WFS %d: %d x %d", tmp, ptc.wfs[tmp].res.x, ptc.wfs[tmp].res.y);
	}

	else if (strcmp(var, "CS_LISTEN_IP") == 0) {
		strncpy(cs_config.listenip, value, 16);
		
		logInfo("CS_LISTEN_IP initialized: %s", cs_config.listenip);
	}
	else if (strcmp(var, "CS_LISTEN_PORT") == 0) {
		cs_config.listenport = (int) strtol(value, NULL, 10);
		
		logInfo("CS_LISTEN_PORT initialized: %d", cs_config.listenport);
	}
	else if (strcmp(var, "CS_USE_SYSLOG") == 0) {
		cs_config.use_syslog = ((int) strtol(value, NULL, 10) == 0) ? false : true;
		
		logInfo("CS_USE_SYSLOG initialized: %d", cs_config.use_syslog);
	}
	else if (strcmp(var, "CS_USE_STDERR") == 0) {
		cs_config.use_stderr = ((int) strtol(value, NULL, 10) == 0) ? false : true;
		
		logInfo("CS_USE_STDERR initialized: %d", cs_config.use_stderr);
	}
	else if (strcmp(var, "CS_INFOFILE") == 0) {
		strncpy(cs_config.infofile,value, (size_t) FILENAMELEN);
		cs_config.infofile[FILENAMELEN-1] = '\0'; // TODO: is this necessary?
		logInfo("CS_INFOFILE initialized: %s", cs_config.infofile);
	}
	else if (strcmp(var, "CS_ERRFILE") == 0) {
		strncpy(cs_config.errfile,value, (size_t) FILENAMELEN);
		cs_config.errfile[FILENAMELEN-1] = '\0'; // TODO: is this necessary?
		logInfo("CS_ERRFILE initialized: %s", cs_config.errfile);
	}
	else if (strcmp(var, "CS_DEBUGFILE") == 0) {
		strncpy(cs_config.debugfile, value, (size_t) FILENAMELEN);
		cs_config.debugfile[FILENAMELEN-1] = '\0'; // TODO: is this necessary?
		logInfo("CS_DEBUGFILE initialized: %s", cs_config.debugfile);
	}

	return EXIT_SUCCESS;
}

int loadConfig(char *file) {
	logDebug("Reading configuration from file: %s",file);
	FILE *fp;
	char line[COMMANDLEN];
	char var[COMMANDLEN], value[COMMANDLEN];
	
	fp = fopen(file,"r");
	if (fp == NULL)		// We can't open the file?
		return EXIT_FAILURE;
	
	while (fgets(line, COMMANDLEN, fp) != NULL) {	// Read while there is no error
		if (*line == ' ' || *line == '\t' || *line == '\n' || *line == '#')
			continue;	// Skip bogus lines
		
		if (sscanf(line,"%s = %s", var, value) != 2)
			continue;	// Skip lines which do not adhere to 'var = value'-syntax
		
		logDebug("Parsing '%s' '%s' settings pair.", var, value);
		
		if (parseConfig(var,value) != EXIT_SUCCESS)	// pass the pair on to be parsed and inserted in ptc
			return EXIT_FAILURE;		
	}
	
	if (!feof(fp)) 		// Oops, not everything was read?
		return EXIT_FAILURE;
	else 
		fclose(fp);
		
	// begin postconfig check
	// TvW: TODO
	
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
	else {
		logDebug("Not logging general info to disk.");
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
	else {
		logDebug("Not logging errors to disk.");
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
	else {
		logDebug("Not logging debug to disk.");
	}

	return EXIT_SUCCESS;
}

int saveConfig(char *file) {
	FILE *fp;
		
	fp = fopen(file,"w+");
	if (fp == NULL)		// We can't open the file
		return EXIT_FAILURE;

	// this is not complete yet
	// TODO
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
	ptc.frames++;
	logInfo("Entering open loop.");

	// perform some sanity checks before actually entering the open loop
	if (ptc.wfs_count == 0) {				// we need wave front sensors
		logErr("Error, no WFSs defined.");
		ptc.mode = AO_MODE_LISTEN;
		return;
	}
	
	// Run the initialisation function of the modules used, pass
	// along a pointer to ptc
	if (modOpenInit(&ptc) != EXIT_SUCCESS) {		// check if we can init the module
		logWarn("modOpenInit failed");
		ptc.mode = AO_MODE_LISTEN;
		return;
	}
	
	
//	int tmp[] = {32, 32};

	while (ptc.mode == AO_MODE_OPEN) {
		ptc.frames++;								// increment the amount of frames parsed		
		logInfo("Operating in open loop"); 
		
		if (modOpenLoop(&ptc) != EXIT_SUCCESS) {
			logWarn("modOpenLoop failed");
			ptc.mode = AO_MODE_LISTEN;
			return;
		}
								

		// if (ptc.frames > 500) 					// exit for debugging
		// 	stopFOAM();

		//sleep(DEBUG_SLEEP);
	}
	
	return; // mode is not open anymore, decide what to to next
}

void modeClosed() {	
	logInfo("Entering closed loop.");

	// perform some sanity checks before actually entering the open loop
	if (ptc.wfs_count == 0) {						// we need wave front sensors
		logErr("Error, no WFSs defined.");
		ptc.mode = AO_MODE_LISTEN;
		return;
	}
	
	// Run the initialisation function of the modules used, pass
	// along a pointer to ptc
	if (modClosedInit(&ptc) != EXIT_SUCCESS) {
		logWarn("modClosedInit failed");
		ptc.mode = AO_MODE_LISTEN;
		return;
	}
	
	ptc.frames++;
	while (ptc.mode == AO_MODE_CLOSED) {
		logInfo("Operating in closed loop"); 
		
		if (modClosedLoop(&ptc) != EXIT_SUCCESS) {
			logWarn("modClosedLoop failed");
			ptc.mode = AO_MODE_LISTEN;
			return;
		}			
		// if (ptc.frames > 2000) // exit for debugging
		// 	exit(EXIT_SUCCESS);
				
		ptc.frames++;								// increment the amount of frames parsed		

		//sleep(DEBUG_SLEEP);
	}
	
	return;					// back to modeListen (or where we came from)
}

void modeCal() {
	logInfo("Entering calibration loop");
	
	// this links to a module
	if (modCalibrate(&ptc) != EXIT_SUCCESS) {
		logWarn("modCalibrate failed");
		tellClients("400 ERROR CALIBRATION FAILED");
		ptc.mode = AO_MODE_LISTEN;
		return;
	}
	
	logDebug("Calibration loop done, switching to listen mode");
	tellClients("200 CALIBRATION SUCCESSFUL");
	ptc.mode = AO_MODE_LISTEN;
		
	return;
}

void modeListen() {
	
	while (true) {
		logInfo("Entering listen mode");
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
			case AO_MODE_LISTEN:
				// we wait until the mode changed
				pthread_mutex_lock(&mode_mutex);
				pthread_cond_wait(&mode_cond, &mode_mutex);
				pthread_mutex_unlock(&mode_mutex);
				break;
		}
	} // end while(true)
}

int sockListen() {
	int sock;						// Stores the main socket listening for connections
	struct event sockevent;			// Stores the associated event
	int optval=1; 					// Used to set socket options
	struct sockaddr_in serv_addr;	// Store the server address here
	
	event_init();					// Initialize libevent
	
	logInfo("Starting socket.");
	
	// Initialize the internet socket. We want streaming and we want TCP
	if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		logErr("Listening socket error: %s",strerror(errno));
		return EXIT_FAILURE; // TODO: we cant return here, we're in a thread. Exit?
	}
	
	logDebug("Socket created.");
		
	// Get the address fixed:
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(cs_config.listenip);
	serv_addr.sin_port = htons(cs_config.listenport);
	memset(serv_addr.sin_zero, '\0', sizeof(serv_addr.sin_zero));
	
	logDebug("Mem set to zero for serv_addr.");

	logDebug("Setting SO_REUSEADDR, SO_NOSIGPIPE and nonblocking...");
	// Set reusable and nosigpipe flags so we don't get into trouble later on. TODO: doesn't work?
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) != 0)
		logErr("Could not set socket flag SO_REUSEADDR, continuing.");
	
	// We now actually bind the socket to something
	if (bind(sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) != 0) {
		logErr("Binding socket failed: %s", strerror(errno));
		exit(EXIT_FAILURE);
	}
	
	if (listen (sock, 1) != 0) {
		logErr("Listen failed: %s", strerror(errno));
		exit(EXIT_FAILURE);
	}
		
	// Set socket to non-blocking mode, nice for events
	if (setnonblock(sock) != 0)
		logErr("Coult not set socket to non-blocking mode, might cause undesired side-effects, continuing.");
		
	logDebug("Successfully initialized socket, setting up events.");
	
    event_set(&sockevent, sock, EV_READ | EV_PERSIST, sockAccept, NULL);
    event_add(&sockevent, NULL);

	event_dispatch();				// Start looping
	
	return EXIT_SUCCESS;
}

int setnonblock(int fd) {
	int flags;

    flags = fcntl(fd, F_GETFL);
    if (flags < 0)
            return flags;
    flags |= O_NONBLOCK;
    if (fcntl(fd, F_SETFL, flags) < 0)
            return EXIT_FAILURE;

    return EXIT_SUCCESS;
}

void sockAccept(const int sock, const short event, void *arg) {
	int newsock, i;					// Store the newly accepted connection here
	struct sockaddr_in cli_addr;	// Store the client address here
	socklen_t cli_len;				// store the length here
	client_t *client;				// connection data
	cli_len = sizeof(cli_addr);
	
	logDebug("Handling new client connection.");
	
	newsock = accept(sock, (struct sockaddr *) &cli_addr, &cli_len);
		
	if (newsock < 0) { 				// on error, quit
		logErr("Accepting socket failed: %s!", strerror(errno));
		return;
	}
	
	if (setnonblock(newsock) < 0)
		logErr("Unable to set new client socket to non-blocking.");

	logDebug("Accepted & set to nonblock.");
	
	// Check if we do not exceed the maximum number of connections:
	if (clientlist.nconn >= MAX_CLIENTS) {
		sleep(1);
		close(newsock);
		logWarn("Refused connection, maximum clients reached (%d)", MAX_CLIENTS);
		return;
	}

	
	client = calloc(1, sizeof(*client));
	if (client == NULL)
		logErr("Malloc failed in sockAccept.");

	client->fd = newsock;			// Add socket to the client struct
	client->buf_ev = bufferevent_new(newsock, sockOnRead, sockOnWrite, sockOnErr, client);

	clientlist.nconn++;
	for (i=0; clientlist.connlist[i] != NULL && i < 16; i++) 
		;	// Run through array until we find an empty connlist entry

	client->connid = i;
	clientlist.connlist[i] = client;

	// We have to enable it before our callbacks will be
	if (bufferevent_enable(client->buf_ev, EV_READ) != 0) {
		logErr("Failed to enable buffered event.");
		return;
	}

	logInfo("Succesfully accepted connection from %s (using sock %d and buf_ev %p)", \
		inet_ntoa(cli_addr.sin_addr), newsock, client->buf_ev);
	
	bufferevent_write(client->buf_ev,"200 OK CONNECTION ESTABLISHED\n", sizeof("200 OK CONNECTION ESTABLISHED\n"));

}

void sockOnErr(struct bufferevent *bev, short event, void *arg) {
	client_t *client = (client_t *)arg;

	if (event & EVBUFFER_EOF) { // En
		logInfo("Client disconnected.");
	}
	else {
		logErr("Client socket error, disconnecting.");
	}
	clientlist.nconn--;
	clientlist.connlist[(int) client->connid] = NULL; // Does this work nicely?
	
	bufferevent_free(client->buf_ev);
	close(client->fd);
	free(client);
}

// This does nothing, but libevent really wants an onwrite function, NULL pointers crash it
void sockOnWrite(struct bufferevent *bev, void *arg) {
}

void sockOnRead(struct bufferevent *bev, void *arg) {
	client_t *client = (client_t *)arg;

	char msg[COMMANDLEN];
	char *tmp;
	int nbytes;
	
	while ((nbytes = bufferevent_read(bev, msg, (size_t) COMMANDLEN-1)) == COMMANDLEN-1) // detect very long messages
		logErr("Received very long command over socket which got cropped.");
		
	msg[nbytes] = '\0';
	// TODO: this still requires a neat solution, does not work with TELNET.
	if ((tmp = strchr(msg, '\n')) != NULL) // there might be a trailing newline which we don't want
		*tmp = '\0';
	if ((tmp = strchr(msg, '\r')) != NULL) // there might be a trailing newline which we don't want
		*tmp = '\0';
	
	logDebug("Received %d bytes on socket reading: '%s'.", nbytes, msg);
	parseCmd(msg, nbytes, client);
}

/*int explode(char *msg, char **arr) {
	size_t begin, end;	
	int i, maxlen=0;
	char *orig=msg;
	
	for(i=0; end != 0; i++) {
		begin = strspn(msg, " \t\n");
		if (begin > 0) msg = msg+begin;
		
		// get first next position of a space
		end = strcspn(msg, " \t\n");

		msg = msg+end+1;
		if ((int) end > maxlen) maxlen = (int) end;
	}
	
	if (i == 0) return EXIT_FAILURE;
	
	char array[i][maxlen+1];
	
	for(i=0; end != 0; i++) {
		begin = strspn(orig, " \t\n");
		if (begin > 0) orig = orig+begin;
		
		// get first next position of a space
		end = strcspn(orig, " \t\n");
		
		strncpy(array[i], orig, end);
		array[i][end] = '\0';
		
		orig = orig+end+1;
	}
	
	return EXIT_SUCCESS;
}*/

int popword(char **msg, char *cmd) {
	size_t begin, end;
	
	// remove initial whitespace
	begin = strspn(*msg, " \t\n");
	if (begin > 0) {
		logDebug("Trimming string begin %d chars", begin);
		*msg = *msg+begin;
	}
	
	// get first next position of a space
	end = strcspn(*msg, " \t\n");
	if (end == 0) { // No characters? return 0 to the calling function
		cmd[0] = '\0';
		return 0;
	}
		
	strncpy(cmd, *msg, end);
	cmd[end] = '\0'; // terminate string (TODO: strncpy does not do this, solution?)
	*msg = *msg+end+1;
	return strlen(cmd);
}

int parseCmd(char *msg, const int len, client_t *client) {
	char tmp[len+1];	// reserve space for the command (TODO: can be shorter using strchr. Can it? wordlength can vary...)	
	tmp[0] = '\0';
	char *chk;

//	logDebug("Command was: '%s'",msg);
		
	if (popword(&msg, tmp) > 0)
		logDebug("First word: '%s'", tmp);
		

	if (strcmp(tmp,"help") == 0) {
		// Show the help command
		if (popword(&msg, tmp) > 0) // Does the user want help on a specific command?
			showHelp(client, tmp);
		else
			showHelp(client, NULL);			
//		logDebug("Got help command & sent it! (subhelp %s)", tmp);
	}
	else if ((strcmp(tmp,"exit") == 0) || (strcmp(tmp,"quit") == 0)) {
		tellClients("200 OK EXIT");
//		bufferevent_write(client->buf_ev,"200 OK EXIT\n", sizeof("200 OK EXIT\n"));
		sockOnErr(client->buf_ev, EVBUFFER_EOF, client);
	}
	else if (strcmp(tmp,"shutdown") == 0) {
		tellClients("200 OK SHUTDOWN");
//		bufferevent_write(client->buf_ev,"200 OK SHUTDOWN\n", sizeof("200 OK SHUTDOWN\n"));
		sockOnErr(client->buf_ev, EVBUFFER_EOF, client);
		stopFOAM();
	}

	else if (strcmp(tmp,"mode") == 0) {
		if (popword(&msg, tmp) > 0) {
			if (strcmp(tmp,"closed") == 0) {
				ptc.mode = AO_MODE_CLOSED;
				pthread_mutex_lock(&mode_mutex); // signal a change to the main thread
				pthread_cond_signal(&mode_cond);
				pthread_mutex_unlock(&mode_mutex);
				tellClients("200 OK MODE CLOSED");
//				bufferevent_write(client->buf_ev,"200 OK MODE CLOSED\n", sizeof("200 OK MODE CLOSED\n"));
			}
			else if (strcmp(tmp,"open") == 0) {
				ptc.mode = AO_MODE_OPEN;
				pthread_mutex_lock(&mode_mutex);
				pthread_cond_signal(&mode_cond);
				pthread_mutex_unlock(&mode_mutex);
				tellClients("200 OK MODE OPEN");
//				bufferevent_write(client->buf_ev,"200 OK MODE OPEN\n", sizeof("200 OK MODE OPEN\n"));
			}
			else if (strcmp(tmp,"listen") == 0) {
				ptc.mode = AO_MODE_LISTEN;
				pthread_mutex_lock(&mode_mutex);
				pthread_cond_signal(&mode_cond);
				pthread_mutex_unlock(&mode_mutex);
				tellClients("200 OK MODE LISTEN");
//				bufferevent_write(client->buf_ev,"200 OK MODE LISTEN\n", sizeof("200 OK MODE LISTEN\n"));
			}
			else {
				bufferevent_write(client->buf_ev,"401 UNKNOWN MODE\n", sizeof("400 UNKNOWN MODE\n"));
			}
		}
		else {
			bufferevent_write(client->buf_ev,"402 MODE REQUIRES ARG\n", sizeof("400 MODE REQUIRES ARG\n"));
		}
	}
	else if (strcmp(tmp,"step") == 0) {
		if (ptc.mode == AO_MODE_CAL) bufferevent_write(client->buf_ev,"403 STEP NOT ALLOWED DURING CALIBRATION\n", sizeof("403 STEP NOT ALLOWED DURING CALIBRATION\n"));
		else if (popword(&msg, tmp) > 0) {
			if (strcmp(tmp,"x") == 0) {
				if (popword(&msg, tmp) > 0) {
					if (strtof(tmp, NULL) > -10 && strtof(tmp, NULL) < 10) {
						ptc.wfs[0].stepc.x = strtof(tmp, NULL);
						tellClients("200 OK STEP X");
					}
					else bufferevent_write(client->buf_ev,"401 UNKNOWN STEPSIZE\n", sizeof("401 UNKNOWN STEPSIZE\n"));
				}
				else {
					ptc.wfs[0].stepc.x += 1;
					tellClients("200 OK STEP X +1");
				}
			}
			if (strcmp(tmp,"y") == 0) {
				if (popword(&msg, tmp) > 0) {
					if (strtof(tmp, NULL) > -10 && strtof(tmp, NULL) < 10) {
						ptc.wfs[0].stepc.y = strtof(tmp, NULL);
						tellClients("200 OK STEP Y");
					}
					else bufferevent_write(client->buf_ev,"401 UNKNOWN STEPSIZE\n", sizeof("401 UNKNOWN STEPSIZE\n"));
				}
				else {
					ptc.wfs[0].stepc.y += 1;
					tellClients("200 OK STEP Y +1");
				}
			}
			else bufferevent_write(client->buf_ev,"401 UNKNOWN STEP\n", sizeof("401 UNKNOWN STEP\n"));
		}
		else bufferevent_write(client->buf_ev,"402 STEP REQUIRES ARG\n", sizeof("402 STEP REQUIRES ARG\n"));
	}
	else if (strcmp(tmp,"gain") == 0) {
		if (popword(&msg, tmp) > 0) {
			if (strtof(tmp, NULL) > -5 && strtof(tmp, NULL) < 5) {
				tellClients("200 OK GAIN");
				ptc.wfc[0].gain = strtof(tmp, NULL);
			}
			else bufferevent_write(client->buf_ev,"401 UNKNOWN GAIN\n", sizeof("401 UNKNOWN GAIN\n"));
		}
		else bufferevent_write(client->buf_ev,"402 GAIN REQUIRES ARG\n", sizeof("402 GAIN REQUIRES ARG\n"));
	}
	else if (strcmp(tmp,"calibrate") == 0) {
		if (popword(&msg, tmp) > 0) {

			if (strcmp(tmp,"pinhole") == 0) {
				ptc.mode = AO_MODE_CAL;
				ptc.calmode = CAL_PINHOLE;
				pthread_mutex_lock(&mode_mutex);
				pthread_cond_signal(&mode_cond);
				pthread_mutex_unlock(&mode_mutex);
				tellClients("200 OK CALIBRATE PINHOLE");
//				bufferevent_write(client->buf_ev,"200 OK CALIBRATE PINHOLE\n", sizeof("200 OK CALIBRATE PINHOLE\n"));
			}
			else if (strcmp(tmp,"lintest") == 0) {
				ptc.mode = AO_MODE_CAL;
				ptc.calmode = CAL_LINTEST;
				pthread_mutex_lock(&mode_mutex);
				pthread_cond_signal(&mode_cond);
				pthread_mutex_unlock(&mode_mutex);
				tellClients("200 OK CALIBRATE LINTEST");
//				bufferevent_write(client->buf_ev,"200 OK CALIBRATE LINTEST\n", sizeof("200 OK CALIBRATE LINTEST\n"));
			}
			else if (strcmp(tmp,"influence") == 0) {
				ptc.mode = AO_MODE_CAL;
				ptc.calmode = CAL_INFL;
				pthread_mutex_lock(&mode_mutex);
				pthread_cond_signal(&mode_cond);
				pthread_mutex_unlock(&mode_mutex);	
				tellClients("200 OK CALIBRATE INFLUENCE");
				sleep(1);
//				bufferevent_write(client->buf_ev,"200 OK CALIBRATE INFLUENCE\n", sizeof("200 OK CALIBRATE INFLUENCE\n"));
			}
			else {
				bufferevent_write(client->buf_ev,"401 UNKNOWN CALIBRATION\n", sizeof("400 UNKNOWN CALIBRATION\n"));
			}
		}
		else {
			bufferevent_write(client->buf_ev,"402 CALIBRATE REQUIRES ARG\n", sizeof("400 CALIBRATE REQUIRES ARG\n"));
		}
	}
	else {
		return bufferevent_write(client->buf_ev,"400 UNKNOWN\n", sizeof("400 UNKNOWN\n"));
	}
	
	return EXIT_SUCCESS;
}

int tellClients(char *msg, ...) {
	va_list ap;
	int i;
	char *out, *out2;

	va_start(ap, msg);

	vasprintf(&out, msg, ap);
	asprintf(&out2, "%s\n", out);

//	logDebug("message was: %s length %d and %d", msg, strlen(msg), strlen(msg));
	for (i=0; i < clientlist.nconn; i++)
		if (bufferevent_write(clientlist.connlist[i]->buf_ev, out2, strlen(out2)+1) != 0) return EXIT_FAILURE; // +1 for \0
		
	return EXIT_SUCCESS;
}

int showHelp(const client_t *client, const char *subhelp) {
	if (subhelp == NULL) {
		char help[] = "\
200 OK HELP\n\
help [command]:         help (on a certain command, if available).\n\
mode <mode>:            close or open the loop.\n\
calibrate <mode>:       calibrate a component.\n\
gain <value>:           sets gain for wfc 0 (TT).\n\
step <x|y> [value]:     moves corrected frame of reference\n\
info <wfc|wfs>:         gives information on the wfcs or wfss\n\
exit or quit:           disconnect from daemon.\n\
shutdown:               shutdown the FOAM progra.\n";

		return bufferevent_write(client->buf_ev, help, sizeof(help));
	}
	else if (strcmp(subhelp, "mode") == 0) {
		char help[] = "200 OK HELP MODE\n\
mode <mode>: close or open the loop.\n\
   mode=open: opens the loop and only records what's happening with the AO \n\
        system and does not actually drive anything.\n\
   mode=closed: closes the loop and starts the feedbackloop, correcting the\n\
        wavefront as fast as possible.\n\
   mode=listen: stops looping and waits for input from the users. Basically\n\
        does nothing\n";
		return bufferevent_write(client->buf_ev, help, sizeof(help));
	}
	else if (strcmp(subhelp, "set") == 0) {
		char help[] = "200 OK HELP SET\n\
set <var> <value>\n\
   this changes the value of a run-time variable.\n";
		return bufferevent_write(client->buf_ev, help, sizeof(help));
	}
	else if (strcmp(subhelp, "calibrate") == 0) {
		char help[] = "200 OK HELP CALIBRATE\n\
calibrate <mode>\n\
   mode=pinhole: do a pinhole calibration.\n\
   mode=influence: do a WFC influence matrix calibration.\n";
		return bufferevent_write(client->buf_ev, help, sizeof(help));
	}
	else {
		char help[] = "401 UNKOWN HELP\n";
		return bufferevent_write(client->buf_ev, help, sizeof(help));
	}
	

}

// DOXYGEN GENERAL DOCUMENTATION //
/*********************************/

/* Doxygen needs the following aliases in the configuration file!

ALIASES += authortim="Tim van Werkhoven (T.I.M.vanWerkhoven@phys.uu.nl)"
ALIASES += license="GPL"
ALIASES += wisdomfile="fftw_wisdom.dat"
ALIASES += name="FOAM"
ALIASES += longname="Modular Adaptive Optics Framework"
ALIASES += uilib="foam_ui_library.*"
ALIASES += cslib="foam_cs_library.*"
*/


/*!	\mainpage @name 

	\section aboutdoc About this document
	
	This is the (developer) documentation for @name, the @longname 
	 (yes, @name is backwards). It is intended to clarify the
	code and give some help to people re-using the code for their specific needs.
	
	\section aboutfoam About @name
	
	@name, the @longname, is intended to be a modular
	framework which works independent of hardware and should provide such
	flexibility that the framework can be implemented on any AO system.
	
	A short list of the features of @name follows:
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
	
	@name depends on the following libraries to be present on the system:
	\li \a libevent to be installed on the target machine, which is released under a 3-clause BSD license and is avalaible here:
	 http://www.monkey.org/~provos/libevent/
	\li \a libsdl which is used to display the sensor output
	
	For simulation mode, the following is also required:
	\li \a cfitsio, a library to read (and write) FITS files. Used to read the simulated wavefront.
	\li \a fftw3 which is used to compute FFT's to simulate the SH lenslet array	
	
	Furthermore @name requires basic things like a hosted compilation environment, pthreads, etc. For a full list of dependencies,
	see the header files. @name is however supplied with an (auto-)configure script which checks these
	basic things and tells you what the capabilities of @name on your system will be. 
	
	\subsection drivers Write drivers
	
	You'll have to write your own drivers for all hardware components that 
	adhere to the interfaces described below. The functions will have to be
	defined in the config file, see next section.
	
	\subsection config Configure @name
	
	Configure @name, especially ao_config.cfg. Make sure you do \b not copy the FFTW wisdom file 'fftw_wisdom.dat' to new machines,
	this file contains some simple benchmarking results done by FFTW and are very machine dependent. @name will regenerate the file
	if necessary, but it cannot detect `wrong' wisdom files. Copying bad files is worse than deleting.

	\section network Networking
	
	<info on networking interface>
	200: Succesful reception of command, executing immediately.
	400: General error, something is wrong
	401: Argument is not known
	402: Command is incomplete (missing argument)
	
	\section limit_sec Limitations/bugs
	
	There are some limitations to @name which are discussed in this section. The list includes:

	\li The subaperture resolution must be a multiple of 4,
	\li The configuration file linelength is at max 1024 characters,
	\li Commands given to @name over the socket/network can be at most 1024 characters,
	\li At the moment, most modules work with floats to process data (no bytes or doubles)
	
	Points with the 'at the moment' prefix will hopefully be resolved in the future, other constraints will not be `fixed' because
	these pose no big problems for most to all working setups.
			
*/
