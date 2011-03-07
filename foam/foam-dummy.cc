/*
 FOAM_dummy.h -- dummy module
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

// HEADERS //
/***********/

#include "foam.h"
using namespace std;

/*!
 @brief FOAM dummy implementation
 
 This FOAM implementation has all virtual functions from the FOAM base 
 class, but does nothing else than report which function is being called.
 This class is a simple example of how to implement your own specific
 AO setup with FOAM.
 
 FOAM is the base class that can be derived to specific AO setups. It provides
 basic necessary functions to facilitate the control software itself, but
 does not implement anything specifically for AO. A bare example 
 implementation is provided as foam-dummy to show the idea behind the 
 framework.
 
 Extra command line arguments supported are:
 - none
 
 Extra networking commands supported are:
 - none
 */
class FOAM_dummy : public FOAM {
public:
	FOAM_dummy(int argc, char *argv[]): FOAM(argc, argv) { io.msg(IO_DEB2, "FOAM_dummy::FOAM_dummy()"); } 
	virtual ~FOAM_dummy() { io.msg(IO_DEB2, "FOAM_dummy::~FOAM_dummy()"); } 
	
	virtual int load_modules() { io.msg(IO_DEB2, "FOAM_dummy::load_modules()"); return 0; } 
	virtual void on_message(Connection *connection, string line) { FOAM::on_message(connection, line); io.msg(IO_DEB2, "FOAM_dummy::on_message()"); } 
		
	virtual int closed_init() { io.msg(IO_DEB2, "FOAM_dummy::closed_init()"); return 0; }
	virtual int closed_loop()  { io.msg(IO_DEB2, "FOAM_dummy::closed_loop()"); return 0; }
	virtual int closed_finish() { io.msg(IO_DEB2, "FOAM_dummy::closed_finish()"); return 0; }
	
	virtual int open_init() { io.msg(IO_DEB2, "FOAM_dummy::open_init()"); return 0; }
	virtual int open_loop() { io.msg(IO_DEB2, "FOAM_dummy::open_loop()"); return 0; }
	virtual int open_finish() { io.msg(IO_DEB2, "FOAM_dummy::open_finish()"); return 0; }
	
	virtual int calib() { io.msg(IO_DEB2, "FOAM_dummy::calib()"); return 0; }
};


int main(int argc, char *argv[]) {
	// Init FOAM_dummy class
	FOAM_dummy foam(argc, argv);
	
	if (foam.init())
		exit(-1);

	foam.io.msg(IO_INFO, "Running dummy mode");
	foam.listen();
	
	return 0;
}
