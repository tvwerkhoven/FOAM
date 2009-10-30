/*
 Copyright (C) 2008 Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)
 
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

 $Id$
*/ 
/*! 
	@file libfoam.c
	@brief Library file for @name
	@author @authortim
	@date November 13 2007

	This file contains things necessary to run the @name that are 
	not related to adaptive optics itself. This includes things like networking,
	info/error logging, etc. 
 
	Note that 'cs' originated from 'control software', which was the name of 
	the software controlling AO before @name was thought of.
*/

#include "libfoam.h"

control_t ptc = { //!< Global struct to hold system characteristics and other data. Initialize with complete but minimal configuration
	.mode = AO_MODE_LISTEN,
	.calmode = CAL_INFL,
	.wfs_count = 0,
	.wfc_count = 0,
	.fw_count = 0,
	.frames = 0,
	.logfrac = 1000,
	.capped = 0,
};

config_t cs_config = { //!< Global struct to hold system configuration. Init with complete but minimal configuration
	.listenip = "0.0.0.0", 	// listen on any IP by default, can be overridden by config file
	.listenport = 10000,	// listen on port 10000 by default
	.infofile = NULL,
	.infofd = NULL,
	.errfile = NULL,
	.errfd = NULL,
	.debugfile = NULL,
	.debugfd = NULL,
	.use_syslog = false,
	.syslog_prepend = "foam",
	.use_stdout = true,
	.loglevel = LOGDEBUG,
	.nthreads = 0
};

conntrack_t clientlist;
struct event_base *sockbase;

