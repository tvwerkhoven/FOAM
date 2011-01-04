/*
 foamcontrol.h -- FOAM control connection 
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
/*!
 @file foamcontrol.h
 @author Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)
 
 @brief This is the FOAM control connection class
 */

#ifndef HAVE_FOAMCONTROL_H
#define HAVE_FOAMCONTROL_H

#include <string>
#include <map>
#include <glibmm/dispatcher.h>

#include "protocol.h"
#include "pthread++.h"
#include "foamtypes.h"
#include "log.h"

using namespace std;

/*!
 @brief Main FOAM control class.

 This class takes care of the base connection to FOAM. It controls common 
 functions of all FOAM setups allows GUI elements to register callbacks to
 several signals that notify changes in the system. It locally keeps track 
 of the remote AO system setup as well.
 */
class FoamControl {
public:
	typedef struct _device_t {
		string name;
		string type;		
	} device_t;													//!< Describes hardware devices
	
private:
	string mode2str(aomode_t m) {
		switch (m) {
			case AO_MODE_OPEN: return "open";
			case AO_MODE_CLOSED: return "closed";
			case AO_MODE_CAL: return "calib";
			case AO_MODE_LISTEN: return "listen";
			case AO_MODE_UNDEF: return "undef";
			case AO_MODE_SHUTDOWN: return "shutdown";
			default: return "unknown";
		}
	}
	
	aomode_t str2mode(string m) {
		if (m == "open") return AO_MODE_OPEN;
		else if (m == "closed") return AO_MODE_CLOSED;
		else if (m == "calib") return AO_MODE_CAL;
		else if (m == "listen") return AO_MODE_LISTEN;
		else if (m == "undef") return AO_MODE_UNDEF;
		else if (m == "shutdown") return AO_MODE_SHUTDOWN;
		else return AO_MODE_UNDEF;
	}
	
	Protocol::Client protocol;					//!< Connection used to FOAM
	Log &log;														//!< Reference to MainWindow::log

	pthread::mutex mutex;
	
	struct state_t {
		aomode_t mode;										//!< AO mode (see aomode_t)
		int numdev;												//!< Number of devices connected
		device_t devices[32];							//!< List of devices
		uint64_t numframes;								//!< Number of frames processed
		int numcal;												//!< Number of calibration modes
		string calmodes[32];							//!< Different calibration modes
		string lastreply;									//!< Last reply (stored in on_message())
		string lastcmd;										//!< Last command issued to FOAM
	} state;														//!< Basic state of the remote AO system
	
	bool ok;														//!< Status of the system
	string errormsg;										//!< If not ok, this was the error
	
	void on_message(string line);				//!< Callback for new messages from FOAM
	void on_connected(bool conn);				//!< Callback for new connection to FOAM
	
public:
	string host;
	string port;
	
	FoamControl(Log &log);
	~FoamControl() { };
	
	int connect(const string &host, const string &port);
	int disconnect();
	void send_cmd(const string &cmd);
	
	// get-like commands
	string getpeername() { return protocol.getpeername(); }
	string getsockname() { return protocol.getsockname(); }
	int get_numdev() { return state.numdev; }
	void set_numdev(int n) { state.numdev = n; }
	uint64_t get_numframes() { return state.numframes; }
	aomode_t get_mode() { return state.mode; }
	string get_mode_str() { return mode2str(state.mode); }
	int get_numcal() { return state.numcal; }
	string get_calmode(int i) { return state.calmodes[i]; }
	device_t get_device(int i ) { return state.devices[i]; }
	string get_lastreply() { return state.lastreply; }
	string get_lastcmd() { return state.lastcmd; }
	
	// set-like commands
	void set_mode(aomode_t mode);
	void shutdown() { send_cmd("shutdown"); }
	void calibrate(string calmode) { send_cmd(format("calib %s", calmode.c_str())); }

	
	bool is_ok() { return ok; }
	bool is_connected() { return protocol.is_connected(); }
	string get_errormsg() { return errormsg; }
	
	Glib::Dispatcher signal_connect;
	Glib::Dispatcher signal_message;
	Glib::Dispatcher signal_device;
};


#endif // HAVE_FOAMCONTROL_H
