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

 $Id$
 */
/*! 
	@file foam-simstatic.h
	@author Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)
	@date 2008-04-18 12:55

*/

#ifndef FOAM_PRIME_SIMSTATIC
#define FOAM_PRIME_SIMSTATIC

// We always use config.h
#include "config.h"
#include "foam.h"
#include "types.h"

#define FOAM_CONFIG_PRE "foam-static"

// ROUTINE PROTOTYPES //
/**********************/

// These *must* be defined in a prime module
int drvSetupHardware(control_t *ptc, aomode_t aomode, calmode_t calmode);
int drvSetActuator(control_t *ptc, int wfc);
int drvGetImg(control_t *ptc, int wfs);

// LIBRARIES //
/*************/

#ifdef FOAM_SIMSTAT_DISPLAY
// for displaying stuff (SDL)
#include "dispcommon.h"
#endif

// for image file I/O
#include "img.h"
// for calibrating the image lateron
#include "calib.h"

// These are simstatic specific (for the time being)
int MMAvgFramesByte(control_t *ptc, gsl_matrix_float *output, wfs_t *wfs, int rounds);
int MMDarkFlatFullByte(wfs_t *wfs, mod_sh_track_t *shtrack);
int MMDarkFlatSubapByte(wfs_t *wfs, mod_sh_track_t *shtrack);


#endif // #ifndef FOAM_PRIME_SIMSTATIC

