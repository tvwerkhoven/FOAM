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

int FOAM_dummy::calib(const string &calib_mode, const string &calib_opts) {
	io.msg(IO_DEB2, "FOAM_dummy::calib()=%s", calib_mode.c_str());
	string this_opts = calib_opts;

	if (calib_mode == "dummy") {
		protocol->broadcast(format("ok calib dummy :opts= %s", this_opts.c_str()));
	} else if (calib_mode == "hello") {
		protocol->broadcast(format("ok calib hello :hi there %s!", this_opts.c_str()));
	}
	else
		return -1;

	return 0;
}

void FOAM_dummy::on_message(Connection *conn, string line) {
	io.msg(IO_DEB2, "FOAM_dummy::on_message()");
	FOAM::on_message(conn, line);
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