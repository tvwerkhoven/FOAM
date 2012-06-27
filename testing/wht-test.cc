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
#include <unistd.h>

#include "serial.h"
#include "socket.h"

#include "foam-dummy.h"

#include "io.h"
#include "telescope.h"
#include "wht.h"


using namespace std;

int main(int argc, char *argv[]) {
	printf("Init Io...\n");
	Io io(10);
	
	// Make dummy FOAM to parse arguments and allow the GUI to connect
	FOAM_dummy fd(argc, argv);
	
	if (fd.init())
		exit(-1);
	
	string port = fd.ptc->listenport;

	fd.io.msg(IO_INFO, "Init WHT...");
	WHT *wht;
	try {
		wht = new WHT(fd.io, fd.ptc, "wht-test", port, Path("./wht-test.cfg"), true);
	} catch (std::runtime_error &e) {
		io.msg(IO_ERR, "Failed to initialize WHT: %s", e.what());
		return 1;
	}
	sleep(1);
	
	io.msg(IO_INFO, "Init complete, sending test (0,0) and (1,1).");
	
	wht->set_track_offset(0, 0);
	usleep(0.5 * 1E6);

	wht->set_track_offset(1, 1);
	usleep(0.5 * 1E6);

	io.msg(IO_INFO, "WHT instance listening on port %s. ^C to stop.", port.c_str());

	while (true) {
		sleep(1);
	}
	
	io.msg(IO_INFO, "Quitting now...");
	delete wht;
	
	io.msg(IO_INFO, "Program exit in 1 seconds...");
	sleep(1);
	
	return 0;
}