/*
 FOAM_dummy.h -- dummy module
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

// HEADERS //
/***********/

#include "foam-dummy.h"

using namespace std;

FOAM_dummy::FOAM_dummy(int argc, char *argv[]): FOAM(argc, argv) {
	io.msg(IO_DEB2, "FOAM_dummy::FOAM_dummy()");
	// Register calibration modes
	
	calib_modes["dummy"] = calib_mode("dummy", "this is a dummy calibration mode", "", true);
	calib_modes["hello"] = calib_mode("hello", "calibratin says hello", "<name>", true);
}

void FOAM_dummy::on_message(Connection *conn, string line) {
	io.msg(IO_DEB2, "FOAM_dummy::on_message()");
	bool parsed = true;
	string orig = line;
	string cmd = popword(line);
	
	if (cmd == "calib") {					// calib <calmode> <calopts>
		conn->write("ok cmd calib");
		string calmode = popword(line);
		string calopts = line;
		// Check if we know this calibration mode
		if (calib_modes.find(calmode) != calib_modes.end()) {
			if (calmode == "dummy")
				conn->write(format("ok calib %s :opts %s", calmode.c_str(), calopts.c_str()));
			else if (calmode == "hello")
				conn->write(format("ok calib %s :hi there %s!", calmode.c_str(), calopts.c_str()));
		}
		else {
			conn->write("err calib :calib mode not found");
		}
	} else {
		parsed = false;
	}
	
	
	if (!parsed)
		FOAM::on_message(conn, orig);
}

int main(int argc, char *argv[]) {
	// Init FOAM_dummy class
	FOAM_dummy foam(argc, argv);
	
	if (foam.init())
		exit(-1);

	foam.io.msg(IO_INFO, "Running dummy mode");
	foam.listen();
	
	return 0;
}