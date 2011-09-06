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
#include <memory>
#include <algorithm>

#include <unistd.h>

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
	auto_ptr<AlpaoDM> alpao_dm97;
	try {
		alpao_dm97 = auto_ptr<AlpaoDM> (new AlpaoDM(io, &ptc, "alpao_dm97-test", "1234", Path("./alpaodm-test.cfg"), true));
	} catch (std::runtime_error &e) {
		io.msg(IO_ERR | IO_FATAL, "AlpaoDM: problem init: %s", e.what());
		throw;
	} catch (...) {
		io.msg(IO_ERR, "AlpaoDM: unknown error during init");
		throw;
	}

	io.msg(IO_INFO, "Init complete, starting tests now.");

	
	io.msg(IO_INFO, "Test0: Setting all %d actuators to 0.12...", alpao_dm97->get_nact());
	// Set all to 0.12
	alpao_dm97->set_control(0.12);
	// Actuate mirror
	alpao_dm97->actuate();
	sleep(2);

	io.msg(IO_INFO, "Test1: Setting all %d actuators to -0.12...", alpao_dm97->get_nact());
	// Set all to 0.12
	alpao_dm97->set_control(-0.12);
	// Actuate mirror
	alpao_dm97->actuate();
	sleep(2);

	io.msg(IO_INFO, "Test2: Setting %d actuators to 0.12 one by one...", alpao_dm97->get_nact());
	
	for (size_t idx=0; idx< (size_t) alpao_dm97->get_nact(); idx++) {
		if ((idx % 10 == 0 && idx > 0) || idx == alpao_dm97->get_nact()-1)
			io.msg(IO_INFO | IO_NOID, "");

		if (idx % 10 == 0)
			io.msg(IO_INFO | IO_NOLF, "Setting actuators %d--%d: .", 
						 idx, 
						 min((size_t) alpao_dm97->get_nact(), (idx+10)));
		else
			io.msg(IO_INFO | IO_NOLF | IO_NOID, ".");
		
		// Set all to 0
		alpao_dm97->set_control(0.0);
		// set idx to 0.12
		alpao_dm97->set_control_act(0.0, idx);
		// Actuate mirror
		alpao_dm97->actuate();
		// Sleep 1
		usleep(0.4 * 1E6);
	}
	
	io.msg(IO_INFO, "Test3: Setting waffle pattern...");
	alpao_dm97->set_wafflepattern(0.12);
	sleep(2);
	//io.msg(IO_INFO, "Quitting now...");
	//delete alpao_dm97;
	
	io.msg(IO_INFO, "Program exit in 2 seconds...");
	sleep(2);
	
	return 0;
}