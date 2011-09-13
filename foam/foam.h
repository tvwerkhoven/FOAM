/*
 foam.h -- main FOAM header file, defines FOAM framework 
 Copyright (C) 2009--2011 Tim van Werkhoven <t.i.m.vanwerkhoven@xs4all.nl>
 
 This file is part of FOAM.
 
 FOAM is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 2 of the License, or
 (at your option) any later version.
 
 FOAM is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with FOAM.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef HAVE_FOAM_H
#define HAVE_FOAM_H

#ifdef HAVE_CONFIG_H
// Contains various library we have.
#include "autoconfig.h"
#endif

// C & C++ headers
#include <stdio.h>
#include <math.h>
#include <pthread.h>

#include <string>
#include <stdexcept>

// libsiu headers
#include <perflogger.h>
#include <sighandle.h>
#include <protocol.h>
#include <config.h>

// FOAM headers
#include "foamtypes.h"
#include "foamctrl.h"
#include "devices.h"

using namespace std;

typedef Protocol::Server::Connection Connection;

/*!
 @brief Main FOAM class
 
 FOAM is the base class that can be derived to specific AO setups. It provides
 basic necessary functions to facilitate the control software itself, but
 does not implement anything specifically for AO. A bare example 
 implementation is provided as FOAM_Dummy to show the idea behind the 
 framework.
 
 \section foam_cmdline Command line arguments
 
 - -c or --config=FILE: configuration file
 - -v: increase verbosity
 - -q: decrease verbosity
 - --verb=LEVEL: set verbosity
 - --nodaemon: don't start network daemon
 - -s, --sighandle=0|1: toggle signal handling
 - -h or --help: show help
 - --version: show version info
 
 \section foam_cfg Configuration parameters
 
 The configuration file is read by foamctrl::parse(), see documentation there
 about configuration variables supported.
 
 \section foam_netio Network IO

 Networking commands supported are:
 
 - help (ok cmd help): show help
 - exit or quit or bye (ok cmd <cmd>) [ok client disconnected]: disconnect client
 - shutdown (ok cmd shutdown) [warn :shutting down now]: shutdown FOAM
 - broadcast <msg> (ok cmd broadcast) [ok broadcast <msg> :from <client>]: broadcast msg to all clients
 - verb <+|-|INT> [ok verb <LEVEL>]: set verbosity
 - get mode (ok mode <mode>): get runmode
 - get frames (ok frames <nframes>): get foamctrl::frames
 - get devices (ok devices <ndev> <dev1> <dev1>): get devices, see DeviceManager::getlist
 - mode <mode> (ok cmd mode <mode>): set runmode
 
 \section foam_stop Shutting down

 When a signal is received (or any other asynchronous event takes place), the
 following takes place:
 
 -# Set foamctrl::mode to AO_MODE_SHUTDOWN and signal this with 
 FOAM::mode_cond. 
 -# Use FOAM::stop_mutex as check to see if the main FOAM::listen() thread has
 stopped. 
 -# Once FOAM::listen() stops, it returns to main() which then exits, calling 
 the FOAM destructor. 
 -# From FOAM::~FOAM, we can now safely clean up the rest of the program since 
 we are now running synchronously (i.e. the main thread has been instructed to 
 stop).
 
 This solution allows the main listen() thread to finish its last iteration of 
 FOAM::open_loop() and FOAM::open_finish() such that in a real system the
 hardware can be stopped gracefully. Furthermore, since all destructors are 
 called (from FOAM::~FOAM, when the system is already stopped), these can also
 handle device-specific stop instructions.
 */
class FOAM {
private:
	void show_clihelp(const bool) const;//!< Show help on command-line syntax.
	int show_nethelp(const Connection *const connection, string topic, string rest); //!< Show help on network command usage
	void show_version() const;					//!< Show version information
	void show_welcome() const;					//!< Show welcome banner
	
	bool do_sighandle;									//!< Toggle for signal handling or not (default: yes)
	auto_ptr<SigHandle> sighandler;			//!< Signal handler object
	
	bool do_perflog;										//!< Toggle for performance logging
	auto_ptr<PerfLog> open_perf;				//!< Open-loop performance
	auto_ptr<PerfLog> closed_perf;			//!< Closed-loop performance
	
protected:
	// Properties set at start
	bool nodaemon;											//!< Run daemon or not
	bool error;													//!< Error flag
	Path conffile;											//!< Configuration file to use
	const Path execname;								//!< Executable name, i.e. Path(argv[0])

	struct {
		bool ok;													//!< Track whether a network command is ok or not
	} netio;

	Protocol::Server *protocol;					//!< Network control socket
	
	pthread::mutex mode_mutex;					//!< Network thread <-> main thread mutex/cond pair
	pthread::cond mode_cond;						//!< Network thread <-> main thread mutex/cond pair
	
	pthread::mutex stop_mutex;					//!< Mutex used to check if main loop has completed
	
	void openperf_addlog(const int i) const { if (do_perflog && open_perf.get() != NULL) open_perf.get()->addlog(i); }
	void closedperf_addlog(const int i) const { if (do_perflog && closed_perf.get() != NULL) closed_perf.get()->addlog(i); }

	/*!
	 @brief Run on new connection to FOAM
	 
	 @param [in] *conn Connection used for this event
	 @param [in] status Connection status (connect or disconnect)
	 */
	virtual void on_connect(const Connection * const conn, const bool status) const;
	
	/*!
	 @brief Run on new incoming message to FOAM
	 @param [in] *conn Connection the message was received on
	 @param [in] line The message received
	 
	 This is called when a new network message is received. Callback registred 
	 through protocol. This routine is virtual and can (and should) be overloaded
	 by the setup-specific child-class such that it can process setup-specific
	 commands in addition to generic commands.
	 */
	virtual void on_message(Connection * const conn, string line);

public:
	FOAM(int argc, char *argv[]);
	virtual ~FOAM() = 0;
	void stopfoam();										//!< Common cleanup code, used to stop on signals
	
	foamctrl *ptc;											//!< AO control class
	foam::DeviceManager *devices;							//!< Device/hardware management
	Io io;															//!< Terminal diagnostics output
	
	bool has_error() const { return error; } //!< Return error status
	
	int init();													//!< Initialize FOAM setup
	int parse_args(int argc, char *argv[]); //!< Parse command-line arguments
	int load_config();									//!< Load FOAM configuration (from arguments)
	int verify() const;									//!< Verify setup integrity (from configuration)
	void daemon();											//!< Start network daemon
	int listen();												//!< Start main FOAM control loop
	
	string mode2str(const aomode_t m) const;
	aomode_t str2mode(const string m) const;
	
	/*!
	 @brief Load setup-specific modules
	 
	 This routine should be defined in a child class of FOAM and should load
	 the modules necessary for operation. These typically include a wavefront 
	 sensor with camera and one or more wave front correctors, such as a DM or
	 tip-tilt mirror.
	 */
	virtual int load_modules() = 0;

	/*!
	 @brief Closed-loop wrapper, calling child routines.
	 
	 This is a bare closed loop routine that mostly calls closed_init() once 
	 at the start, then runs closed_loop() continuously, and finally runs
	 closed_finish() at the end.
	 */
	int mode_closed();
	
	/*!
	 @brief Closed-loop initialisation routines
	 
	 This is called at the start of the closed loop. Should be implemented in 
	 derived classes. See mode_closed() for details.
	 */
	virtual int closed_init() = 0;
	
	/*!
	 @brief Closed-loop body routine
	 
	 Called as the main body for the closed loop. Should be implemented in derived
	 classes. See mode_closed() for details.
	 */
	virtual int closed_loop() = 0;
	
	/*!
	 @brief Closed-loop finalising routine
	 
	 Called at the end of the closed loop. Should be implemented in derived 
	 classes. See mode_closed() for details.
	 */
	virtual int closed_finish() = 0;
	
	/*!
	 @brief Open-loop wrapper, calling child routines.
	 
	 This is a bare open loop routine that mostly calls open_init() once 
	 at the start, then runs open_loop() continuously, and finally runs
	 open_finish() at the end.
	 */	
	int mode_open();
	
	/*!
	 @brief Open-loop initialisation routine
	 
	 This is called at the start of the open loop. Should be implemented in 
	 derived classes. See mode_open() for details.
	 */
	virtual int open_init() = 0;

	/*!
	 @brief Open-loop body routine

	 Called as the main body for the open loop. Should be implemented in derived
	 classes. See mode_open() for details.
	 */
	virtual int open_loop() = 0;

	/*!
	 @brief Open-loop finalising routine

	 Called at the end of the open loop. Should be implemented in derived 
	 classes. See mode_open() for details.
	 */
	virtual int open_finish() = 0;
	
	/*!
	 @brief Calibration mode wrapper, calling child routines.

	 This routine is a little different from mode_open() and mode_closed() and 
	 simply calls calib() which should be implemented in derived classes.
	 */	
	int mode_calib();
	/*!
	 @brief Calibration routine, used to calibrate various system aspects
	 
	 This function should take care of all calibration of the system.
	 */
	virtual int calib() = 0;
};

#endif // HAVE_FOAM_H
