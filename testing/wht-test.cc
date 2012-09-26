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
	// Init FOAM_dummy class
	FOAM_dummy fd(argc, argv);
		
	if (fd.init())
		exit(-1);

	fd.io.msg(IO_INFO, "Running test mode");
	fd.io.msg(IO_INFO, "Init WHT...");
	
	// Init WHT telescope interface
	WHT *wht;
	try {
		WHT *wht = new WHT(fd.io, fd.ptc, "wht-test", fd.ptc->listenport, Path("./wht-test.cfg"));
		fd.devices->add((foam::Device *) wht);
	} catch (std::runtime_error &e) {
		fd.io.msg(IO_ERR, "Failed to initialize WHT: %s", e.what());
		return 1;
	}

	sleep(1);
	Protocol::Client protocol("127.0.0.1", fd.ptc->listenport);
	protocol.connect();
	
	// Test commands
	protocol.write(format("wht-test set ccd_ang %lf", 117.0));

	protocol.write(("wht-test track tcs 51 52 9.9"));
	sleep(1);

	protocol.write(("wht-test track pixshift 10 10"));
	sleep(1);
	
	protocol.write(("wht-test track telshift -10 -10"));
	sleep(1);

	fd.listen();
	
	return 0;
}