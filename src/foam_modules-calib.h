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

//#define NS 256 // maximum number of subapts (TODO: do we need this?)

// PROTOTYPES //
/**************/

int modCalWFC(control_t *ptc, int wfc, int wfs);

int modCalPinhole(control_t *ptc, int wfs);
#endif
