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


#include <time.h>
#include <stdio.h>  // defines FILENAME_MAX
#include <syslog.h>
#include <unistd.h>
#include <string.h>

#include "path++.h"
#include "foamctrl.h"
#include "types.h"
#include "config.h"
#include "io.h"

foamctrl::foamctrl(Io &io, Path const file): 
err(0), io(io),
conffile(file), 
cfg(NULL),
listenip("0.0.0.0"), 
listenport("1025"),
datadir("./"), 
outdir("./"), 
logfile("foam-log"),
use_syslog(false), 
syslog_prepend("foam"), 
mode(AO_MODE_LISTEN), 
calib(""),
starttime(time(NULL)), 
frames(0)
{ 
	io.msg(IO_DEB2, "foamctrl::foamctrl(file=%s)", conffile.c_str());
	if (conffile.length())
		parse();
}

foamctrl::~foamctrl(void) {
	io.msg(IO_DEB2, "foamctrl::~foamctrl(void)");
	
	if (use_syslog) 
		closelog();
	
	// Store updated configuration (in a different file)
	if (cfg) {
		cfg->write(conffile + ".autosave");
		delete cfg;
	}
	io.msg(IO_DEB2, "foamctrl::~foamctrl() complete");
}

int foamctrl::parse() {
	io.msg(IO_DEB2, "foamctrl::parse()");
	
	char curdir[FILENAME_MAX];
	progdir = string(getcwd(curdir, sizeof curdir));
	io.msg(IO_INFO, "Progdir: '%s'", progdir.c_str());
	
	// Get absolute path of configuration file (reference for further relative paths
	confdir = progdir + conffile.dirname();
	io.msg(IO_INFO, "Confdir: '%s', file: '%s'", confdir.c_str(), conffile.basename().c_str());
	cfg = new config(conffile);
	
	// Datadir: always system default
	datadir = FOAM_DATADIR;
	io.msg(IO_INFO, "Datadir: '%s'.", datadir.c_str());

	// Outdir, store data here. Create subdirectory unique for this run
	// if not set: use FOAM_DATADIR as root
	// if set: relative to progdir (if relative), else absolute as root
	if (cfg->exists("outdir"))
		outdir = progdir + cfg->getstring("outdir");
	else 
		outdir = FOAM_DATADIR;

	struct tm *tmp = gmtime(&starttime);
	char tstamp[16];
	strftime(tstamp, sizeof(tstamp), "%Y%m%d_%H%M%S", tmp);
	outdir += format("FOAM_data_%s/", tstamp);
	make_path(outdir.c_str());
	io.msg(IO_INFO, "Output dir: '%s'.", outdir.c_str());	
	
	// Daemon settings
	listenip = cfg->getstring("listenip", "0.0.0.0");
	io.msg(IO_INFO, "IP: '%s'.", listenip.c_str());
	listenport = cfg->getstring("listenport", "1025");
	io.msg(IO_INFO, "Port: '%s'.", listenport.c_str());
	
	// Syslog settings
	use_syslog = cfg->getbool("use_syslog", false);
	syslog_prepend = cfg->getstring("syslog_prepend", "foam");
	if (use_syslog) 
		openlog(syslog_prepend.c_str(), LOG_PID, LOG_USER);
	io.msg(IO_INFO, "Use syslog: %d, prefix: '%s'.", use_syslog, syslog_prepend.c_str());
	
	// Logfile settings
	logfile = cfg->getstring("logfile", "foam.log");
	if (logfile.length()) {
		// If a log file was given, init the logging to datadir
		logfile = outdir + logfile;
		io.setLogfile(logfile);
		io.msg(IO_INFO, "Logfile: %s.", logfile.c_str());
	}
	else
		io.msg(IO_INFO, "Not logging to disk for now...");

	
	io.msg(IO_XNFO, "Successfully parsed control config.");
	return 0;
}

int foamctrl::verify() {
	int ret=0;
	
	return ret;
}

// From: <http://nion.modprobe.de/blog/archives/357-Recursive-directory-creation.html>
void foamctrl::make_path(const char *dir) {
	char tmp[512];
	char *p = NULL;
	size_t len;

	snprintf(tmp, sizeof(tmp),"%s",dir);
	len = strlen(tmp);
	
	if(tmp[len - 1] == '/')
		tmp[len - 1] = 0;
	
	for(p = tmp + 1; *p; p++)
		if(*p == '/') {
			*p = 0;
			mkdir(tmp, S_IRWXU);
			*p = '/';
		}
	
	mkdir(tmp, S_IRWXU);
}
