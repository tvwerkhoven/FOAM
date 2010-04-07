/*
 FOAM_dummy.h -- dummy module
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
	@file FOAM_dummy.c
	@author Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)
	@date November 30 2007

	@brief This is a dummy module to test the bare FOAM framework capabilities.
*/

// HEADERS //
/***********/

#include "foam.h"

class FOAM_dummy : public FOAM {
public:
	FOAM_dummy(int argc, char *argv[]): FOAM(argc, argv) { io.msg(IO_DEB2, "FOAM_dummy::FOAM_dummy()"); } 
	virtual ~FOAM_dummy() { io.msg(IO_DEB2, "FOAM_dummy::~FOAM_dummy()"); } 
	
	virtual bool load_modules() { io.msg(IO_DEB2, "FOAM_dummy::load_modules()"); return true; } 
	virtual void on_message(Connection *connection, std::string line) { FOAM::on_message(connection, line); io.msg(IO_DEB2, "FOAM_dummy::on_message()"); } 
		
	virtual bool closed_init() { io.msg(IO_DEB2, "FOAM_dummy::closed_init()"); return true; }
	virtual bool closed_loop()  { io.msg(IO_DEB2, "FOAM_dummy::closed_loop()"); return true; }
	virtual bool closed_finish() { io.msg(IO_DEB2, "FOAM_dummy::closed_finish()"); return true; }
	
	virtual bool open_init() { io.msg(IO_DEB2, "FOAM_dummy::open_init()"); return true; }
	virtual bool open_loop() { io.msg(IO_DEB2, "FOAM_dummy::open_loop()"); return true; }
	virtual bool open_finish() { io.msg(IO_DEB2, "FOAM_dummy::open_finish()"); return true; }
	
	virtual bool calib() { io.msg(IO_DEB2, "FOAM_dummy::calib()"); return true; }
};


int main(int argc, char *argv[]) {
	// Init FOAM_dummy class
	FOAM_dummy foam(argc, argv);
	
	foam.init();
	
	foam.io.msg(IO_INFO, "Running dummy mode");
	
	foam.listen();
	
	return 0;
}
