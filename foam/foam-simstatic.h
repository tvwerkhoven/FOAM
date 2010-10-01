/*
 foam-simstatic.h -- static simulation module header file
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
/*! 
	@file foam-simstatic.h
	@author Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)
	@date 2008-04-18 12:55

*/

#ifndef HAVE_FOAM_SIMSTATIC_H
#define HAVE_FOAM_SIMSTATIC_H

#define FOAM_CONFIG_PRE "foam-simstat"

#include "config.h"
#include "path++.h"

// LIBRARIES //
/*************/

#include "foam.h"
#include "types.h"
#include "io.h"

/*!
 @brief FOAM static simulation implementation
 
 This FOAM implementation provides a simple static simulation. The WFS camera
 is fed by an image stored on disk and the shifts calculated that way are 
 thus static.
 */
class FOAM_simstatic : public FOAM {
public:
	FOAM_simstatic(int argc, char *argv[]): FOAM(argc, argv) { io.msg(IO_DEB2, "FOAM_simstatic::FOAM_simstatic()"); } 
	virtual ~FOAM_simstatic() { io.msg(IO_DEB2, "FOAM_simstatic::~FOAM_simstatic()"); } 
	
	virtual int load_modules();
	virtual void on_message(Connection *connection, std::string line);
	
	virtual int closed_init();
	virtual int closed_loop();
	virtual int closed_finish();
	
	virtual int open_init();
	virtual int open_loop();
	virtual int open_finish();
	
	virtual int calib();
};

#endif // HAVE_FOAM_SIMSTATIC_H

