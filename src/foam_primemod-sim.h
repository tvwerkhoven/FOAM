/*! 
	@file foam_modules-sim.h
	@author @authortim
	@date November 30 2007

	@brief This header file prototypes simulation functions.
*/
#ifndef FOAM_PRIMEMOD_SIM
#define FOAM_PRIMEMOD_SIM

#include "foam_cs_library.h"		// we link to the main program here (i.e. we use common (log) functions)
#include "foam_modules-sim.h"		// we need simulation routines
#include "foam_modules-sh.h"		// we need SH analysis routines
#include "foam_modules-pgm.h"		// we need PGM routines

// These are defined in foam_cs_library.c
extern control_t ptc;
//extern config_t cs_config;



#endif /* FOAM_PRIMEMOD_SIM */

