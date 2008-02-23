/*! 
	@file foam_modules-calib.h
	@author @authortim
	@date 2008-02-12

	@brief This header file prototypes functions used in calibration
	
	TODO: document
	
	\section License
	This file is released under the GPL.
*/

#ifndef FOAM_MODULES_CALIB
#define FOAM_MODULES_CALIB

// LIBRABRIES //
/**************/

#include "foam_cs_library.h"
#include <sys/stat.h>

// DEPENDENCIES //
/****************/
extern int drvFilterWheel(control_t *ptc, fwheel_t mode);
extern int drvSetActuator();

// PROTOTYPES //
/**************/

int modCalWFC(control_t *ptc, int wfs);

int modCalPinhole(control_t *ptc, int wfs);

int modSVD(control_t *ptc, int wfs);

#endif
