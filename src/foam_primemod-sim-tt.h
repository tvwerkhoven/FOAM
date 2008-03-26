/*! 
	@file foam_primemod-sim.h
	@author @authortim
	@date 2008-03-12

	@brief This header file links the prime module `sim' to various modules.
*/
#ifndef FOAM_PRIMEMOD_SIM
#define FOAM_PRIMEMOD_SIM

// For foam_cs.c, this must hold some configuration for the WFS sensor etc:
#define FOAM_CONFIG_FILE "../config/ao_config-tt.cfg"

// For foam_modules.c, we need some files to do simulation with:
#define FOAM_MODSIM_WAVEFRONT "../config/wavefront.pgm"
#define FOAM_MODSIM_APERTURE "../config/apert15-256.pgm"
#define FOAM_MODSIM_APTMASK "../config/apert15-256.pgm"
#define FOAM_MODSIM_ACTPAT "../config/dm37-256.pgm"

#define FOAM_MODSIM_WINDX 0		//!< Simulated wind in x direction
#define FOAM_MODSIM_WINDY 0 	//!< Simulated wind in y direction
#define FOAM_MODSIM_SEEING 50	//!< Seeing factor to use (0.1 is good seeing, 0.5 is really bad)

#define FOAM_MODSIM_ERR 0		//!< Introduce artificial WFC error?
#define FOAM_MODSIM_ERRWFC 0	//!< Introduce error using which WFC?
#define FOAM_MODSIM_ERRTYPE 1	//!< Error type: 1 sawtooth, 2 random drift
#define FOAM_MODSIM_ERRVERB 1	//!< Be verbose about error simulation?

#include "foam_cs_library.h"		// we link to the main program here (i.e. we use common (log) functions)
#include "foam_modules-sim.h"		// we need simulation routines
#include "foam_modules-sh.h"		// we need SH analysis routines
#include "foam_modules-img.h"		// we need img routines

// These are defined in foam_cs_library.c
// we need ptc for the global control of the AO
extern control_t ptc;
// we need cs_config for some meta configuration (i.e. when to print what info)
extern config_t cs_config;
// we need these two pthread_ things for communication between threads
extern pthread_mutex_t mode_mutex;
extern pthread_cond_t mode_cond;



#endif /* FOAM_PRIMEMOD_SIM */

