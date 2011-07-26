/*
 foamctrl.h -- control class for the complete AO system
 Copyright (C) 2008--2011 Tim van Werkhoven <t.i.m.vanwerkhoven@xs4all.nl>
 
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

#include "path++.h"
#include "io.h"
#include "config.h"
#include "foamtypes.h"

/*! 
 @brief Stores the control state of the AO system
 
 This class is used to store several variables indicating the state of the AO 
 system, including logging, networking, terminal I/O, pidfiles, etc. At 
 startup it reads the general configuration from a file using 
 foamctrl::parse().
 
 Supported configuration parameters include: (with defaults between square brackets)
 - datadir (relative to system data dir) [system dir]
 - outdir (relative to progdir if set)
 - listenip [0.0.0.0]
 - listenport [1025]
 - use_syslog [false]
 - syslog_prepend [foam]
 - logfile (relative to outdir) [foam.log]
 
 */
class foamctrl {
private:
	int err;											//!< Error flag
	Io &io;												//!< Terminal logging
	
	void make_path(const char *dir);
	
public:
	foamctrl(Io &io, Path const file=Path(""));
	~foamctrl(void);
	
	int parse();									//!< Parse configuration file. See foamctrl:: for more info.
	int verify();									//!< Verify whether settings are sane
	int error() const { return err; } //!< Return error status
	
	Path progdir;									//!< Path of the program executable (Path(argv[0]).dirname())
	
	Path conffile;								//!< Configuration file used
	Path confdir;									//!< Configuration dir (used for other config files)
	config *cfg;									//!< Parsed configuration settings
	Path pidfile;									//!< file to store PID to (def: /tmp/foam.pid)
	
	string listenip;							//!< IP to listen on (def: 0.0.0.0)
	string listenport;						//!< port to listen on (def: 1025)
	
	Path datadir;									//!< path to data directory (to read configuration data from, such as wavefront files etc.)
	Path outdir;									//!< path to data output directory (to store data)
	
	Path logfile;									//!< file to log info messages to (def: none)
	
	bool use_syslog; 							//!< syslog usage flag (def: no)
	string syslog_prepend;				//!< string to prepend to syslogs (def: "foam")
	
	aomode_t mode;								//!< AO system mode (def: AO_MODE_LISTEN)
	string calib;									//!< Calibration mode passed to FOAM (def: none)
	
	time_t starttime;							//!< FOAM start timestamp
	time_t lasttime;							//!< Last frame timestamp
	size_t frames;								//!< Number of frames parsed
};

#endif // HAVE_FOAMCTRL_H
