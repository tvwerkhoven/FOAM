/*
 foam-hwtest.h -- hardware testing mode
 Copyright (C) 2008--2011 Tim van Werkhoven <werkhoven@strw.leidenuniv.nl>
 
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


#ifndef HAVE_FOAM_SIMSTATIC_H
#define HAVE_FOAM_SIMSTATIC_H

#define FOAM_CONFIG_PRE "foam-simstat"

#include "config.h"

// LIBRARIES //
/*************/

#include "foam.h"
#include "types.h"
#include "io.h"

using namespace std;

/*!
 @brief FOAM hardware test implementation
*/
class FOAM_hwtest : public FOAM {
public:
	FOAM_hwtest(int argc, char *argv[]): FOAM(argc, argv) { io.msg(IO_DEB2, "FOAM_hwtest::FOAM_hwtest()"); } 
	virtual ~FOAM_hwtest() { io.msg(IO_DEB2, "FOAM_hwtest::~FOAM_hwtest()"); } 
	
	virtual int load_modules();
	virtual void on_message(Connection *connection, string line);
	
	virtual int closed_init();
	virtual int closed_loop();
	virtual int closed_finish();
	
	virtual int open_init();
	virtual int open_loop();
	virtual int open_finish();
	
	virtual int calib();
};

#endif // HAVE_FOAM_SIMSTATIC_H

