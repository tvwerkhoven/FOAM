/*
 foamcontrol.h -- FOAM control connection 
 Copyright (C) 2009--2011 Tim van Werkhoven <werkhoven@strw.leidenuniv.nl>
 
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

#ifndef HAVE_FOAMCONTROL_H
#define HAVE_FOAMCONTROL_H

#include <string>
#include <map>
#include <vector>
#include <glibmm/dispatcher.h>

#include "config.h"
#include "protocol.h"
#include "pthread++.h"
#include "foamtypes.h"
#include "log.h"

//! @todo Mutual inclusion of DevicePage/DeviceCtrl and FoamControl problem (both DevicePage/DeviceCtrl need FoamControl, and the latter need the two former in a struct for handling devices)

class DevicePage;
class DeviceCtrl;

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
		string name;											//!< Device name
		string type;											//!< Device type, hierarchical (i.e. dev.cam.simulcam)
		DevicePage *page;									//!< GUI element for this device
		DeviceCtrl *ctrl;									//!< Control element for this device
		_device_t(string _n="undef", string _t="undef"): name(_n), type(_t), page(NULL), ctrl(NULL) { ; }
	} device_t;													//!< Describes hardware devices
	
private:
	string mode2str(const aomode_t m) const {
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
	
	aomode_t str2mode(const string m) const {
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
	config cfg;													//!< GUI configuration (position etc.)
	
	string conffile;										//!< Configuration file
	string execname;										//!< Executable name (argv[0])

	pthread::mutex mutex;
	
	struct state_t {
		aomode_t mode;										//!< AO mode (see aomode_t)
		int numdev;												//!< Number of devices connected
		device_t devices[32];							//!< List of devices
		uint64_t numframes;								//!< Number of frames processed
		vector<string> calmodes;					//!< Different calibration modes
		string lastreply;									//!< Last reply (stored in on_message())
		string lastcmd;										//!< Last command issued to FOAM
		state_t(): mode(AO_MODE_UNDEF), numdev(0), numframes(0), lastreply("undef"), lastcmd("undef") { ; }
	} state;														//!< Basic state of the remote AO system
	
	bool ok;														//!< Status of the system
	string errormsg;										//!< If not ok, this was the error
	
	void on_message(string line);				//!< Callback for new messages from FOAM
	void on_connected(bool conn);				//!< Callback for new connection to FOAM
	
	void show_version();
	void show_clihelp(const bool error = false);
	int parse_args(int argc, char *argv[]);
	
public:
	string host;
	string port;
	
	pthread::mutex gui_mutex;
	
	FoamControl(Log &log, string &conffile, string &execname);
	~FoamControl() { };
	
	int connect(const string &host, const string &port);
	int disconnect();
	void send_cmd(const string &cmd);
	
	// Device management
	bool add_device(const string name, const string type);
	bool rem_device(const string name);
	device_t *get_device(const string name);
	device_t *get_device(const DevicePage *page);
	device_t *get_device(const int i) { return &(state.devices[i]); }

	// get-like commands
	string getpeername() { return protocol.getpeername(); }
	string getsockname() { return protocol.getsockname(); }
	
	int get_numdev() const { return (const int) state.numdev; }
	
	uint64_t get_numframes() const { return (const uint64_t) state.numframes; }
	aomode_t get_mode() const { return (const aomode_t) state.mode; }
	string get_mode_str() const { return mode2str(state.mode); }

	size_t get_numcal() const { return state.calmodes.size(); }
	string get_calmode(const size_t i) const { return state.calmodes[i]; }

	string get_lastreply() const { return state.lastreply; }
	string get_lastcmd() const { return state.lastcmd; }
	
	// set-like commands
	void set_mode(aomode_t mode);
	void shutdown() { send_cmd("shutdown"); }
	void calibrate(const string &calmode) { send_cmd(format("calib %s", calmode.c_str())); }
	void calibrate(const string &calmode, const string &opt) { send_cmd(format("calib %s %s", calmode.c_str(), opt.c_str())); }

	
	bool is_ok() { return ok; }
	bool is_connected() { return protocol.is_connected(); }
	string get_errormsg() { return errormsg; }
	
	Glib::Dispatcher signal_connect;
	Glib::Dispatcher signal_message;
	Glib::Dispatcher signal_device;
};

#include "deviceview.h"
#include "devicectrl.h"



#endif // HAVE_FOAMCONTROL_H
