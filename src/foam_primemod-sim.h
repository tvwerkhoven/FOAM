/*! 
	@file foam_primemod-sim.h
	@author @authortim
	@date 2008-03-12

	@brief This header file links the prime module `sim' to various modules.
*/
#ifndef FOAM_PRIMEMOD_SIM
#define FOAM_PRIMEMOD_SIM

#include "foam_cs_library.h"		// we link to the main program here (i.e. we use common (log) functions)
#include "foam_modules-sim.h"		// we need simulation routines
#include "foam_modules-sh.h"		// we need SH analysis routines
#include "foam_modules-pgm.h"		// we need PGM routines

// These are defined in foam_cs_library.c
// we need ptc for the global control of the AO
extern control_t ptc;
// we need cs_config for some meta configuration (i.e. when to print what info)
extern config_t cs_config;
// we need these two pthread_ things for communication between threads
extern pthread_mutex_t mode_mutex;
extern pthread_cond_t mode_cond;



#endif /* FOAM_PRIMEMOD_SIM */

