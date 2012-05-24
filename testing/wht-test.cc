/*
 wht-test.cc -- test WHT interface
 Copyright (C) 2012 Tim van Werkhoven <werkhoven@strw.leidenuniv.nl>
 
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

#include "serial.h"
#include "socket.h"
#include "io.h"
#include "telescope.h"
#include "wht.h"

using namespace std;

int main() {
	printf("Init Io...\n");
	Io io(10);
	
	io.msg(IO_INFO, "Init foamctrl...");
	foamctrl ptc(io);
	
	io.msg(IO_INFO, "Init WHT...");
	WHT *wht;
	try {
		wht = new WHT(io, &ptc, "wht-test", "1234", Path("./wht-test.cfg"), true);
	} catch (...) {
		io.msg(IO_ERR, "Failed to initialize WHT, deleting & aborting!");
		return 1;
	}
	sleep(1);
	
	io.msg(IO_INFO, "Init complete.");
	while (true) {
		float c0=drand48(), c1=drand48();
		wht->set_track_offset(c0, c1);
		sleep(1);
	}
	
	io.msg(IO_INFO, "Quitting now...");
	delete wht;
	
	io.msg(IO_INFO, "Program exit in 1 seconds...");
	sleep(1);
	
	return 0;
}