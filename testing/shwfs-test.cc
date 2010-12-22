/*
 shwfs-test.cc -- test shwfs class
 Copyright (C) 2010 Tim van Werkhoven <t.i.m.vanwerkhoven@xs4all.nl>
 
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

#include "io.h"
#include "shwfs.h"
#include "dummycam.h"
#include "foamctrl.h"
#include "simulcam.h"

using namespace std;

int main(int argc, char *argv[]) {
	Io io(10);
	io.msg(IO_INFO, "Init Io");
	Path tmp("");

	io.msg(IO_INFO, "Init foamctrl");
	foamctrl ptc(io);
	
	if (argc <= 1) {
		io.msg(IO_INFO, "Have argc<=1: will do dummycam");
		
		io.msg(IO_INFO, "Init DummyCamera");
		DummyCamera wfscam(io, &ptc, "wfscam", "12345", tmp);
		
		io.msg(IO_INFO, "Init Shwfs");
		Shwfs shwfs(io, &ptc, "shwfs-test", "12345", tmp, wfscam);
		Path mlaout("./mla_grid");
		shwfs.store_mla_grid(mlaout, true);
	}
	else {
		io.msg(IO_INFO, "Have argc>1: will do simulcam with configuration file '%s'", argv[1]);
	
		tmp = argv[1];
		io.msg(IO_INFO, "Init SimulCam with %s", tmp.c_str());
		SimulCam wfscam(io, &ptc, "wfscam", "12345", tmp);
				
		sleep(1);

		Path mlaout("./mla_grid");
		wfscam.shwfs.store_mla_grid(mlaout, true);
		
// Take one frame and store it
		wfscam.set_store(1);
		wfscam.cam_set_mode(Camera::SINGLE);
		
		sleep(1);
	}
	return 0;
}