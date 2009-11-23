/*
 cam.cc -- generic camera input/output wrapper
 Copyright (C) 2008-2009 Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)
 
 This file is part of FOAM.
 
 FOAM is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 FOAM is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with FOAM.	If not, see <http://www.gnu.org/licenses/>.

 */
/*! 
 @file cam.cc 
 @author Tim van Werkhoven
 @brief camera input/output routines

*/

#include <string>

#include "io.h"
#include "cam.h"

extern Io *io;

Cam::Cam(int camtype) {
	cam = new cam_t;
	cam->type = camtype;
}

Cam::~Cam(void) {
}

int Cam::init() {
	if (!setup)
		return io->msg(IO_ERR, "Cannot init, camera not set up yet.");
	
	switch (cam->type) {
		case CAM_STATIC: {
			break;
		}
		default:
			return io->msg(IO_ERR, "Cannot init, unknown camera type.");
	}
	
	return 0;
}

Cam::getFrame() {
	
}