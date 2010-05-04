/*
 foamctrl.h -- control class for complete AO system
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

#include <time.h>
#include <syslog.h>

#include "foamctrl.h"
#include "types.h"
#include "config.h"
#include "io.h"

foamctrl::~foamctrl(void) {
	io.msg(IO_DEB2, "foamctrl::~foamctrl(void)");

	if (use_syslog) closelog();
	delete cfgfile;
}

foamctrl::foamctrl(Io &io): 
err(0), io(io),
conffile(""), pidfile("/tmp/foam.pid"), 
listenip("0.0.0.0"), listenport("1025"),
datadir(FOAM_DATADIR), logfile("foam-log"),
use_syslog(false), syslog_prepend("foam"), 
mode(AO_MODE_LISTEN), calib(""),
starttime(time(NULL)), frames(0)
{ 
	io.msg(IO_DEB2, "foamctrl::foamctrl(void)");
}

foamctrl::foamctrl(Io &io, string &file): 
err(0), io(io), conffile(file),
mode(AO_MODE_LISTEN), calib(""),
starttime(time(NULL)), frames(0)
{
	io.msg(IO_DEB2, "foamctrl::foamctrl()");
	
	parse(file);
}

int foamctrl::parse(string &file) {
	io.msg(IO_DEB2, "foamctrl::parse(f=%s)", file.c_str());

	int idx = conffile.find_last_of("/");
	confpath = conffile.substr(0, idx);
	
	cfgfile = new config(conffile);
	
	// PID file
	pidfile = cfgfile->getstring("pidfile", "/tmp/foam.pid");
	
	// Datadir
	datadir = cfgfile->getstring("datadir", FOAM_DATADIR);
	if (datadir == ".") io.msg(IO_WARN, "datadir not set, using current directory.");
	else io.msg(IO_DEB1, "Datadir: '%s'.", datadir.c_str());
	
	// Daemon settings
	listenip = cfgfile->getstring("listenip", "0.0.0.0");
	io.msg(IO_DEB1, "IP: '%s'.", listenip.c_str());
	listenport = cfgfile->getstring("listenport", "1025").c_str();
	io.msg(IO_DEB1, "Port: '%s'.", listenport.c_str());
	
	// Syslog settings
	use_syslog = cfgfile->getbool("use_syslog", false);
	syslog_prepend = cfgfile->getstring("syslog_prepend", "foam");
	io.msg(IO_DEB1, "Use syslog: %d, prefix: '%s'.", use_syslog, syslog_prepend.c_str());
	if (use_syslog) openlog(syslog_prepend.c_str(), LOG_PID, LOG_USER);
	
	// Logfile settings
	logfile = cfgfile->getstring("logfile", "");
	if (logfile.length()) {
		if (logfile[0] != '/') logfile = datadir + "/" + logfile;
		io.setLogfile(logfile);
		io.msg(IO_DEB1, "Logfile: %s.", logfile.c_str());

	}
	
	io.msg(IO_INFO, "Successfully parsed control config.");
	return 0;
}

int foamctrl::verify() {
	int ret=0;
	
	return ret;
}
