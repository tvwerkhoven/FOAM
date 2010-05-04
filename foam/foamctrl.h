/*
 foamctrl.h -- control class for the complete AO system
 Copyright (C) 2008--2010 Tim van Werkhoven <t.i.m.vanwerkhoven@xs4all.nl>
 
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

#ifndef HAVE_FOAMCTRL_H
#define HAVE_FOAMCTRL_H

#include <time.h>
#include "wfs.h"
#include "wfc.h"
#include "io.h"
#include "config.h"


/*! 
 @brief Stores the control state of the AO system
 
 This struct is used to store several variables indicating the state of the AO system.
 */
class foamctrl {
private:
	int err;
	Io &io;
	
public:
	foamctrl(Io &io);
	foamctrl(Io &io, string &file);
	~foamctrl(void);
	
	int parse(string &file);
	int verify();
	int error() { return err; }
	
	string conffile;							//!< Configuration file used
	string confpath;							//!< Configuration path (used for other config files)
	config *cfgfile;							//!< Parsed configuration settings
	string pidfile;								//!< file to store PID to (/tmp/foam.pid)
	
	string listenip;							//!< IP to listen on (0.0.0.0)
	string listenport;						//!< port to listen on (1025)
	
	string datadir;								//!< path to data directory (pgm, fits files) (./)
	
	string logfile;								//!< file to log info messages to (none)
	
	bool use_syslog; 							//!< syslog usage flag (no)
	string syslog_prepend;				//!< string to prepend to syslogs ("foam")
	
	aomode_t mode;								//!< AO system mode (AO_MODE_LISTEN)
	string calib;									//!< Calibration mode passed to FOAM (none)
	
	time_t starttime;							//!< FOAM start timestamp
	time_t lasttime;							//!< Last frame timestamp
	long frames;									//!< Number of frames parsed
};

#endif // HAVE_FOAMCTRL_H
