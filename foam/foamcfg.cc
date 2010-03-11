/*
 foamcfg.cc -- FOAM configuration class
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

#include <syslog.h>
#include <string>

#include "foamcfg.h"
#include "config.h"
#include "io.h"

extern Io *io;

foamcfg::~foamcfg(void) {
	io->msg(IO_DEB2, "foamcfg::~foamcfg()");
	
	delete cfgfile;
	
	if (use_syslog) closelog();
}

foamcfg::foamcfg(void) {
	io->msg(IO_DEB2, "foamcfg::foamcfg()");
	err = 0;
}

foamcfg::foamcfg(string &file) {
	io->msg(IO_DEB2, "foamcfg::foamcfg()");
	err = 0;
	parse(file);
}

int foamcfg::parse(string &file) {
	io->msg(IO_DEB2, "foamcfg::parse()");
	
	// Configfile and path
	conffile = file;
	int idx = conffile.find_last_of("/");
	confpath = conffile.substr(0, idx);
	
	cfgfile = new config(conffile);
	
	// PID file
	pidfile = cfgfile->getstring("pidfile", "/tmp/foam.pid");
	
	// Datadir
	datadir = cfgfile->getstring("datadir", FOAM_DATADIR);
	if (datadir == ".") io->msg(IO_WARN, "datadir not set, using current directory.");
	else io->msg(IO_DEB1, "Datadir: '%s'.", datadir.c_str());
	
	// Daemon settings
	listenip = cfgfile->getstring("listenip", "0.0.0.0");
	io->msg(IO_DEB1, "IP: '%s'.", listenip.c_str());
	listenport = cfgfile->getstring("listenport", "1025").c_str();
	io->msg(IO_DEB1, "Port: '%s'.", listenport.c_str());
	
	// Syslog settings
	use_syslog = cfgfile->getbool("use_syslog", false);
	syslog_prepend = cfgfile->getstring("syslog_prepend", "foam");
	io->msg(IO_DEB1, "Use syslog: %d, prefix: '%s'.", use_syslog, syslog_prepend.c_str());
	if (use_syslog) openlog(syslog_prepend.c_str(), LOG_PID, LOG_USER);

	// Logfile settings
	logfile = cfgfile->getstring("logfile", "");
	if (logfile.length()) {
		if (logfile[0] != '/') logfile = datadir + "/" + logfile;
		io->setLogfile(logfile);
	}
	
	return 0;
}

int foamcfg::verify(void) {
	io->msg(IO_DEB2, "foamcfg::verify(void)");

	int ret = 0;
	
	// TODO: verify stuff here
	
	return ret;
}
