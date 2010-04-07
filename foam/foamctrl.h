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

#ifndef HAVE_FOAMCTRL_H
#define HAVE_FOAMCTRL_H

#include <time.h>
#include "wfs.h"
#include "wfc.h"
#include "io.h"
#include "foamcfg.h"
#include "config.h"

/*!
 @brief Struct for filter wheel identification (used by foamctrl)
 
 This datatype must be used by the user to configure the AO system.
 To do anything useful, FOAM must know what filterwheels you are using,
 and therefore you must fill in the (user) fields at the beginning.
 */
typedef struct {
	char *name;					//!< (user) Filterwheel name
	int nfilts;					//!< (user) Number of filters present in this wheel
	filter_t curfilt;		//!< (foam) Current filter in place
	filter_t *filters;	//!< (user) All filters present in this wheel
	int delay;          //!< (user) The time in seconds the filterwheel needs to adjust, which is used in a sleep() call to give the filterwheel time
	int id;             //!< (user) a unique ID to identify the filterwheel
} fwheel_t;


/*! 
 @brief Stores the control state of the AO system
 
 This struct is used to store several variables indicating the state of the AO system 
 which are shared between the different threads. The thread interfacing with user(s)
 can then read these variables and report them to the user, or change them to influence
 the behaviour.
 
 The struct should be configured by the user in the prime module c-file for useful operation.
 (user) fields must be configured by the user, (foam) fields should be left untouched,
 although it is generally safe to read these fields.
 
 The logfrac field is used to stop superfluous logging. See logInfo() and logDebug()
 documentation for details. Errors and warnings are always logged/displayed, as these
 shouldn't occur, and supressing these are generally unwanted.
 
 The datalog* variables can be used to do miscellaneous logging to, in addition to general
 operational details that are logged to the debug, info and error logfiles. Logging 
 to this file must be taken care of by the prime module, 
 
 Also take a look at wfs_t, wfc_t and fwheel_t.
 */
class foamctrl {
private:
	config *cfgfile;
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
	string confpath;							//!< Configuration path
	
	aomode_t mode;								//!< AO system mode, default AO_MODE_LISTEN
	calmode_t calmode;						//!< Calibration mode, default CAL_PINHOLE
	
	time_t starttime;							//!< (foam) Starting time
	time_t lasttime;							//!< (foam) End time
	long frames;									//!< (foam) Number of frames parsed
	int logfrac;									//!< (user) Produce verbose info every logfrac frames. Default 1000
	float fps;										//!< (foam) Current FPS
	
	int wfs_count;								//!< Number of WFSs in the system, default 0
	string *wfscfgs;							//!< WFS configuration files
	Wfs **wfs;										//!< WFS entities
	
	int wfc_count;								//!< Number of WFCs in the system, default 0
	string *wfccfgs;							//!< WFC configuration files
	Wfc **wfc;										//!< WFC entities
	
	int fw_count;									//!< Number of filterwheels in the system, default 0
	fwheel_t *filter;							//!< FW entities
};

#endif // HAVE_FOAMCTRL_H
