/*! 
	@file foam_modules-sim.h
	@author @authortim
	@date November 30 2007

	@brief This header file prototypes simulation functions.
*/
#ifndef FOAM_MODULES_DM
#define FOAM_MODULES_DM

#include "foam_cs_library.h"

// constants

#define	SOR_LIM	  (1.e-8)  // limit value for SOR iteration
//#define NACT      37       // number of actuators

// local prototypes

/*!
@brief Simulates the DM shape as function of input voltages.

This routine, based on response2.c by C.U. Keller, takes a boundarymask,
an actuatorpattern for the DM being simulated, the number of actuators
and the voltages as input. It then calculates the shape of the mirror
in the output variable image. Additionally, niter can be set to limit the 
amount of iterations (0 is auto).

@param [in] *boundarymask The pgm-file containing the boundary mask (aperture)
@param [in] *actuatorpat The pgm-file containing the DM-actuator pattern
@param [in] nact The number of actuators, must be the same as used in \a *actuatorpat
@param [in] *ctrl The control commands array, \a nact long
@param [in] niter The number of iterations, pass 0 for automatic choice
@param [out] *image The DM wavefront correction in um, 2d array.
@param [in] res The resolution of the image to be processed
@return EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
*/
int modSimDM(char *boundarymask, char *actuatorpat, int nact, float *ctrl, float *image, int res[2], int niter);

// TODO: document (simple function anyway)
int modSimTT(float *ctrl, float *image, int res[2]);

#endif /* FOAM_MODULES_SIM */
