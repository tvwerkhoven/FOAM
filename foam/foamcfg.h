/*
 foamcfg.h -- configuration class for FOAM
 
 Copyright (C) 2009 Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)
 
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

#ifndef __FOAMCFG_H__
#define __FOAMCFG_H__

#include "types.h"
#include "config.h"

/*!
 @brief Runtime configuration class.
 
 This class stores relevant runtime configuration settings.
 */
class foamcfg {
	config *cfgfile;
	
	public:
	foamcfg();
	foamcfg(string &file);
	~foamcfg();
	
	int verify();
	int parse(string &file);
	
	string conffile;				//!< (user) configuration file to use
	string pidfile;					//!< (user) file to store PID to
	
	string listenip;				//!< (user) IP to listen on, default "0.0.0.0"
	string listenport;			//!< (user) port to listen on, default 1010
	
	string datadir;					//!< (user) path to data directory (pgm, fits files)
	
	string logfile;					//!< (user) file to log info messages to, (none)
	
	bool use_syslog; 				//!< (user) syslog usage flag, default no
	string syslog_prepend;	//!< (user) string to prepend to syslogs, default "foam"
	
	pthread_t *threads;			//!< (foam) this stores the thread ids of all threads created
	int nthreads;						//!< (foam) stores the number of threads in use
};

#endif // __FOAMCFG_H__
