/*
 Copyright (C) 2008 Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)
 
 This file is part of FOAM.
 
 FOAM is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 FOAM is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with FOAM.  If not, see <http://www.gnu.org/licenses/>.
 */
/*! 
	@file foam-dummy.c
	@author Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)
	@date November 30 2007

	@brief This is a dummy module to test the bare FOAM framework capabilities.
*/

// HEADERS //
/***********/

#include "foam.h"
#include "io.h"

extern pthread_mutex_t mode_mutex;
extern pthread_cond_t mode_cond;
extern Io *io;

int modInitModule(foamctrl *ptc, foamcfg *cs_config) {
  io->msg(IO_INFO, "Running in dummy mode, don't expect great AO results :)");
	return EXIT_SUCCESS;
}

int modPostInitModule(foamctrl *ptc, foamcfg *cs_config) {
	return EXIT_SUCCESS;
}

int modOpenInit(foamctrl *ptc) {
	return EXIT_SUCCESS;
}
void modStopModule(foamctrl *ptc) {
	// placeholder ftw!
}

int modOpenLoop(foamctrl *ptc) {
	return EXIT_SUCCESS;
}

int modOpenFinish(foamctrl *ptc) {
	return EXIT_SUCCESS;
}


int modClosedInit(foamctrl *ptc) {
	return EXIT_SUCCESS;
}

int modClosedLoop(foamctrl *ptc) {
	return EXIT_SUCCESS;
}

int modClosedFinish(foamctrl *ptc) {
	return EXIT_SUCCESS;
}

int modCalibrate(foamctrl *ptc) {
	return EXIT_SUCCESS;
}

int modMessage(foamctrl *ptc, Connection *connection, string cmd, string rest) {
	return 0;
}
