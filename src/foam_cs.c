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

#ifndef FOAM_CONFIG_FILE
#error "FOAM_CONFIG_FILE undefined, please define in foam_cs_config.h"
#endif

// GLOBAL VARIABLES //
/********************/	

// These are defined in foam_cs_library.c
extern control_t ptc;
extern config_t cs_config;
extern conntrack_t clientlist;

// These are used for communication between worker thread and
// networking thread
pthread_mutex_t mode_mutex;
pthread_cond_t mode_cond;

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


	/*! 
	@brief Initialisation function.
	
	\c main() initializes necessary variables, threads, etc. and
	then runs the AO in open-loop mode, from where the user can decide
	what to do.\n
	\n
	The order in which the program is initialized is as follows:
	\li Setup thread mutexes
	\li Setup signal handlers for SIGINT and SIGPIPE
	\li Load configuration using loadConfig() from \c FOAM_CONFIG_FILE
	\li Run modInitModule() such that the modules can initialize themselves
	\li Start a thread which runs sockListen()
	\li Let the other thread start modeListen()
	
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
	
	logInfo(0,"Starting %s (%s) by %s",FOAM_NAME, FOAM_VERSION, FOAM_AUTHOR);

	ptc.starttime = time (NULL);
	loctime = localtime (&ptc.starttime);
	strftime (date, 64, "%A, %B %d %H:%M:%S, %Y (%Z).", loctime);	
	logInfo(0,"at %s", date);
		
	// BEGIN LOADING CONFIG
	if (loadConfig(FOAM_CONFIG_FILE) != EXIT_SUCCESS)
		logErr("Loading configuration failed");

	logInfo(0, "Configuration successfully loaded...");	
	
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
	
	logInfo(0, "Trying to stop modules...");
	modStopModule(&ptc);
	
	logInfo(0, "Stopping threads...");
	pthread_mutex_destroy(&mode_mutex);
	pthread_cond_destroy(&mode_cond);
	// TODO: we need to stop the threads here, not just kill them?
//	pthread_exit(NULL);
	
	logInfo(0, "Stopping FOAM at %s", date);
	logInfo(0, "Ran for %ld seconds and parsed %ld frames (framerate: %f).", \
		end-ptc.starttime, ptc.frames, ptc.frames/(float) (end-ptc.starttime));

	if (cs_config.infofd) fclose(cs_config.infofd);
	if (cs_config.errfd) fclose(cs_config.errfd);
	if (cs_config.debugfd) fclose(cs_config.debugfd);
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
		if (ptc.wfs == NULL)
			logErr("Failed to allocate ptc.wfs");
		
		// initialize some things to zero
		for (i=0; i<ptc.wfs_count; i++) {
			ptc.wfs[i].singular = NULL;
			ptc.wfs[i].dmmodes = NULL;
			ptc.wfs[i].wfsmodes = NULL;
			ptc.wfs[i].stepc.x = 0;
			ptc.wfs[i].stepc.y = 0;
		}
		
		logInfo(0, "WFS_COUNT initialized: %d", ptc.wfs_count);
	}
	else if (strcmp(var, "WFC_COUNT") == 0) {
		ptc.wfc_count = (int) strtol(value, NULL, 10);
		ptc.wfc = calloc(ptc.wfc_count, sizeof(*ptc.wfc));
		
		if (ptc.wfc == NULL)
			logErr("Failed to allocate ptc.wfc");

		logInfo(0, "WFC_COUNT initialized: %d", ptc.wfc_count);
	}
	else if (strstr(var, "WFC_NAME") != NULL) {
		if (issetWFC(var) != EXIT_SUCCESS) return EXIT_FAILURE;
		if ((tmp = validWFC(var)) < 0) return EXIT_FAILURE;
				
		strncpy(ptc.wfc[tmp].name, value, (size_t) FILENAMELEN);
		ptc.wfc[tmp].name[FILENAMELEN-1] = '\0'; // TODO: This might not be necessary
		
		logInfo(0, "WFC_NAME initialized for WFC %d: %s", tmp, ptc.wfc[tmp].name);
	}
	else if (strstr(var, "WFC_TYPE") != NULL) {
		if (issetWFC(var) != EXIT_SUCCESS) return EXIT_FAILURE;
		if ((tmp = validWFC(var)) < 0) return EXIT_FAILURE;
		
		ptc.wfc[tmp].type = (int) strtol(value, NULL, 10);
//		strncpy(ptc.wfc[tmp].name, value, (size_t) FILENAMELEN);
//		ptc.wfc[tmp].name[FILENAMELEN-1] = '\0'; // TODO: This might not be necessary
		
		logInfo(0, "WFC_TYPE initialized for WFC %d: %d", tmp, ptc.wfc[tmp].type);
	}
    else if (strstr(var, "WFC_NACT") != NULL) {
		if (issetWFC(var) != EXIT_SUCCESS) return EXIT_FAILURE;
		if ((tmp = validWFC(var)) < 0) return EXIT_FAILURE;

		// Get the number of actuators for which WFC?
		// tmp = strtol(strstr(var,"[")+1, NULL, 10);
		ptc.wfc[tmp].nact = strtol(value, NULL, 10);
		ptc.wfc[tmp].ctrl = gsl_vector_float_calloc(ptc.wfc[tmp].nact);
		if (ptc.wfc[tmp].ctrl == NULL) return EXIT_FAILURE;

		logInfo(0, "WFS_NACT initialized for WFS %d: %d", tmp, ptc.wfc[tmp].nact);
    }
    else if (strstr(var, "WFC_GAIN") != NULL) {
		if (issetWFC(var) != EXIT_SUCCESS) return EXIT_FAILURE;
		if ((tmp = validWFC(var)) < 0) return EXIT_FAILURE;
		
		ptc.wfc[tmp].gain = strtof(value, NULL);

		logInfo(0, "WFC_GAIN initialized for WFS %d: %f", tmp, ptc.wfc[tmp].gain);
    }
	else if (strstr(var, "WFS_NAME") != NULL) {
		if (issetWFS(var) != EXIT_SUCCESS) return EXIT_FAILURE;
		if ((tmp = validWFS(var)) < 0) return EXIT_FAILURE;
		
		strncpy(ptc.wfs[tmp].name, value, (size_t) FILENAMELEN);
		ptc.wfs[tmp].name[FILENAMELEN-1] = '\0'; // This might not be necessary
				
		logInfo(0, "WFS_NAME initialized for WFS %d: %s", tmp, ptc.wfs[tmp].name);
	}
	else if (strstr(var, "WFS_DF") != NULL) {
		if (issetWFS(var) != EXIT_SUCCESS) return EXIT_FAILURE;
		if ((tmp = validWFS(var)) < 0) return EXIT_FAILURE;

		strncpy(ptc.wfs[tmp].darkfile, value, (size_t) FILENAMELEN);
		ptc.wfs[tmp].darkfile[FILENAMELEN-1] = '\0'; // This might not be necessary
		
		logInfo(0, "WFS_DF initialized for WFS %d: %s", tmp, ptc.wfs[tmp].darkfile);
	}
	else if (strstr(var, "WFS_SKY") != NULL) {
		if (issetWFS(var) != EXIT_SUCCESS) return EXIT_FAILURE;
		if ((tmp = validWFS(var)) < 0) return EXIT_FAILURE;

		strncpy(ptc.wfs[tmp].skyfile, value, (size_t) FILENAMELEN);
		ptc.wfs[tmp].skyfile[FILENAMELEN-1] = '\0'; // This might not be necessary
		
		logInfo(0, "WFS_SKY initialized for WFS %d: %s", tmp, ptc.wfs[tmp].skyfile);
	}
	else if (strstr(var, "WFS_PINHOLE") != NULL) {
		if (issetWFS(var) != EXIT_SUCCESS) return EXIT_FAILURE;
		if ((tmp = validWFS(var)) < 0) return EXIT_FAILURE;

		strncpy(ptc.wfs[tmp].pinhole, value, (size_t) FILENAMELEN);
		ptc.wfs[tmp].pinhole[FILENAMELEN-1] = '\0'; // This might not be necessary
		
		logInfo(0, "WFS_PINHOLE initialized for WFS %d: %s", tmp, ptc.wfs[tmp].pinhole);
	}
	else if (strstr(var, "WFS_INFL") != NULL) {
		if (issetWFS(var) != EXIT_SUCCESS) return EXIT_FAILURE;
		if ((tmp = validWFS(var)) < 0) return EXIT_FAILURE;

		strncpy(ptc.wfs[tmp].influence, value, (size_t) FILENAMELEN);
		ptc.wfs[tmp].influence[FILENAMELEN-1] = '\0'; // This might not be necessary
		
		logInfo(0, "WFS_INFL initialized for WFS %d: %s", tmp, ptc.wfs[tmp].influence);
	}
	else if (strstr(var, "WFS_FF") != NULL) {
		if (issetWFS(var) != EXIT_SUCCESS) return EXIT_FAILURE;
		if ((tmp = validWFS(var)) < 0) return EXIT_FAILURE;

		strncpy(ptc.wfs[tmp].flatfile, value, (size_t) FILENAMELEN);
		ptc.wfs[tmp].flatfile[FILENAMELEN-1] = '\0'; // This might not be necessary

		logInfo(0, "WFS_FF initialized for WFS %d: %s", tmp, ptc.wfs[tmp].flatfile);
	}
	else if (strstr(var, "WFS_CELLS") != NULL){
		if (issetWFS(var) != EXIT_SUCCESS) return EXIT_FAILURE;
		if ((tmp = validWFS(var)) < 0) return EXIT_FAILURE;
		
		if (strstr(value,"{") == NULL || strstr(value,"}") == NULL || strstr(value,",") == NULL) 
			return EXIT_FAILURE; // return if we don't have the {x,y} syntax

		ptc.wfs[tmp].cells[0] = strtol(strtok(value,"{,}"), NULL, 10);
		ptc.wfs[tmp].cells[1] = strtol(strtok(NULL ,"{,}"), NULL, 10);
		
		if (ptc.wfs[tmp].cells[0] % 2 != 0 || ptc.wfs[tmp].cells[1] % 2 != 0)
			logErr("WFS %d has an odd cell-resolution (%dx%d), not supported. Please only use 2nx2n cells.", \
				tmp, ptc.wfs[tmp].cells[0], ptc.wfs[tmp].cells[1]);
		
		ptc.wfs[tmp].subc = calloc(ptc.wfs[tmp].cells[0] * ptc.wfs[tmp].cells[1], sizeof(*ptc.wfs[tmp].subc));
		ptc.wfs[tmp].gridc = calloc(ptc.wfs[tmp].cells[0] * ptc.wfs[tmp].cells[1], sizeof(*ptc.wfs[tmp].gridc));
		
		ptc.wfs[tmp].refc = gsl_vector_float_calloc(ptc.wfs[tmp].cells[0] * ptc.wfs[tmp].cells[1] * 2);
		ptc.wfs[tmp].disp = gsl_vector_float_calloc(ptc.wfs[tmp].cells[0] * ptc.wfs[tmp].cells[1] * 2);
		if (ptc.wfs[tmp].subc == NULL || ptc.wfs[tmp].refc == NULL || ptc.wfs[tmp].disp == NULL)
			logErr("Cannot allocate memory for tracker window coordinates, or other tracking vectors.");
		
		if (ptc.wfs[tmp].res.x * ptc.wfs[tmp].res.y <= 0)
			logErr("Cannot initialize WFS_CELLS before WFS_RES");
		
		ptc.wfs[tmp].shsize[0] = (ptc.wfs[tmp].res.x / ptc.wfs[tmp].cells[0]);
		ptc.wfs[tmp].shsize[1] = (ptc.wfs[tmp].res.y / ptc.wfs[tmp].cells[1]);
		ptc.wfs[tmp].refim = calloc(ptc.wfs[tmp].shsize[0] * \
			ptc.wfs[tmp].shsize[1], sizeof(ptc.wfs[tmp].refim));
		if (ptc.wfs[tmp].refim == NULL)
			logErr("Failed to allocate image memory for reference image.");

		logInfo(0, "WFS_CELLS initialized for WFS %d: (%dx%d). Subapt resolution is (%dx%d) pixels", \
			tmp, ptc.wfs[tmp].cells[0], ptc.wfs[tmp].cells[1], ptc.wfs[tmp].shsize[0], ptc.wfs[tmp].shsize[1]);
	}
	else if (strstr(var, "WFS_RES") != NULL){
		if (issetWFS(var) != EXIT_SUCCESS) return EXIT_FAILURE;
		if ((tmp = validWFS(var)) < 0) return EXIT_FAILURE;
		
		if (strstr(value,"{") == NULL || strstr(value,"}") == NULL || strstr(value,",") == NULL) 
			return EXIT_FAILURE;

		ptc.wfs[tmp].res.x = strtol(strtok(value,"{,}"), NULL, 10);
		ptc.wfs[tmp].res.y = strtol(strtok(NULL ,"{,}"), NULL, 10);
		
		if (ptc.wfs[tmp].res.x % 2 != 0 || ptc.wfs[tmp].res.y % 4 != 0)
			logErr("WFS %d has an odd resolution (%dx%d), not supported. Please only use 2nx2n pixels.", \
				tmp, ptc.wfs[tmp].res.x, ptc.wfs[tmp].res.y);
		
		// Allocate memory for all images we need lateron
		ptc.wfs[tmp].image = calloc(ptc.wfs[tmp].res.x * ptc.wfs[tmp].res.y, sizeof(ptc.wfs[tmp].image));
		ptc.wfs[tmp].darkim = calloc(ptc.wfs[tmp].res.x * ptc.wfs[tmp].res.y, sizeof(ptc.wfs[tmp].darkim));
		ptc.wfs[tmp].flatim = calloc(ptc.wfs[tmp].res.x * ptc.wfs[tmp].res.y, sizeof(ptc.wfs[tmp].flatim));
		ptc.wfs[tmp].corrim = calloc(ptc.wfs[tmp].res.x * ptc.wfs[tmp].res.y, sizeof(ptc.wfs[tmp].corrim));
		
		// check if everything worked out ok
		if ((ptc.wfs[tmp].image == NULL) ||
				(ptc.wfs[tmp].darkim == NULL) ||
				(ptc.wfs[tmp].flatim == NULL) ||
				(ptc.wfs[tmp].corrim == NULL))
			logErr("Failed to allocate image memory (image, dark, flat, corrected).");
		
		logInfo(0, "WFS_RES initialized for WFS %d: %d x %d", tmp, ptc.wfs[tmp].res.x, ptc.wfs[tmp].res.y);
	}

	else if (strcmp(var, "CS_LISTEN_IP") == 0) {
		strncpy(cs_config.listenip, value, 16);
		
		logInfo(0, "CS_LISTEN_IP initialized: %s", cs_config.listenip);
	}
	else if (strcmp(var, "CS_LISTEN_PORT") == 0) {
		cs_config.listenport = (int) strtol(value, NULL, 10);
		
		logInfo(0, "CS_LISTEN_PORT initialized: %d", cs_config.listenport);
	}
	else if (strcmp(var, "CS_USE_SYSLOG") == 0) {
		cs_config.use_syslog = ((int) strtol(value, NULL, 10) == 0) ? false : true;
		
		logInfo(0, "CS_USE_SYSLOG initialized: %d", cs_config.use_syslog);
	}
	else if (strcmp(var, "CS_USE_STDOUT") == 0) {
		cs_config.use_stdout = ((int) strtol(value, NULL, 10) == 0) ? false : true;
		
		logInfo(0, "CS_USE_STDERR initialized: %d", cs_config.use_stdout);
	}
	else if (strcmp(var, "CS_INFOFILE") == 0) {
		strncpy(cs_config.infofile,value, (size_t) FILENAMELEN);
		cs_config.infofile[FILENAMELEN-1] = '\0'; // TODO: is this necessary?
		logInfo(0, "CS_INFOFILE initialized: %s", cs_config.infofile);
	}
	else if (strcmp(var, "CS_ERRFILE") == 0) {
		strncpy(cs_config.errfile,value, (size_t) FILENAMELEN);
		cs_config.errfile[FILENAMELEN-1] = '\0'; // TODO: is this necessary?
		logInfo(0, "CS_ERRFILE initialized: %s", cs_config.errfile);
	}
	else if (strcmp(var, "CS_DEBUGFILE") == 0) {
		strncpy(cs_config.debugfile, value, (size_t) FILENAMELEN);
		cs_config.debugfile[FILENAMELEN-1] = '\0'; // TODO: is this necessary?
		logInfo(0, "CS_DEBUGFILE initialized: %s", cs_config.debugfile);
	}

	return EXIT_SUCCESS;
}

int loadConfig(char *file) {
	logDebug(0, "Reading configuration from file: %s",file);
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
		
		logDebug(0, "Parsing '%s' '%s' settings pair.", var, value);
		
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
		logInfo(0, "Syslog successfully initialized.");
	}
	return EXIT_SUCCESS;
}

int initLogFiles() {
	if (strlen(cs_config.infofile) > 0) {
		if ((cs_config.infofd = fopen(cs_config.infofile,"a")) == NULL) {
			logWarn("Unable to open file %s for info-logging! Not using this logmethod!", cs_config.infofile);
			cs_config.infofile[0] = '\0';
		}	
		else logInfo(0, "Info logfile '%s' successfully opened.", cs_config.infofile);
	}
	else
		logInfo(0, "Not logging general info to disk.");

	if (strlen(cs_config.errfile) > 0) {
		if (strcmp(cs_config.errfile, cs_config.infofile) == 0) {	// If the errorfile is the same as the infofile, use the same FD
			cs_config.errfd = cs_config.infofd;
			logDebug(0, "Using the same file '%s' for info- and error- logging.", cs_config.errfile);
		}
		else if ((cs_config.errfd = fopen(cs_config.errfile,"a")) == NULL) {
			logWarn("Unable to open file %s for error-logging! Not using this logmethod!", cs_config.errfile);
			cs_config.errfile[0] = '\0';
		}
		else logInfo(0, "Error logfile '%s' successfully opened.", cs_config.errfile);
	}
	else {
		logInfo(0, "Not logging errors to disk.");
	}


	if (strlen(cs_config.debugfile) > 0) {
		if (strcmp(cs_config.debugfile,cs_config.infofile) == 0) {
			cs_config.debugfd = cs_config.infofd;	
			logDebug(0, "Using the same file '%s' for debug- and info- logging.", cs_config.debugfile);
		}
		else if (strcmp(cs_config.debugfile,cs_config.errfile) == 0) {
			cs_config.debugfd = cs_config.errfd;	
			logDebug(0, "Using the same file '%s' for debug- and error- logging.", cs_config.debugfile);
		}
		else if ((cs_config.debugfd = fopen(cs_config.debugfile,"a")) == NULL) {
			logWarn("Unable to open file %s for debug-logging! Not using this logmethod!", cs_config.debugfile);
			cs_config.debugfile[0] = '\0';
		}
		else logInfo(0, "Debug logfile '%s' successfully opened.", cs_config.debugfile);
	}
	else {
		logInfo(0, "Not logging debug to disk.");
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
	logInfo(0, "Entering open loop.");

	if (ptc.wfs_count == 0) {				// we need wave front sensors
		logWarn("Error, no WFSs defined.");
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
	
	while (ptc.mode == AO_MODE_OPEN) {
		ptc.frames++;								// increment the amount of frames parsed
		// logInfo(0, "Entering open loop.");
		if (modOpenLoop(&ptc) != EXIT_SUCCESS) {
			logWarn("modOpenLoop failed");
			ptc.mode = AO_MODE_LISTEN;
			return;
		}
	}
	
	return; // mode is not open anymore, decide what to to next in modeListen
}

void modeClosed() {	
	logInfo(0, "Entering closed loop.");

	if (ptc.wfs_count == 0) {						// we need wave front sensors
		logWarn("Error, no WFSs defined.");
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
		
		if (modClosedLoop(&ptc) != EXIT_SUCCESS) {
			logWarn("modClosedLoop failed");
			ptc.mode = AO_MODE_LISTEN;
			return;
		}							
		ptc.frames++;								// increment the amount of frames parsed
	}
	
	return;					// back to modeListen (or where we came from)
}

void modeCal() {
	logInfo(0, "Starting Calibration");
	
	// this links to a module
	if (modCalibrate(&ptc) != EXIT_SUCCESS) {
		logWarn("modCalibrate failed");
		tellClients("404 ERROR CALIBRATION FAILED");
		ptc.mode = AO_MODE_LISTEN;
		return;
	}
	
	logInfo(0, "Calibration loop done, switching to listen mode");
	tellClients("201 CALIBRATION SUCCESSFUL");
	ptc.mode = AO_MODE_LISTEN;
		
	return;
}

void modeListen() {
	
	while (true) {
		logInfo(0, "Now running in listening mode.");
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
	
	logDebug(0, "Starting listening socket on %s:%d.", cs_config.listenip, cs_config.listenport);
	
	// Initialize the internet socket. We want streaming and we want TCP
	if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0)
		logErr("Failed to set up socket.");
		
	// Get the address fixed:
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(cs_config.listenip);
	serv_addr.sin_port = htons(cs_config.listenport);
	memset(serv_addr.sin_zero, '\0', sizeof(serv_addr.sin_zero));

	// Set reusable and nosigpipe flags so we don't get into trouble later on. TODO: doesn't work?
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) != 0)
		logWarn("Could not set socket flag SO_REUSEADDR.");
	
	// We now actually bind the socket to something
	if (bind(sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) != 0)
		logErr("Binding socket failed: %s", strerror(errno));
	
	if (listen (sock, 1) != 0) 
		logErr("Listen failed: %s", strerror(errno));
		
	// Set socket to non-blocking mode, nice for events
	if (setnonblock(sock) != 0) 
		logWarn("Coult not set socket to non-blocking mode, might cause undesired side-effects.");
		
	logInfo(0, "Successfully initialized socket on %s:%d, setting up event schedulers.", cs_config.listenip, cs_config.listenport);
	
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
	
//	logDebug(0, "Handling new client connection.");
	
	newsock = accept(sock, (struct sockaddr *) &cli_addr, &cli_len);
		
	if (newsock < 0) 
		logWarn("Accepting socket failed: %s!", strerror(errno));
	
	if (setnonblock(newsock) < 0)
		logWarn("Unable to set new client socket to non-blocking.");

//	logDebug(0, "Accepted & set to nonblock.");
	
	// Check if we do not exceed the maximum number of connections:
	if (clientlist.nconn >= MAX_CLIENTS) {
		sleep(1);
		close(newsock);
		logWarn("Refused connection, maximum clients reached (%d)", MAX_CLIENTS);
		return;
	}
	
	client = calloc(1, sizeof(*client));
	if (client == NULL)
		logErr("Calloc failed in sockAccept.");

	client->fd = newsock;			// Add socket to the client struct
	client->buf_ev = bufferevent_new(newsock, sockOnRead, sockOnWrite, sockOnErr, client);

	clientlist.nconn++;
	for (i=0; clientlist.connlist[i] != NULL && i < MAX_CLIENTS; i++) 
		;	// Run through array until we find an empty connlist entry

	client->connid = i;
	clientlist.connlist[i] = client;

	// We have to enable it before our callbacks will be
	if (bufferevent_enable(client->buf_ev, EV_READ) != 0)
		logErr("Failed to enable buffered event.");

	logInfo(0, "Succesfully accepted connection from %s (using sock %d and buf_ev %p)", \
		inet_ntoa(cli_addr.sin_addr), newsock, client->buf_ev);
	
	bufferevent_write(client->buf_ev,"200 OK CONNECTION ESTABLISHED\n", sizeof("200 OK CONNECTION ESTABLISHED\n"));

}

void sockOnErr(struct bufferevent *bev, short event, void *arg) {
	client_t *client = (client_t *)arg;

	if (event & EVBUFFER_EOF) 
		logInfo(0, "Client successfully disconnected.");
	else
		logWarn("Client socket error, disconnecting.");
		
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
	
	while ((nbytes = bufferevent_read(bev, msg, (size_t) COMMANDLEN-1)) == COMMANDLEN-1) {// detect very long messages
		logErr("Received very long command over socket which was ignored.");
		bufferevent_write(client->buf_ev,"400 COMMAND TOO LONG (MAX: COMMANDLEN)\n", sizeof("400 COMMAND TOO LONG (MAX: COMMANDLEN)\n"));
		return;
	}
		
	msg[nbytes] = '\0';
	if ((tmp = strchr(msg, '\n')) != NULL) // there might be a trailing newline which we don't want
		*tmp = '\0';
	if ((tmp = strchr(msg, '\r')) != NULL) // there might be a trailing newline which we don't want
		*tmp = '\0';
	
	logDebug(0, "Received %d bytes on socket reading: '%s'.", nbytes, msg);
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
	if (begin > 0)// Trimming string begin %d chars", begin
		*msg = *msg+begin;
	
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
	char tmp[len+1];
	tmp[0] = '\0';
		
	popword(&msg, tmp);
		

	if (strcmp(tmp,"help") == 0) {
		// Show the help command
		if (popword(&msg, tmp) > 0) // Does the user want help on a specific command?
			showHelp(client, tmp);
		else
			showHelp(client, NULL);			
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

//	logDebug(0, "message was: %s length %d and %d", msg, strlen(msg), strlen(msg));
	for (i=0; i < clientlist.nconn; i++) {
		if (bufferevent_write(clientlist.connlist[i]->buf_ev, out2, strlen(out2)+1) != 0) {
			logWarn("Error telling client %d", i);
			return EXIT_FAILURE; 
		}
	}
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
	code and give some help to people re-using or implementing the code for their specific needs.
	
	\section aboutfoam About @name
	
	@name, the @longname, is intended to be a modular
	framework which works independent of hardware and should provide such
	flexibility that the framework can be implemented on any AO system.
	
	A short list of the features of @name follows:
	\li Portable - It will run on many (Unix) systems and is hardware independent
	\li Scalable - It  will scale easily and handle multiple cores/CPU's
	\li Usable - It will be controllable over a network by multiple clients simultaneously
	\li Simulation - It will be able to simulate the whole AO system
	\li MCAO - It will support multiple correctors and sensors simultaneously
	\li Offloading - It will be able to offload/distribute wavefront correction over several correctors
	\li Stepping - It will allow for stepping over an image

	
	For more information, see the FOAM wiki at http://www.astro.uu.nl/~astrowik/astrowiki/index.php/FOAM or the documentation
	on http://www.phys.uu.nl/~0315133/foam/docs/ .
	
	\section struct @name structure
	
	In this section, the structure of @name will be clarified. First, some key concepts are explained which are used within @name.
	
	@name uses several concepts, for which it is useful to have names. First of all, the complete set of files downloadable
	from for example http://www.phys.uu.nl/~0315133/foam/tags/ is simply called a \e `distribution', or \e `version' or just \e @name.
	
	Within the distribution itself, there are two parts of @name worth mentioning. These are the Control Software (CS) and the
	User Interface (UI). Currently, the UI is not being developed actively, as telnet works just as well (for now). This explains
	the \c `_cs' and \c `_ui' prefix in some filenames.
	
	The Control Software consists of three parts:
	\li The \e framework itself, in \c foam_cs.c
	\li \e Prime \e modules, in \c foam_primemod-*
	\li \e Modules, in \c foam_modules-*
	
	To get a running program, the framework, a prime module and zero or more modules are combined into one executable. This
	program is called a \e `package' for obvious reasons. 
	
	By default a dummy package is supplied, which take the framework
	and the dummy prime module (which does nothing) which results in the most basic package possible. A more interesting
	package, the simulation package, is also provided. This package simulates a complete AO system and can be used to test 
	new routines (center-of-gravity tracking, correlation tracking, load distribution) in a reproducible fashion.
	
	\subsection struct-frame The Framework
	
	The framework itself does very little. It reads the configuration into memory and if necessary allocates memory for
	control vectors, images and what not. After that it provides a hook for prime modules to initialize themselves. Immediately
	after that, it splits in a worker thread which does the actual AO calculations, and in a networking thread which
	listens for connections on a TCP socket. The worker thread starts by running in listening mode, where it waits for 
	commands coming from clients connected over the network. Communication between the two threads is done with mutexes.
	
	The framework provides the following hooks to prime modules:	
	\li int modInitModule(control_t *ptc);
	\li void modStopModule(control_t *ptc);
	\li int modOpenInit(control_t *ptc);
	\li int modOpenLoop(control_t *ptc);
	\li int modClosedInit(control_t *ptc);
	\li int modClosedLoop(control_t *ptc);
	\li int modCalibrate(control_t *ptc);

	\subsection struct-prime The Prime Modules
	
	Prime modules must define the functions defined in the above list. The prime module should tell what these hooks do,
	e.g. what happens during open loop, closed loop and during calibration. These functions can be just placeholders,
	which can be seen in \c foam_primemod-dummy.c , or more complex functions which actually do something.
	
	Almost always, prime modules link to modules in turn, by simply including their header files and compiling
	the source files along with the prime module. The modules contain the functions which do the hard work,
	and are divided into seperate files depending on the functionality of the routines.
	
	\subsection struct-mod The Modules
	
	Modules provide functions which can be used by other modules or by prime modules. If a module uses routines from
	another module, this module depends on the other module. This is for example the case between the `sim' module
	which needs to read in PGM files and therefore depends on the `pgm' module.
	
	A few default modules are provided with the distribution, and their use can be can be read at the top of the files.

	\section install_sec Installation
	
	@name currently depends on the following libraries to be present on the system:
	\li \c libevent used to handle networking with several simultaneous connections,
	\li \c libsdl which is used to display the sensor output,
	\li \c libpthread used to seperate functions over thread and distribute load,
	\li \c libgsl used to do singular value decomposition, link to BLAS and various other matrix/vector operation.
	
	For simulation mode, the following is also required:
	\li \c cfitsio a library to read (and write) FITS files. Used to read the simulated wavefront,
	\li \c fftw3 which is used to compute FFT's to simulate the SH lenslet array,
	\li \c SDL_Image used to read PGM files.
	
	Furthermore @name requires basic things like a hosted compilation environment, a compiler etc. For a full list of dependencies,
	see the header files. @name is however supplied with an (auto-)configure script which checks these
	basic things and tells you what the capabilities of @name on your system will be.
	
	To install @name, follow these simple steps:
	\li Download a @name release from http://www.phys.uu.nl/~0315133/foam/tags/
	\li Extract the tarball
	\li Run `autoreconf -s -i` which generates the configure script
	\li Run `./configure` to check if your system will support @name. If not, configure will tell you so
	\li Run `make` which makes the various @name packages
	\li Optionally edit config/ao_config.cfg and src/foam_cs_config.h to match your needs
	\li cd into src/, run any of the executables
	\li Connect to localhost:10000 (default) with a telnet client
	\li Type `help' to get a list of @name commands
	
	\subsection config Configure @name
	
	Configure @name, especially \c ao_config.cfg. Make sure you do \b not copy the FFTW wisdom file \c 'fftw_wisdom.dat' to new machines,
	this file contains some simple benchmarking results done by FFTW and are very machine dependent. @name will regenerate the file
	if necessary, but it cannot detect `wrong' wisdom files. Copying bad files is worse than deleting.
	
	\section drivers Developing Packages

	@name itself does not do a lot (run ./foamcs-dummy to verify), it basically provides a framework to which
	modules can be attached. Fortunately, this approach allows for complex bundles of modules, or `packages', as explained
	previously in \ref struct.
	
	If you want to use @name in a specific setup, you'll probably have to program some modules yourself. To do this,
	start with a `prime module' which \b must contain the functions listed in \ref struct-frame 
	(see foam_primemod-dummy.c for example).
		
	These functions provide hooks for the package to work with, and if their meaning is not immediately clear, the documentation
	provides some more details on what these functions do. It is also wise to look at the control_t struct which is used 
	throughout @name to store data, settings and other information.
	
	Once these functions are defined, you can link the prime module to modules already supplied with @name. Simply do this
	by including the header files of the specific modules you want to add. For information on what certain modules do, look
	at the documentation of the files. This documentation tells you what the module does, what functions are available, and
	what other modules this module possibly depends on.
	
	If the default set of modules is insufficient to build a package in your situation, you will have to write your own. This
	scenario is (unfortunately) very likely, because @name does not provide modules to read out all possible framegrabbers/
	cameras and cannot drive all possible filter wheels, tip-tilt mirrors, deformable mirrors etc. If you have written your
	own module, please e-mail it to me (@authortim) so I can add it to the complete distribution.
	
	\subsection ownmodule Write your own modules
	
	A module in @name can take any form, but to keep things at least slightly organised, some conventions apply. These guidelines
	include:
	
	\li Seperate functionally different routines in different modules,
	\li Name your modules `foam_modules-\<modulename\>.c' and supply a header file,
	\li Include doxygen compatible information on the module in the C file (see foam_modules-sh.c),
	\li Include doxygen compatible information on the routines in the header file (see foam_modules-sh.h),
	\li Prefix functions that are used elsewhere with `mod', and hardware related functions with `drv'.
	
	These guidelines are also used in the existing modules so you can easily see what a module does and what functions you have
	to call to get the module working.
	
	\subsection ownmake Adapt the Makefile
	
	Once you have a package, you will need to edit Makefile.am in the src/ directory. But before that, you will need to edit
	the configure.ac script in the root directory. This is necessary so that at configure time, users can decide what packages
	to build or not to build, using --enable-\<package\>. In the configure.ac file, you will need to add two parts:
	\li Add a check to see if the user wants to build the package or not (see 'TEST FOR USER INPUT')
	\li Add a few lines to check if extra libraries necessary for the package are present (see 'ITIFG MODE')

	(TODO: simplify configuration, seperate packages in different files)
	
	After editing the configure.ac script, the Makefile.am needs to know that we want to build a new package/executable. For an
	example on how to do this, see the few lines following '# did we enable UI support or not?'. Basically: check if the package 
	must be built, if yes, add package to bin_PROGRAMS, and setup various \c _SOURCES, \c _CFLAGS and \c _LDFLAGS variables.
	
	\section network Networking

	Currently, the following status codes are used, based loosely on the HTTP status codes. All
	codes are 3 bytes, followed by a space, and end with a single newline. The length of the message
	can vary.
	
	2xx codes are given upon success:
	\li 200: Succesful reception of command, executing immediately,
	\li 201: Executing immediately command succeeded.
	
	4xx codes are errors:
	\li 400: General error, something is wrong
	\li 401: Argument is not known
	\li 402: Command is incomplete (missing argument)
	\li 403: Command given is not allowed at this stage
	\li 404: Previously given and acknowledged command failed.
	
	\section limit_sec Limitations/bugs
	
	There are some limitations to @name which are discussed in this section. The list includes:

	\li The subaperture resolution must be a multiple of 4,
	\li The configuration file linelength is at max 1024 characters,
	\li Commands given to @name over the socket/network can be at most 1024 characters,
	\li At the moment, most modules work with floats to process data (no chars or doubles)
	\li Bug: Feedback to clients connected seems to fail sometimes
	\li Bug: On OS X, SDL does not appear to behave nicely, especially during shutdown
	
	Points with the `at the moment' prefix will hopefully be resolved in the future, other constraints will not be `fixed' because
	these pose no big problems for most to all working setups.

*/
