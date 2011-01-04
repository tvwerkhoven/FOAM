/*
 foamctrl.h -- control class for complete AO system
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

/*! 
 @file foamctrl.cc
 @author Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)
 @brief The FOAM control class, keeps track of logging, networking, terminal 
 I/O, pidfiles, etc.
 */

#include <time.h>
#include <stdio.h>  /* defines FILENAME_MAX */
#include <syslog.h>
#include <unistd.h>

#include "path++.h"
#include "foamctrl.h"
#include "types.h"
#include "config.h"
#include "io.h"

foamctrl::~foamctrl(void) {
	io.msg(IO_DEB2, "foamctrl::~foamctrl(void)");

	if (use_syslog) closelog();
	delete cfg;
}

foamctrl::foamctrl(Io &io): 
err(0), io(io),
conffile(""), pidfile("/tmp/foam.pid"), 
listenip("0.0.0.0"), listenport("1025"),
datadir("/tmp/"), logfile("foam-log"),
use_syslog(false), syslog_prepend("foam"), 
mode(AO_MODE_LISTEN), calib(""),
starttime(time(NULL)), frames(0)
{ 
	io.msg(IO_DEB2, "foamctrl::foamctrl(void)");
}

foamctrl::foamctrl(Io &io, Path &file): 
err(0), io(io), conffile(file),
mode(AO_MODE_LISTEN), calib(""),
starttime(time(NULL)), frames(0)
{
	io.msg(IO_DEB2, "foamctrl::foamctrl()");
	io.msg(IO_DEB2, "foamctrl::foamctrl() %s", conffile.c_str());
	parse();
}

int foamctrl::parse() {
	io.msg(IO_DEB2, "foamctrl::parse()");
	
	char curdir[FILENAME_MAX];
	progdir = string(getcwd(curdir, sizeof curdir));

	// Get absolute path of configuration file (reference for further relative paths
	confdir = progdir + conffile.dirname();
	io.msg(IO_DEB1, "Confdir: '%s', file: '%s'", confdir.c_str(), conffile.basename().c_str());
	cfg = new config(conffile);
	
	// Datadir (relative to confdir)
	datadir = confdir + cfg->getstring("datadir", "/tmp/");
	if (datadir == confdir + ".") io.msg(IO_WARN, "datadir not set, using current directory.");
	else io.msg(IO_DEB1, "Datadir: '%s'.", datadir.c_str());

	// PID file (relative to confdir)
	pidfile = cfg->getstring("pidfile", "/tmp/foam.pid");
	if (pidfile.isrel())
		pidfile = datadir + pidfile;
	io.msg(IO_DEB1, "Pidfile: '%s'.", pidfile.c_str());
	
	// Daemon settings
	listenip = cfg->getstring("listenip", "0.0.0.0");
	io.msg(IO_DEB1, "IP: '%s'.", listenip.c_str());
	listenport = cfg->getstring("listenport", "1025").c_str();
	io.msg(IO_DEB1, "Port: '%s'.", listenport.c_str());
	
	// Syslog settings
	use_syslog = cfg->getbool("use_syslog", false);
	syslog_prepend = cfg->getstring("syslog_prepend", "foam");
	io.msg(IO_DEB1, "Use syslog: %d, prefix: '%s'.", use_syslog, syslog_prepend.c_str());
	if (use_syslog) openlog(syslog_prepend.c_str(), LOG_PID, LOG_USER);
	
	// Logfile settings
	logfile = cfg->getstring("logfile", "");
	if (logfile.length()) {
		// If a log file was given, init the logging to datadir
		logfile = datadir + logfile;
		io.setLogfile(logfile);
		io.msg(IO_DEB1, "Logfile: %s.", logfile.c_str());
	}
	else
		io.msg(IO_DEB1, "Not logging to disk for now...");

	
	io.msg(IO_INFO, "Successfully parsed control config.");
	return 0;
}

int foamctrl::verify() {
	int ret=0;
	
	return ret;
}
