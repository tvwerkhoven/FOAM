/*
 foamcfg.h -- FOAM configuration class
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

#ifndef HAVE_FOAMCFG_H
#define HAVE_FOAMCFG_H

#include "types.h"
#include "config.h"
#include "io.h"

/*!
 @brief Runtime configuration class.
 
 This class stores relevant runtime configuration settings.
 */
class foamcfg {
private:
	config *cfgfile;
	int err;
	Io &io;
	
public:
	foamcfg(Io &io);
	foamcfg(Io &io, string &file);
	~foamcfg();
	
	int verify();
	int parse(string &file);
	int error() { return err; }
	
	string conffile;				//!< configuration file to use
	string confpath;				//!< configuration path
	string pidfile;					//!< file to store PID to
	
	string listenip;				//!< IP to listen on, default "0.0.0.0"
	string listenport;			//!< port to listen on, default 1010
	
	string datadir;					//!< path to data directory (pgm, fits files)
	
	string logfile;					//!< file to log info messages to, (none)
	
	bool use_syslog; 				//!< syslog usage flag, default no
	string syslog_prepend;	//!< string to prepend to syslogs, default "foam"
	
	pthread_t *threads;			//!< this stores the thread ids of all threads created
	int nthreads;						//!< stores the number of threads in use
};

#endif // HAVE_FOAMCFG_H
