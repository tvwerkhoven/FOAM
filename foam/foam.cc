/*
 Copyright (C) 2008-2009 Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)
 
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
	@file foam.cc
	@author Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)

	@brief This is the main file for FOAM.

	This is the framework for FOAM, it provides basic functionality and can be 
	customized through the use of certains `hooks'. These hooks are functions that
	are called at crucial moments during the adaptive optics controlling such 
	that the person implementing FOAM on a specific AO setup can customize
	what FOAM does.
 
*/

// HEADERS //
/***********/

#include "foam.h"
#include "types.h"
#include "io.h"
#include "protocol.h"


// GLOBAL VARIABLES //
/********************/	

using namespace std;

// Global AO and FOAM configuration 
control_t *ptc;
config_t *cs_config;

// Inter-thread communication
pthread_mutex_t mode_mutex;
pthread_cond_t mode_cond;
static pthread_attr_t attr;

// Server network functions
static Protocol::Server *protocol = 0;
// Message output and logging 
Io *io;

// PROTOTYPES //
/**************/	

// These come from prime modules, these MUST be defined there
extern void modStopModule(control_t *ptc);
extern int modInitModule(control_t *ptc, config_t *cs_config);
extern int modPostInitModule(control_t *ptc, config_t *cs_config);

extern int modOpenInit(control_t *ptc);
extern int modOpenLoop(control_t *ptc);
extern int modOpenFinish(control_t *ptc);

extern int modClosedInit(control_t *ptc);
extern int modClosedLoop(control_t *ptc);
extern int modClosedFinish(control_t *ptc);

extern int modCalibrate(control_t *ptc);
extern int modMessage(control_t *ptc, const client_t *client, char *list[], const int count);

	/*! 
	@brief Initialisation function.
	
	\c main() initializes necessary variables, threads, etc. and
	then runs the AO in open-loop mode, from where the user can decide
	what to do.\n
	\n
	The order in which the program is initialized is as follows:
	\li Setup thread mutexes
	\li Setup signal handlers for SIGINT and SIGPIPE
	\li Run modInitModule() such that the prime modules can initialize
	\li Start a thread which runs startThread()
	\li Run modPostInitModule() such that the prime modules can initialize after threading (useful for SDL)
	\li Let the other thread start sockListen()
	
	@return \c EXIT_FAILURE on failure, \c EXIT_SUCESS on successful completion.
	*/
int main(int argc, char *argv[]) {
	// INIT VARS // 
	/*************/
	io = new Io(4);
	
  ptc = (control_t*) calloc(1, sizeof(*ptc));
  cs_config = (config_t*) calloc(1, sizeof(*cs_config));
  
	if (pthread_mutex_init(&mode_mutex, NULL) != 0)
		io->msg(IO_ERR, "pthread_mutex_init failed.");
	if (pthread_cond_init (&mode_cond, NULL) != 0)
		io->msg(IO_ERR, "pthread_cond_init failed.");
	
	// we use this to block signals in threads
	// see http://www.opengroup.org/onlinepubs/009695399/functions/sigprocmask.html
	static sigset_t signal_mask;
	
	
	// BEGIN FOAM //
	/**************/
	ptc->starttime = time(NULL);
	struct tm *loctime = localtime(&(ptc->starttime));
	char date[64];
	strftime (date, 64, "%A, %B %d %H:%M:%S, %Y (%Z).", loctime);	
	
	io->msg(IO_NOID|IO_INFO, "      ___           ___           ___           ___     \n\
     /\\  \\         /\\  \\         /\\  \\         /\\__\\    \n\
    /::\\  \\       /::\\  \\       /::\\  \\       /::|  |   \n\
   /:/\\:\\  \\     /:/\\:\\  \\     /:/\\:\\  \\     /:|:|  |   \n\
  /::\\~\\:\\  \\   /:/  \\:\\  \\   /::\\~\\:\\  \\   /:/|:|__|__ \n\
 /:/\\:\\ \\:\\__\\ /:/__/ \\:\\__\\ /:/\\:\\ \\:\\__\\ /:/ |::::\\__\\\n\
 \\/__\\:\\ \\/__/ \\:\\  \\ /:/  / \\/__\\:\\/:/  / \\/__/~~/:/  /\n\
      \\:\\__\\    \\:\\  /:/  /       \\::/  /        /:/  / \n\
       \\/__/     \\:\\/:/  /        /:/  /        /:/  /  \n\
                  \\::/  /        /:/  /        /:/  /   \n\
                   \\/__/         \\/__/         \\/__/ \n");

	io->msg(IO_INFO,"Starting %s (%s) at %s", PACKAGE_NAME, PACKAGE_VERSION, date);
	io->msg(IO_INFO,"Copyright 2007-2009 Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)");
	
	// Read configuration 
	

	// INITIALIZE MODULES //
	/**********************/
	
	io->msg(IO_INFO,"Initializing modules...");
	// this routine will populate ptc and possibly 
	// adapt cs_config-> changes will be processed below
	modInitModule(ptc, cs_config);
	
	// Check user configuration done in modInitModule();
	checkAOConfig(ptc);
	checkFOAMConfig(cs_config);
	
	// START DAEMON //
	/****************/
		
	io->msg(IO_INFO,"Starting daemon at port %s...", cs_config->listenport);
  protocol = new Protocol::Server(cs_config->listenport);
  protocol->slot_message = sigc::ptr_fun(on_message);
  protocol->slot_connected = sigc::ptr_fun(on_connect);
  protocol->listen();
  
	// START THREADING //
	/*******************/
	
	// ignore all signals that might cause problems (^C),
	// and enable them on a per-thread basis lateron.
  sigemptyset(&signal_mask);
  sigaddset(&signal_mask, SIGINT); // 'user' stuff
  sigaddset(&signal_mask, SIGTERM);
  sigaddset(&signal_mask, SIGPIPE);
	
	sigaddset(&signal_mask, SIGSEGV); // 'bad' stuff, try to do a clean exit
	sigaddset(&signal_mask, SIGBUS);
    
	int threadrc;
	
	threadrc = pthread_sigmask (SIG_BLOCK, &signal_mask, NULL);
    if (threadrc) {
		io->msg(IO_WARN,"Could not set signal blocking for threads.");
		io->msg(IO_WARN,"This might cause problems when sending signals to the program.");
  }
	
	// make thread explicitly joinable
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	
	// Create thread which does all the work
	// this thread inherits the signal blocking defined above
	threadrc = pthread_create(&(cs_config->threads[0]), &attr, &startThread, NULL);
	if (threadrc)
		io->msg(IO_ERR,"Error in pthread_create, return code was: %d.", threadrc);

	cs_config->nthreads = 1;
	
	// now make sure the main thread handles the signals by unblocking them:
	// SIGNAL HANDLING //
	/*******************/
	struct sigaction act;
	
	act.sa_handler = catchSIGINT;
	act.sa_flags = 0;		// No special flags
	act.sa_mask = signal_mask;	// Use this mask
	sigaction(SIGINT, &act, NULL);
	sigaction(SIGINT, &act, NULL);
	pthread_sigmask (SIG_UNBLOCK, &signal_mask, NULL);
	
	while (ptc->mode != AO_MODE_SHUTDOWN)
    usleep(1000*1000);
    
  // TODO: Cleanup & shutdown should go here!
  
  delete protocol;
  delete io;
	return EXIT_SUCCESS;
}

void *startThread(void *arg) {
	// POST-THREADING MODULE INIT//
	/*****************************/
	
	// some things have to be init'ed after threading, like
	// my rather crude implementation of OpenGL.
	// That's done here
	modPostInitModule(ptc, cs_config);

	// directly afterwards, start modeListen
	modeListen();
}

void catchSIGINT(int) {
	// it could be a good idea to reset signal handler, as noted 
	// on http://www.cs.cf.ac.uk/Dave/C/node24.html , but it is
	// currently disabled. This means that after a failed ^C,
	// this signal goes back to its default action.
	
	// signal(SIGINT, catchSIGINT);
	
	// stop the framework
	stopFOAM();
}

void stopFOAM() {
	int i;
	char date[64];
	struct tm *loctime;
	time_t end;
	void *status;
	int rc;
	
  // Change mode
	ptc->mode = AO_MODE_SHUTDOWN;
	
	// Notify shutdown
	io->msg(IO_INFO, "Shutting down FOAM now");
  protocol->broadcast("500 :SHUTTING DOWN NOW");
	
	// Signal the change to the other thread(s)
	pthread_cond_signal(&mode_cond);
	
	// Wait for the modules
	usleep(100000); // 0.1 sec

	// get the time to see how long we've run
	end = time (NULL);
	loctime = localtime (&end);
	strftime (date, 64, "%A, %B %d %H:%M:%S, %Y (%Z).", loctime);	
	
	// stop prime prime module if it hasn't already
	io->msg(IO_INFO, "Trying to stop modules...");
	modStopModule(ptc);
	
	// and join with all threads
	io->msg(IO_INFO, "Waiting for threads to stop...");
	for (i=0; i<cs_config->nthreads; i++) {
		rc = pthread_join(cs_config->threads[i], &status);
		if (rc)
			io->msg(IO_WARN, "There was a problem joining worker thread %d/%d, return code was: %d (%s)", \
					i+1, cs_config->nthreads, rc, strerror(errno));
		else
			io->msg(IO_DEB1, "Thread %d/%d joined successfully, exit status was: %ld", \
					i+1, cs_config->nthreads, (long) status);

	}
	
	// finally, destroy the pthread variables
	io->msg(IO_DEB1, "Destroying thread configuration (mutex, cond, attr)...");
	pthread_mutex_destroy(&mode_mutex);
	pthread_cond_destroy(&mode_cond);
	pthread_attr_destroy(&attr);
	
	// last log message just before closing the logfiles
	io->msg(IO_INFO, "Stopping FOAM at %s", date);
	io->msg(IO_INFO, "Ran for %ld seconds, parsed %ld frames (%.1f FPS).", \
		end-ptc->starttime, ptc->frames, ptc->frames/(float) (end-ptc->starttime));

	// close the logfiles
	if (cs_config->infofd) fclose(cs_config->infofd);
	if (cs_config->errfd) fclose(cs_config->errfd);
	if (cs_config->debugfd) fclose(cs_config->debugfd);
	
	// and exit with success
	exit(EXIT_SUCCESS);
}

void checkFieldFiles(wfs_t *wfsinfo) {
	FILE *fieldfd;
	if (wfsinfo->darkfile == NULL) {
	  // File not set, not using darkfield calibration
		io->msg(IO_INFO, "Not using darkfield calibration, no darkfield file given");
	} else {
		// Allocate memory, try to load the file into memory
		wfsinfo->darkim = gsl_matrix_float_calloc(wfsinfo->res.x, wfsinfo->res.y);
		fieldfd = fopen(wfsinfo->darkfile, "r");
		if (fieldfd != NULL) {
			io->msg(IO_INFO, "Loading darkfield file (%s)...", wfsinfo->darkfile);
			gsl_matrix_float_fscanf(fieldfd, wfsinfo->darkim);
			fclose(fieldfd);
		}
	}
	
	// Same for flatfield
	if (wfsinfo->flatfile == NULL) {
		io->msg(IO_INFO, "Not using flatfield calibration, no flatfield file given");
	} else {
	  wfsinfo->flatim = gsl_matrix_float_calloc(wfsinfo->res.x, wfsinfo->res.y);
		fieldfd = fopen(wfsinfo->flatfile, "r");
		if (fieldfd != NULL) {
			io->msg(IO_INFO, "Loading flatfield file (%s)...", wfsinfo->flatfile);
			gsl_matrix_float_fscanf(fieldfd, wfsinfo->flatim);
			fclose(fieldfd);
		}
	}
	
	// Same for skyfield
	if (wfsinfo->skyfile == NULL) {
		io->msg(IO_INFO, "Not using skyfield calibration, no skyfield file given");
	} else {
	  wfsinfo->skyim = gsl_matrix_float_calloc(wfsinfo->res.x, wfsinfo->res.y);
		fieldfd = fopen(wfsinfo->skyfile, "r");
		if (fieldfd != NULL) {
			io->msg(IO_INFO, "Loading skyfield file (%s)...", wfsinfo->skyfile);
			gsl_matrix_float_fscanf(fieldfd, wfsinfo->skyim);
			fclose(fieldfd);
		}
	}
}

void checkAOConfig(control_t *ptc) {
	int i;
	// Check WFS configuration here //
	//////////////////////////////////
	
	// first check the amount of WFSs, less than 0 is weird, as is more than 4 (in simple systems at least)
	if (ptc->wfs_count < 0 || ptc->wfs_count > 3) {
		io->msg(IO_WARN, "Total of %d WFS, seems unsane?", ptc->wfs_count);
	}
	else {
		// then check each WFS setting individually
		for (i=0; i< ptc->wfs_count; i++) {
			if (ptc->wfs[i].res.x < 0 || ptc->wfs[i].res.x > 1024 || ptc->wfs[i].res.y < 0 || ptc->wfs[i].res.y > 1024) 
				io->msg(IO_WARN, "Resolution of WFS %d is odd: %dx%d", i, ptc->wfs[i].res.x, ptc->wfs[i].res.y);

			if (ptc->wfs[i].bpp != 8) {
				io->msg(IO_WARN, "Bitdepth %d for WFS %d unsupported, defaulting to 8.", ptc->wfs[i].bpp, i);
				ptc->wfs[i].bpp = 8;
			}
			
			// check dark, flat and sky calibration files
			checkFieldFiles(&(ptc->wfs[i]));
			
			// allocate memory for corrected image
			ptc->wfs[0].corrim = gsl_matrix_float_alloc(ptc->wfs[0].res.x, ptc->wfs[0].res.y);
			
			// check scan direction
			if (ptc->wfs[i].scandir != AO_AXES_XY && ptc->wfs[i].scandir != AO_AXES_Y && ptc->wfs[i].scandir != AO_AXES_X) {
				io->msg(IO_WARN, "Scandir not set to either AO_AXES_XY, AO_AXES_X or AO_AXES_Y, defaulting to AO_AXES_XY");
				ptc->wfs[i].scandir = AO_AXES_XY;
			}
		}
	}
	
	// Check WFC configuration here //
	//////////////////////////////////
	
	// first check the amount of WFCs
	if (ptc->wfc_count < 0 || ptc->wfc_count > 3) {
		io->msg(IO_WARN, "Total of %d WFC, seems unsane?", ptc->wfc_count);
	}
	else {
		// then check each WFC setting individually
		for (i=0; i< ptc->wfc_count; i++) {
			if (ptc->wfc[i].nact < 1) {
				io->msg(IO_WARN, "%d actuators for WFC %d? This is hard to believe, disabling WFC %d.", ptc->wfc[i].nact, i, i);
				// 0 acts effectively disables a WFC
				ptc->wfc[i].nact = 0;
				ptc->wfc[i].ctrl = NULL;
			}
			else {
				if (ptc->wfc[i].nact > 1000) io->msg(IO_INFO, "%d actuators for WFC %d? Impressive...", ptc->wfc[i].nact, i);
				io->msg(IO_INFO, "Allocating memory for %d actuator control voltages.", ptc->wfc[i].nact);
				ptc->wfc[i].ctrl = gsl_vector_float_calloc(ptc->wfc[i].nact);
			}
			
			if (ptc->wfc[0].type != WFC_DM && ptc->wfc[0].type != WFC_TT) {
				io->msg(IO_ERR, "Unknown WFC type (not WFC_DM, nor WFC_TT).");
			}
		}
	}
	
	// Check filter configuration here //
	/////////////////////////////////////
	
	// first check the amount of FWs
	if (ptc->fw_count < 0 || ptc->fw_count > 3) {
		io->msg(IO_WARN, "Total of %d FWs, seems unsane?", ptc->fw_count);
	}
	else {
		// then check each FW setting individually
		for (i=0; i< ptc->fw_count; i++) {
			if (ptc->filter[0].nfilts > MAX_FILTERS) {
				io->msg(IO_WARN, "Warning, number of filters (%d) for filterwheel %d larger than MAX_FILTERS, filters above position %d will not be used.", \
				ptc->filter[0].nfilts, i, MAX_FILTERS);
				ptc->filter[0].nfilts = MAX_FILTERS;
			}
			else if (ptc->filter[0].nfilts <= 0) {
				io->msg(IO_WARN, "Warning, zero or less filters for filterweel %d, disabling filterwheel.", i);
				ptc->filter[0].nfilts = 0;
			}
		}
	}

    // Check other configuration here //
	////////////////////////////////////
    
	if (ptc->logfrac < 1)
		ptc->logfrac = 0;
	else if (ptc->logfrac > 10000)
		io->msg(IO_WARN, "%d might be a rather large value for logfrac.", ptc->logfrac);
	
	io->msg(IO_INFO, "AO Configuration for wavefront sensors, wavefront correctors, and filterwheels verified.");
}

void checkFOAMConfig(config_t *conf) {
	// Check the info, error and debug files that we possibly have to log to
	initLogFiles();
	
	// Init syslog if necessary
	if (conf->use_syslog == true) {
		openlog(conf->syslog_prepend, LOG_PID, LOG_USER);
		io->msg(IO_INFO, "Syslog successfully initialized.");
	}
	
	io->msg(IO_INFO, "Configuration successfully loaded...");
}

int initLogFiles() {
	// INFO LOGGING
	if (cs_config->infofile != NULL) {
		if ((cs_config->infofd = fopen(cs_config->infofile,"a")) == NULL) {
			io->msg(IO_WARN, "Unable to open file %s for info-logging! Not using this logmethod!", cs_config->infofile);
			cs_config->infofile[0] = '\0';
		}	
		else io->msg(IO_INFO, "Info logfile '%s' successfully opened.", cs_config->infofile);
	}
	else
		io->msg(IO_INFO, "Not logging general info to disk.");

	// ERROR/WARNING LOGGING
	if (cs_config->errfile != NULL) {
		if (strcmp(cs_config->errfile, cs_config->infofile) == 0) {	// If the errorfile is the same as the infofile, use the same FD
			cs_config->errfd = cs_config->infofd;
			io->msg(IO_DEB1, "Using the same file '%s' for info- and error- logging.", cs_config->errfile);
		}
		else if ((cs_config->errfd = fopen(cs_config->errfile,"a")) == NULL) {
			io->msg(IO_WARN, "Unable to open file %s for error-logging! Not using this logmethod!", cs_config->errfile);
			cs_config->errfile[0] = '\0';
		}
		else io->msg(IO_INFO, "Error logfile '%s' successfully opened.", cs_config->errfile);
	}
	else {
		io->msg(IO_INFO, "Not logging errors to disk.");
	}

	// DEBUG LOGGING
	if (cs_config->debugfile != 0) {
		if (strcmp(cs_config->debugfile,cs_config->infofile) == 0) {
			cs_config->debugfd = cs_config->infofd;	
			io->msg(IO_DEB1, "Using the same file '%s' for debug- and info- logging.", cs_config->debugfile);
		}
		else if (strcmp(cs_config->debugfile,cs_config->errfile) == 0) {
			cs_config->debugfd = cs_config->errfd;	
			io->msg(IO_DEB1, "Using the same file '%s' for debug- and error- logging.", cs_config->debugfile);
		}
		else if ((cs_config->debugfd = fopen(cs_config->debugfile,"a")) == NULL) {
			io->msg(IO_WARN, "Unable to open file %s for debug-logging! Not using this logmethod!", cs_config->debugfile);
			cs_config->debugfile[0] = '\0';
		}
		else io->msg(IO_INFO, "Debug logfile '%s' successfully opened.", cs_config->debugfile);
	}
	else {
		io->msg(IO_INFO, "Not logging debug to disk.");
	}

	return EXIT_SUCCESS;
}

void modeOpen() {
	// for FPS tracking
	struct timeval last, cur;
	long lastframes, curframes;
	gettimeofday(&last, NULL);
	lastframes = ptc->frames;

	io->msg(IO_INFO, "Entering open loop.");

	if (ptc->wfs_count == 0) {	// we need wave front sensors
		io->msg(IO_WARN, "Error, no WFSs defined.");
		ptc->mode = AO_MODE_LISTEN;
		return;
	}
	
	// Run the initialisation function of the modules used, pass
	// along a pointer to ptc
	if (modOpenInit(ptc) != EXIT_SUCCESS) {		// check if we can init the module
		io->msg(IO_WARN, "modOpenInit failed");
		ptc->mode = AO_MODE_LISTEN;
		return;
	}
	ptc->frames++;
	
	// tellClients("201 MODE OPEN SUCCESSFUL");
	while (ptc->mode == AO_MODE_OPEN) {
		if (modOpenLoop(ptc) != EXIT_SUCCESS) {
			io->msg(IO_WARN, "modOpenLoop failed");
			ptc->mode = AO_MODE_LISTEN;
			return;
		}
		ptc->frames++;	// increment the amount of frames parsed
		if (ptc->frames % ptc->logfrac == 0) {
			curframes = ptc->frames;
			gettimeofday(&cur, NULL);
			ptc->fps = (cur.tv_sec * 1000000 + cur.tv_usec)- (last.tv_sec*1000000 + last.tv_usec);	
			ptc->fps /= 1000000;
			ptc->fps = (curframes-lastframes)/ptc->fps;
			last = cur;
			lastframes = curframes;
		}
	}
	
	// Finish the open loop here
	if (modOpenFinish(ptc) != EXIT_SUCCESS) {		// check if we can finish
		io->msg(IO_WARN, "modOpenFinish failed");
		ptc->mode = AO_MODE_LISTEN;
		return;
	}
	
	return; // mode is not open anymore, decide what to to next in modeListen
}

void modeClosed() {	
	// for FPS tracking
	struct timeval last, cur;
	long lastframes, curframes;
	gettimeofday(&last, NULL);
	lastframes = ptc->frames;
	io->msg(IO_INFO, "Entering closed loop.");

	if (ptc->wfs_count == 0) {						// we need wave front sensors
		io->msg(IO_WARN, "Error, no WFSs defined.");
		ptc->mode = AO_MODE_LISTEN;
		return;
	}
	
	// Run the initialisation function of the modules used, pass
	// along a pointer to ptc
	if (modClosedInit(ptc) != EXIT_SUCCESS) {
		io->msg(IO_WARN, "modClosedInit failed");
		ptc->mode = AO_MODE_LISTEN;
		return;
	}
	
	ptc->frames++;
	// tellClients("201 MODE CLOSED SUCCESSFUL");
	while (ptc->mode == AO_MODE_CLOSED) {
		
		if (modClosedLoop(ptc) != EXIT_SUCCESS) {
			io->msg(IO_WARN, "modClosedLoop failed");
			ptc->mode = AO_MODE_LISTEN;
			return;
		}							
		ptc->frames++;								// increment the amount of frames parsed
		if (ptc->frames % ptc->logfrac == 0) {
			curframes = ptc->frames;
			gettimeofday(&cur, NULL);
			ptc->fps = (cur.tv_sec * 1000000 + cur.tv_usec)- (last.tv_sec*1000000 + last.tv_usec);	
			ptc->fps /= 1000000;
			ptc->fps = (curframes-lastframes)/ptc->fps;
			last = cur;
			lastframes = curframes;
		}
	}
	
	// Finish the open loop here
	if (modClosedFinish(ptc) != EXIT_SUCCESS) {	// check if we can finish
		io->msg(IO_WARN, "modClosedFinish failed");
		ptc->mode = AO_MODE_LISTEN;
		return;
	}
	
	
	return;					// back to modeListen (or where we came from)
}

void modeCal() {
	io->msg(IO_INFO, "Starting Calibration");
	
	// this links to a module
	if (modCalibrate(ptc) != EXIT_SUCCESS) {
		io->msg(IO_WARN, "modCalibrate failed");
		// tellClients("404 ERROR CALIBRATION FAILED");
		ptc->mode = AO_MODE_LISTEN;
		return;
	}
	
	io->msg(IO_INFO, "Calibration loop done, switching to listen mode");
	// tellClients("201 CALIBRATION SUCCESSFUL");
	ptc->mode = AO_MODE_LISTEN;
		
	return;
}

void modeListen() {
	
	while (true) {
		io->msg(IO_INFO, "Now running in listening mode.");
		
		switch (ptc->mode) {
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
				// tellClients("201 MODE LISTEN SUCCESSFUL");
				pthread_mutex_lock(&mode_mutex);
				pthread_cond_wait(&mode_cond, &mode_mutex);
				pthread_mutex_unlock(&mode_mutex);
				break;
			case AO_MODE_SHUTDOWN:
				// we want to shutdown the program, return modeListen
				pthread_exit((void *) 0);
				break;
				
		}
	} // end while(true)
}

static void on_connect(Connection *connection, bool status) {
  if (status) {
    connection->write("201 :CLIENT CONNECTED");
    io->msg(IO_DEB1, "Client connected.");
  }
  else {
    connection->write("201 :CLIENT DISCONNECTED");
    io->msg(IO_DEB1, "Client disconnected.");
  }
}

static void on_message(Connection *connection, string line) {
  io->msg(IO_DEB1, "got %db: '%s'.", line.length(), line.c_str());
  connection->write("200 :OK");
	
	string cmd = popword(line);
	
	if (cmd == "help") {
    // if (showhelp(connection, line) < 0)
    //   if (modMessage(connection, cmd, line) < 0)
    //     connection->write("401 :UNKNOWN HELP TOPIC");
  }
  else if (cmd == "exit" || cmd == "quit" ) {
    //connection->broadcast("201 :CLIENT DISCONNECTED");
    connection->close();
  }
  else if (cmd == "shutdown") {
    connection->write("201 :FOAM SHUTTING DOWN");
  }
  else if (cmd == "mode") {
    //
  }
  else {
    //modMessage(connection, cmd, line) <= 0)
  }
//  if (subhelp == NULL) {
//    tellClient(client->buf_ev, "\
// 200 OK HELP\n\
// help [command]:         help (on a certain command, if available).\n\
// mode <mode>:            close or open the loop.\n\
// broadcast <msg>:        send a message to all connected clients.\n\
// exit or quit:           disconnect from daemon.\n\
// shutdown:               shutdown the FOAM program.");
// //image <wfs>:            get the image from a certain WFS.\n
//  }
	
  //parseCmd(line, nbytes, client);
}
// 
// 
// static int explode(char *str, char **list) {
//  size_t begin, len;
//  int i;
//  // Take a string 'str' which has space-separated
//  // words in it and store pointers to the
//  // individual words in 'list'
//  for (i=0; i<Maxparams; i++ ) { 
//    // search for the first non-space char
//    begin = strspn(str, " \t\n\r\0");
//    str += begin;
//  
//    // search for the first space char,
//    // starting from the first nons-space char
//    len = strcspn(str, " \t\n\r\0");
// 
//    // if len=0, there are no more words
//    // in 'str', use NULL to indicate this
//    // in 'list'
//    if (len == 0)  {
//      list[i] = NULL;
//      return i+1;
//    }   
//    // if len !=0, store the pointer to 
//    // the word in the list
//    list[i] = str;
// 
//    // if the next character is a null
//    // char, we've reached the end of
//    // 'str'
//    if (str[len] == '\0') {
//      // if i<16, we can still use
//      // NULL to indicate this was
//      // the last word. Otherwise,
//      // we've stopped anyway
//      if (i < 15) list[i+1] = NULL;
//      return i+1;
//    }   
//    // replace the first space after
//    // the word with a null character
//    str[len] = '\0';
// 
//    // proceed the pointer with the length
//    // of the word, + 1 for the null char
//    str += len+1;
//  }
//  
//  // if we finish the loop, we parsed Maxlength words:
//  return i+1;
// }
// 
// int parseCmd(char *msg, const int len, client_t *client) {
//  // reserve memory for a local copy of 'msg'
//  char local[strlen(msg)+1];
//  char *list[Maxparams];
//  int count;
//  
//  // copy the string there, append \0
//  strncpy(local, msg, strlen(msg));
//  local[strlen(msg)] = '\0';
// 
//  // length < 2? nothing to see here, move on
//  if (strlen(msg) < 2)
//    return tellClient(client->buf_ev,"400 UNKNOWN");
// 
//  // explode the string, list[i] now holds the ith word in 'local'
//  if ((count = explode(local, list)) == 0) {
//    io->msg(IO_WARN, "parseCmd called without any words: '%s' or '%s' has %d words", msg, local, count);
//    return EXIT_FAILURE;
//  }
//  io->msg(IO_DEB1, "We got: '%s', first word: '%s', words: %d", msg, list[0], count);
//      
//  // inspect the string, see what we are going to do:
//  if (strcmp(list[0],"help") == 0) {
//    // Does the user want help on a specific command?
//    // if so, count > 1 and list[1] holds the command the user wants help on
//    if (count > 1) {    
//      // if both showHelp (general FOAM help) and modMessage (specific module help)
//      // do not have help info for list[1], we tell the client that we don't know either
//      if (showHelp(client, list[1]) <= 0 && modMessage(ptc, client, list, count) <= 0)
//        tellClient(client->buf_ev, "401 UNKOWN HELP");
//    }
//    else {
//      // Give generic help here, both on FOAM and on the modules
//      showHelp(client, NULL);
//      modMessage(ptc, client, list, count);
//    }
//  }
//  else if ((strcmp(list[0],"exit") == 0) || (strcmp(list[0],"quit") == 0)) {
//    tellClient(client->buf_ev, "200 OK EXIT");
//    // we disconnect a client by simulation EOF
//    sockOnErr(client->buf_ev, EVBUFFER_EOF, client);
//  }
//  else if (strcmp(list[0],"shutdown") == 0) {
//    tellClients("200 OK SHUTTING DOWN NOW!");
//    stopFOAM();
//  }
//  /* This probably does not work correctly yet
//  else if (strcmp(list[0],"image") == 0) {
//    if (count > 1) {
//      tmpint = (int) strtol(list[1], NULL, 10);
//      if (tmpint < ptc->wfs_count) {
//        tellClient(client->buf_ev, "200 OK IMAGE WFS %d", tmpint);
//        tellClient(client->buf_ev, "SIZE %d", ptc->wfs[tmpint].res.x * ptc->wfs[tmpint].res.y * ptc->wfs[tmpint].bpp/8);
//        bufferevent_write(client->buf_ev, ptc->wfs[tmpint].image, ptc->wfs[tmpint].res.x * ptc->wfs[tmpint].res.y * ptc->wfs[tmpint].res.x * ptc->wfs[tmpint].bpp/8);
//      }
//      else {
//        tellClient(client->buf_ev,"401 UNKNOWN WFS %d", tmpint);
//      }
//    }
//    else {
//      tellClient(client->buf_ev,"402 IMAGE REQUIRES ARG");
//    }   
//  }*/
//  else if (strcmp(list[0],"broadcast") == 0) {
//    if (count > 1) {
//      tellClients("200 OK %s", msg);
//    }
//    else if (count == Maxparams) {
//      tellClient(client->buf_ev,"401 BROADCAST ACCEPTS ONLY %d WORDS", Maxparams);
//    }
//    else {
//      tellClient(client->buf_ev,"402 BROADCAST REQUIRES ARG (BROADCAST TEXT)");
//    }
//  }
//  else if (strcmp(list[0],"mode") == 0) {
//    if (count > 1) {
//      if (strcmp(list[1],"closed") == 0) {
//        ptc->mode = AO_MODE_CLOSED;
//        pthread_cond_signal(&mode_cond); // signal a change to the main thread
//        tellClients("200 OK MODE CLOSED");
//      }
//      else if (strcmp(list[1],"open") == 0) {
//        ptc->mode = AO_MODE_OPEN;
//        pthread_cond_signal(&mode_cond);
//        tellClients("200 OK MODE OPEN");
//      }
//      else if (strcmp(list[1],"listen") == 0) {
//        ptc->mode = AO_MODE_LISTEN;
//        pthread_cond_signal(&mode_cond);
//        tellClients("200 OK MODE LISTEN");
//      }
//      else {
//        tellClient(client->buf_ev,"401 UNKNOWN MODE");
//      }
//    }
//    else {
//      tellClient(client->buf_ev,"402 MODE REQUIRES ARG (CLOSED, OPEN, LISTEN)");
//    }
//  }
//  else {
//    // No valid command found? pass it to the module
//    if (modMessage(ptc, client, list, count) <= 0) {
//      return tellClient(client->buf_ev,"400 UNKNOWN");
//    }
//  }
//  
//  return EXIT_SUCCESS;
// }

// int tellClients(char *msg, ...) {
//  va_list ap;
//  int i;
//  char *out, *out2;
// 
//  va_start(ap, msg);
// 
//  vasprintf(&out, msg, ap);
//  // format string and add newline
//  asprintf(&out2, "%s\n", out);
// 
//  io->msg(IO_DEB1, "Telling all clients");
//  for (i=0; i < MAX_CLIENTS; i++) {
//    io->msg(IO_DEB1|IO_NOID, "%d ", i);
//    if (clientlist.connlist[i] != NULL && clientlist.connlist[i]->fd > 0) { 
//      io->msg(IO_DEB1|IO_NOID, "Telling! ");  
//      if (bufferevent_write(clientlist.connlist[i]->buf_ev, out2, strlen(out2)+1) != 0) {
//        io->msg(IO_WARN, "Error telling client %d", i);
//        return EXIT_FAILURE; 
//      }
//    }
//  }
//  io->msg(IO_DEB1|IO_NOID, "\n");
//  return EXIT_SUCCESS;
// }
// 
// int tellClient(struct bufferevent *bufev, char *msg, ...) {
//  va_list ap;
//  char *out, *out2;
// 
//  va_start(ap, msg);
// 
//  // !!!:tim:20080414 this can be done faster?
//  vasprintf(&out, msg, ap);
//  asprintf(&out2, "%s\n", out);
// 
//  if (bufferevent_write(bufev, out2, strlen(out2)+1) != 0) {
//    io->msg(IO_WARN, "Error telling client something");
//    return EXIT_FAILURE; 
//  }
//  
//  return EXIT_SUCCESS;
// }
// 
// int showHelp(const client_t *client, const char *subhelp) {
//  // spaces are important here!!!
//  if (subhelp == NULL) {
//    tellClient(client->buf_ev, "\
// 200 OK HELP\n\
// help [command]:         help (on a certain command, if available).\n\
// mode <mode>:            close or open the loop.\n\
// broadcast <msg>:        send a message to all connected clients.\n\
// exit or quit:           disconnect from daemon.\n\
// shutdown:               shutdown the FOAM program.");
// //image <wfs>:            get the image from a certain WFS.\n
//  }
//  else if (strcmp(subhelp, "mode") == 0) {
//    tellClient(client->buf_ev, "\
// 200 OK HELP MODE\n\
// mode <mode>: close or open the loop.\n\
//    mode=open: opens the loop and only records what's happening with the AO \n\
//         system and does not actually drive anything.\n\
//    mode=closed: closes the loop and starts the feedbackloop, correcting the\n\
//         wavefront as fast as possible.\n\
//    mode=listen: stops looping and waits for input from the users. Basically\n\
//         does nothing.\n");
//  }
//  else if (strcmp(subhelp, "image") == 0) {
//    tellClient(client->buf_ev, "\
// 200 OK HELP IMAGE\n\
// image <wfs>: request to send a WFS image over the network.\n\
//     <wfs> must be a valid wavefrontsensor.\n\
//     Note that this transfer may take a while and could slowdown the AO itself.\n");
//  } 
//  else if (strcmp(subhelp, "help") == 0) {
//      tellClient(client->buf_ev, "\
// 200 OK HELP HELP\n\
// help [topic]\n\
//    show help on a topic, or (if omitted) in general");
//  }
//  else {
//    // if we end up here, 'subhelp' was unknown to us, and we tell parseCmd
//    return 0;
//  }
//  
//  // if we end up here, everything went ok
//  return 1;
// }

// DOXYGEN GENERAL DOCUMENTATION //
/*********************************/

/* Doxygen needs the following aliases in the configuration file!

ALIASES += authortim="Tim van Werkhoven (T.I.M.vanWerkhoven@phys.uu.nl)"
ALIASES += license="GPLv2"
ALIASES += wisdomfile="fftw_wisdom.dat"
ALIASES += name="FOAM"
ALIASES += longname="Modular Adaptive Optics Framework"
ALIASES += uilib="foam_ui_library.*"
ALIASES += cslib="foam_cs_library.*"
*/


/*!	\mainpage FOAM 

	\section aboutdoc About this document
	
	This is the (developer) documentation for FOAM, the Modular Adaptive Optics Framework 
	 (yes, FOAM is backwards). It is intended to clarify the
	code and give some help to people re-using or implementing the code for their specific needs.
	
	\section aboutfoam About FOAM
	
	FOAM, the Modular Adaptive Optics Framework, is intended to be a modular
	framework which works independent of hardware and should provide such
	flexibility that the framework can be implemented on any AO system.
	
	A short list of the features of FOAM follows:
 
	\li Portable - It runs on many (Unix) systems and is hardware independent
	\li Scalable - It scales easily and handle multiple cores/CPU's
	\li Extensible - It can be extended with new features or algorithms relatively easily
	\li Usable - It is controllable over a network by multiple clients simultaneously
	\li Free - It licensed under the GPL and therefore can be used by anyone.
	
	For more information, see the FOAM wiki at http://www.astro.uu.nl/~astrowik/astrowiki/index.php/FOAM or the documentation
	at http://dotdb.phys.uu.nl/~tim/foam/ . All versions of FOAM are available at the
	Subversion repository located at http://dotdb.phys.uu.nl/svn/foam/ .
	
	\section struct FOAM structure
	
	In this section, the structure of FOAM will be clarified. First, some key 
	concepts are explained which are used within FOAM.
	
	FOAM uses several concepts, for which it is useful to have names. First of 
	all, the complete set of files (downloadable from for example http://dotdb.phys.uu.nl/~tim/foam/) 
	is simply called a \e `distribution', or \e `version' or just \e FOAM.
	
	Historically, there was a FOAM Control Software (CS) and a User Interface 
	(UI), although the latter is not actively being developed as telnet suffices
	for the moment. This explains the \c `_cs' and \c `_ui' prefix in some filenames.
	
	Within the distribution itself (which consists mainly of the Control Software
	at the moment), there are several different components:
 
	\li The \e framework itself, in \c foam_cs.c
	\li \e Prime \e modules, in \c foam_primemod-*
	\li \e Modules, in \c foam_modules-*
	
	To get a running program, the framework, a prime module and zero or more 
	modules are combined into one executable. This program is called a 
	\e `package' and is what performs AO operations.
	
	By default a dummy package is supplied, which take the framework
	and the dummy prime module (which does nothing) which results in the 
	most basic package possible. A more interesting package, the simulation 
	package, is also provided. This package simulates a complete AO system 
	and can be used to test new routines (center-of-gravity tracking, 
	correlation tracking, load distribution) in a reproducible fashion. 

	There are two simulation prime modules, one is `simdyn', which performs
	dynamical simulation, meaning the atmosphere, WFCs, WFSs etc are all simulated
	dynamically, and `simstat', which uses a static WFS image and performs
	the calculations that a normal AO system would do. The first can be useful
	to see if the system works at all, and to improve algorithms qualitatively,
	while the second can be used to benchmark the performance of the system without
	the need of AO hardware.
	
	\subsection struct-frame The Framework
	
	The framework itself does very little. It performs some initialization, allocating 
	memory for control vectors, images and what not. After that it provides a hook for 
	prime modules to initialize themselves. Immediately after that, it splits in a worker 
	thread which does the AO calculations, and in a networking thread which listens for 
	connections on a (TCP) socket. The worker thread starts startThread(), while the
	other thread starts with sockListen(). Communication between the two threads is done 
	with the mutexes `mode_mutex' and `mode_cond'.
	
	The framework provides the following hooks to prime modules: 
	\li modStopModule(control_t *ptc)
	\li modInitModule(control_t *ptc, config_t *cs_config)
	\li modPostInitModule(control_t *ptc, config_t *cs_config)
	\li modOpenInit(control_t *ptc)
	\li modOpenLoop(control_t *ptc)
	\li modOpenFinish(control_t *ptc)
	\li modClosedInit(control_t *ptc)
	\li modClosedLoop(control_t *ptc)
	\li modClosedFinish(control_t *ptc)
	\li modCalibrate(control_t *ptc)
	\li modMessage(control_t *ptc, const client_t *client, char *list[], const int count)
	
	See the documentation on the specific functions for more details on what these functions
	can be used for.

	\subsection struct-prime The Prime Modules
	
	Prime modules must define the functions defined in the above list. The prime 
	module should tell what these hooks do, e.g. what happens during open loop, 
	closed loop and during calibration. These functions can be just placeholders,
	which can be seen in \c foam_primemod-dummy.c , or more complex functions 
	which actually do something.
	
	Almost always, prime modules link to modules in turn, by simply including 
	their header files and compiling the source files along with the prime module. 
	The modules contain the functions which do the hard work, and are divided into 
	separate files depending on the functionality of the routines. The prime module
	can thus be seen as a sort of hub connecting the framework with the modules.
	
	\subsection struct-mod The Modules
	
	Modules provide functions which can be used by other modules or by prime 
	modules. If a module uses routines from another module, this module depends 
	on the other module. This is for example the case between the `simdyn' module
	which needs to read in PGM files and therefore depends on the `img' module.
	
	A few default modules are provided with the distribution, and their function
	can be can be read at the top of the header file associated with the module.

	\subsection threading Threading model
 
	As noted before, the framework uses two threads. The main thread is used for
	networking with the clients, while the AO routines are performed in a different
	thread which separates from the main thread at the beginning. 
 
	These threads are not equal, as the first thread is created at program startup while the
	second one splits off this thread. This can be important for example when 
	using SDL on OS X, which requires some initialization code that *is* 
	automatically run when starting SDL from the main thread, but that is not
	run when starting SDL from any other thread. Another issue can be OpenGL,
	which can only be called from one thread, and thus must be initialized in
	the thread it will be called from.
 
	To circumvent possible problems, FOAM provides two initialization routines,
	one is run at the beginning before any threading, modInitModule(), while
	the other is run in the thread that will be controlling the AO, 
	modPostInitModule(). Most configuration consists of things like allocating 
	memory, setting variables, reading files etc which are thread-safe. Other 
	things such as OpenGL initialization might not be thread-safe and might 
	have to be initialized after threading.
 
	Another thing to keep in mind is that all functions are called from the AO
	worker thread, except for the modMessage() function, which is called from
	the networking thread. This gives some problems when using libevent to
	send information to clients as libevent is not entirely thread safe
	(and its thread-safety is unclear as well). Currently, only the networking
	thread does interaction with the user.
 
	\subsection codeskel Code skeleton
 
	To provide some insight in the way FOAM functions, consider this code skeleton:

	\code
main() {
	modInitModule()				// Initialize modules,
								// memory, cameras etc.
	
	pthread (startThread())		// Branch into two threads,
								// one for AO, one for user
								// I/O
	
	sockListen()				// Read & process user I/O.
	exit
}

sockListen() {
	while (true) {				// In a continuous loop,
		parseCmd()				// read the user input and
		modMessage()			// process it.
	}
}

startThread() {
	modPostInitModule()			// After threading, provide
								// an additional init hook.
}

listenLoop() {
	while (ptc->mode != shutdown) { // Run continuously until
								// shutdown.
		switch (ptc->mode) {		// Read the mode requested
								// and switch to that mode.
			case 'open': modeOpen()
			case 'closed': modeClosed()
			case 'calibration': modeCal()
		}
	}
	modStopModule()				// Shutdown the modules.
}

modeOpen() {
	modOpenInit()				// Open loop init hook.
	
	while (ptc->mode == AO_MODE_OPEN) { 
								// Loop until mode changes.
		modOpenLoop()			// Actual work is done here.
	}
	
	modOpenFinish()				// Clean up after open loop.
}

modeClosed() {
	modClosedInit()				// Closed loop works the
								// same as open loop.
	
	while (ptc->mode == AO_MODE_CLOSED) {
		modClosedLoop()
	}
	
	modClosedFinish()
}

modeCal() {
	modCalibrate()				// Calibration mode provides
								// simply one hook.
}
	\endcode
 
	Note that all hooks (starting with mod*) are run from the AO thread, except for
	modMessage(), which is run from the networking thread.
 
	\section install_sec Installation
	
	FOAM currently depends on the following libraries to be present on the system:
	\li \c libevent used to handle networking with several simultaneous connections,
	\li \c SDL which is used to display the sensor output,
	\li \c pthread used to separate functions over thread and distribute load,
	\li \c libgsl used to do singular value decomposition, link to BLAS and various other matrix/vector operation.
	
	For other prime modules, additional libraries may be required. For example,
	the simdyn prime module also requires:
	\li \c fftw3 which is used to compute FFT's to simulate the SH lenslet array,
	\li \c SDL_Image used to read image files files.
	
	Furthermore FOAM requires basic things like a hosted compilation environment, 
	a compiler etc. For a full list of dependencies, see the header files. FOAM 
	is however supplied with an (auto-)configure script which checks these
	things and tells you what the capabilities of FOAM on your system will be. If
	libraries are missing, the configure script will tell you so.
	
	To install FOAM, follow these simple steps:
	\li Download a FOAM release from http://dotdb.phys.uu.nl/~tim/foam/ or check out a version of FOAM from http://dotdb.phys.uu.nl/svn/foam/
	\li Extract the tarball
	\li For an svn checkout: run `autoreconf -s -i` to the configure script
	\li Run `./configure --help` to check what options you would like to use
	\li Run `./configure` (with specific options) to prepare FOAM for building. If some libraries are missing, configure will tell you so
	\li Run `make install` which makes the various FOAM packages and installs them in $PREFIX
	\li Run any of the executables (foamcs-*)
	\li Connect to localhost:10000 (default) with a telnet client
	\li Type `help' to get a list of FOAM commands
	
	\subsection config Configure FOAM
	
	With simulation, make sure you do \b not copy the FFTW wisdom file 
	<tt>'fftw_wisdom.dat'</tt> to new machines, this file contains some 
	simple benchmarking results done by FFTW and are very machine dependent. 
	FOAM will regenerate the file if necessary, but it cannot detect 
	`wrong' wisdom files. Copying bad files is worse than deleting, thus
	if unsure: delete the wisdom file.
	
	\section drivers Developing Packages

	As said before, FOAM itself does not do a lot (compile & run 
	<tt>./foamcs-dummy</tt> to verify), it provides a framework to which modules can be 
	attached. Fortunately, this approach allows for complex bundles of 
	modules, or `packages', as explained previously in \ref struct.
	
	If you want to use FOAM in a specific setup, you'll probably have to 
	program some modules yourself. To do this, start with a `prime module' 
	which \b must contain the functions listed in \ref struct-frame (see 
	foam_primemod-dummy.c for example). 
		
	These functions provide hooks for the package to work with, and if 
	their meaning is not immediately clear, the documentation provides some 
	more details on what these functions do. It is also wise to look at the 
	control_t struct which is used throughout FOAM to store data, settings 
	and other information.
	
	Once these functions are defined, you can link the prime module to modules 
	already supplied with FOAM. Simply do this by including the header files 
	of the specific modules you want to add. For information on what certain 
	modules do, look at the documentation of the files. This documentation 
	tells you what the module does, what functions are available, and what 
	other modules this module possibly depends on.
	
	If the default set of modules is insufficient to build a package in your 
	situation, you will have to write your own. This scenario is (unfortunately) 
	very likely, because FOAM does not provide modules to read out all possible 
	framegrabbers/cameras and cannot drive all possible filter wheels, tip-tilt 
	mirrors, deformable mirrors etc. If you have written your own module, please 
	e-mail it to me (Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)) so I can add it to the complete distribution.
	
	\subsection ownmodule Write your own modules
	
	A module in FOAM can take any form, but to keep things at least slightly 
	organised, some conventions apply. These guidelines include:
	
	\li separate functionally different routines in different modules,
	\li Name your modules `foam_modules-\<modulename\>.c' and supply a header file,
	\li Include doxygen compatible information on the module in general in the C file
	\li Include doxygen compatible information on the routines in the header file
	\li Prefix functions that you write with \<modulename\>.
	\li If necessary, provide a datatype to hold module-specific configuration
 
	
	These guidelines are also used in the existing modules so you can easily 
	see what a module does and what functions you have to call to get the module working.
	For examples, see foam_modules-sh.c and foam_modules-sh.h.
	
	\subsection ownmake Adapt the Makefile
	
	Once you have a package, you will need to edit Makefile.am in the src/ 
	directory. But before that, you will need to edit the configure.ac script 
	in the root directory. This is necessary so that at configure time, users 
	can decide what packages to build or not to build, using 
	--enable-\<package\>. In the configure.ac file, you will need to add two 
	parts:
 
	\li Add a check to see if the user wants to build the package or not (see 'TEST FOR USER INPUT')
	\li Add a few lines to check if extra libraries necessary for the package are present (see 'STATIC SIMULATION MODE')
	
	After editing the configure.ac script, the Makefile.am needs to know 
	that we want to build a new package/executable. For an example on how 
	to do this, see the few lines following 'STATIC SIMULATION MODE'. 
	In short: check if the package must be built, if yes, add package to 
	bin_PROGRAMS, and setup various \c _SOURCES, \c _CFLAGS and \c _LDFLAGS 
	variables.
	
	\section network Networking

	Currently, the following status codes are used, based loosely on the HTTP status codes. All
	codes are 3 bytes, followed by a space, and end with a single newline. The length of the message
	can vary.
	
	2xx codes are given upon success:
	\li 200: Successful reception of command, executing immediately,
	\li 201: Executing immediately command succeeded.
	
	4xx codes are errors:
	\li 400: General error, something is wrong
	\li 401: Argument is not known
	\li 402: Command is incomplete (missing argument)
	\li 403: Command given is not allowed at this stage
	\li 404: Previously given and acknowledged command failed.
	
	\section limit_sec Limitations/bugs
	
	There are some limitations to FOAM which are discussed in this section. The list includes:

	\li The subaperture resolution must be a multiple of 4, because of tracking windows
	\li Commands given to FOAM over the socket/network can be at most 1024 characters,
	\li At the moment, most modules work with bytes to process data
	\li On OS X, SDL does not appear to behave nicely, especially during shutdown (this is a problem of the OS X SDL implementation and cannot easily be fixed without using Objective C code)
	
	There might be other limitations or bugs, but these are not listed here 
	because I am not aware of them. If you find some, please let me know (Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)).
 
*/
