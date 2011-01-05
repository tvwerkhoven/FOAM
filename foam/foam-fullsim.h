/*
 foam-full.h -- static simulation module header file
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
	@file foam-simstatic.h
	@author Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)
	@date 2008-04-18 12:55

*/

#ifndef HAVE_FOAM_FULLSIM_H
#define HAVE_FOAM_FULLSIM_H

#define FOAM_CONFIG_PRE "foam-fullsim"


// LIBRARIES //
/*************/

#ifdef HAVE_CONFIG_H
#include "autoconfig.h"
#endif

#include "foam.h"
#include "types.h"
#include "io.h"

/*!
 @brief FOAM full simulation implementation
 
 This FOAM implementation provides a complete end-to-end simulation of the 
 atmosphere, the telescope, the wavefront correctors and -sensors.
 
 
 Extra command line arguments supported are:
 - none
 
 Extra networking commands supported are:
 - help (ok cmd help): show more help
 - get calib (ok calib <ncalib> <calib1> <calib2> ...): get calibration mdoes
 - calib <calib> (ok cmd calib): calibrate setup
 */
class FOAM_FullSim : public FOAM {
public:
	FOAM_FullSim(int argc, char *argv[]): FOAM(argc, argv) { io.msg(IO_DEB2, "FOAM_FullSim::FOAM_FullSim()"); } 
	virtual ~FOAM_FullSim() { io.msg(IO_DEB2, "FOAM_FullSim::~FOAM_FullSim()"); } 
	
	virtual int load_modules();
	virtual void on_message(Connection * const connection, std::string line);
	
	virtual int closed_init();
	virtual int closed_loop();
	virtual int closed_finish();
	
	virtual int open_init();
	virtual int open_loop();
	virtual int open_finish();
	
	virtual int calib();
};

#endif // HAVE_FOAM_FULLSIM_H

