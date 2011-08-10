/*
 andor-test.cc -- test andor camera routines
 Copyright (C) 2011 Tim van Werkhoven <t.i.m.vanwerkhoven@xs4all.nl>
 
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

#include <string>
#include <vector>

#include "path++.h"
#include "io.h"
#include "andor.h"

using namespace std;

int main() {
	Io io(10);
	io.msg(IO_INFO, "Init Io");
	
	io.msg(IO_INFO, "Init foamctrl");
	foamctrl ptc(io);
	
	io.msg(IO_INFO, "Init AndorCam");
	AndorCam *ixoncam;
	try {
		ixoncam = new AndorCam(io, &ptc, "andorcam-test", "1234", Path("./andor-test.cfg"), true);
	} catch (...) {
		io.msg(IO_ERR, "Failed to initialize AndorCam, deleting!");
		delete ixoncam;
		exit(-1);
	}
	
	io.msg(IO_INFO, "Init complete, printing capabilities");
	ixoncam->print_andor_caps();
	
	io.msg(IO_INFO, "Quitting now...");
	delete ixoncam;
	
	sleep(5);
	
	return 0;
}