/*
 alpaodm-test.cc -- test alpao dm routines
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
#include "alpaodm.h"

using namespace std;

int main() {
	printf("Init Io...\n");
	Io io(10);
	
	io.msg(IO_INFO, "Init foamctrl...");
	foamctrl ptc(io);
	
	io.msg(IO_INFO, "Init AlpaoDM...");
	AlpaoDM *alpao_dm97;
	try {
		alpao_dm97 = new AlpaoDM(io, &ptc, "alpao_dm97-test", "1234", Path("./alpaodm-test.cfg"), true);
	} catch (std::runtime_error &e) {
		io.msg(IO_ERR | IO_FATAL, "AlpaoDM: problem init: %s", e.what());
		return 1;
	} catch (...) {
		io.msg(IO_ERR, "AlpaoDM: unknown error during init");
		return 1;
	}

	io.msg(IO_INFO, "Init complete, setting %d actuators to 0.12 one by one...", 
				 alpao_dm97->get_nact());
	
	for (size_t idx=0; idx<alpao_dm97->get_nact(); idx++) {
		// Set all to 0
		alpao_dm97->set_control(0.0);
		// set idx to 0.12
		alpao_dm97->set_control_act(0.0, idx);
		// Actuate mirror
		alpao_dm97->actuate();
		// Sleep 1
		sleep(1);
	}
	
	io.msg(IO_INFO, "Quitting now...");
	delete alpao_dm97;
	
	io.msg(IO_INFO, "Program exit in 5 seconds...");
	sleep(5);
	
	return 0;
}