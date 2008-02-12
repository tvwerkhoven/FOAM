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
#define DM_MAXVOLT 1	// these are normalized and converted to the right values by drvSetActuator()
#define DM_MIDVOLT 0
#define DM_MINVOLT -1
#define FOAM_MODCALIB_DMIF_PRE "ao_dmif"

// PROTOTYPES //
/**************/

int modCalWFC(control_t *ptc, int wfc, int wfs);

#endif