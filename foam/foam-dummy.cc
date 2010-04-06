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
	FOAM_dummy(int argc, char *argv[]): FOAM(argc, argv) { printf("FOAM_dummy::FOAM_dummy()\n"); } 
	virtual ~FOAM_dummy() { printf("FOAM_dummy::~FOAM_dummy\n"); } 
	
	virtual bool load_modules() { printf("FOAM_dummy::load_modules\n"); return true; } 
	virtual bool on_message() { printf("FOAM_dummy::on_message\n"); return true; } 
		
	virtual bool closed_init() { printf("FOAM_dummy::closed_init\n"); return true; }
	virtual bool closed_loop()  { printf("FOAM_dummy::closed_loop\n"); return true; }
	virtual bool closed_finish() { printf("FOAM_dummy::closed_finish\n"); return true; }
	
	virtual bool open_init() { printf("FOAM_dummy::open_init\n"); return true; }
	virtual bool open_loop() { printf("FOAM_dummy::open_loop\n"); return true; }
	virtual bool open_finish() { printf("FOAM_dummy::open_finish\n"); return true; }
	
	virtual bool calib() { printf("FOAM_dummy::calib\n"); return true; }
};


int main(int argc, char *argv[]) {
	printf("FOAM_dummy.\n");
	
	// Init FOAM_dummy class
	FOAM_dummy foam(argc, argv);
	
	foam.init();
	
	foam.io.msg(IO_INFO, "Running dummy mode");
	
	foam.listen();
	
	return 0;
}
