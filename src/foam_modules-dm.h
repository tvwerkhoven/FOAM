/*! 
	@file foam_modules-dm.h
	@author @authortim
	@date November 30 2007

	@brief This header file prototypes simulation functions.
*/
#ifndef FOAM_MODULES_DM
#define FOAM_MODULES_DM

#include "foam_cs_library.h"
#include <math.h>

#define	SOR_LIM	  (1.e-8)  // limit value for SOR iteration

/*!
@brief Simulates the DM shape as function of input voltages.

This routine, based on response2.c by C.U. Keller, takes a boundarymask,
an actuatorpattern for the DM being simulated, the number of actuators
and the voltages as input. It then calculates the shape of the mirror
in the output variable image. Additionally, niter can be set to limit the 
amount of iterations (set to 0 if unsure).\n
\n
The routine updates the image store in the pointer \a image and so there should
already be memory allocated for this variable. \a ctrl should be an array of actuator
controls between -1 and 1. This routine automatically converts this to the corresponding 
`voltages'.\n
\n
This routine uses some global variables *resp, *boundary, *act and *actvolt which
hold various images. *resp will hold the calculated mirror response, which will be applied
to image. *boundary holds the aperture used for the calculation, *act holds the actuator pattern
and *actvolt holds the actuator pattern multiplied with a value depending on the voltage.
These variables are global so the files only have to be read in ones, and the memory only needs
to be allocated once.


@param [in] *boundarymask The pgm-file containing the boundary mask (aperture)
@param [in] *actuatorpat The pgm-file containing the DM-actuator pattern
@param [in] nact The number of actuators, must be the same as used in \a *actuatorpat
@param [in] *ctrl The control commands array, \a nact long
@param [in] niter The number of iterations, pass 0 for automatic choice
@param [out] *image The wavefront to update with the DM response
@param [in] res The resolution of the image to be processed
@return EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
*/
int modSimDM(char *boundarymask, char *actuatorpat, int nact, gsl_vector_float *ctrl, float *image, coord_t res, int niter);

// TODO: document (simple function anyway)
/*!
@brief Simulates a Tip-Tilt mirror

Ctrl should be between -1 and 1, should be linear, and is a 2 element array.
Like modSimDM, this routine updates the image stored in \a image. It again
must already be allocated. \a res is the resolution of the image.\n
\n
Tilting itself is done as follows:
The image is multiplied by values ranging from -amp to +amp over the whole resolution (res[0] or res[1]
depending on the direction), with 0 in the middle. This slope introduced over the image is multiplied
with ctrl[0] and ctrl[1] for the x and y directions. If ctrl = {0,0}, no tip-tilting occurs.

@param [in] *ctrl A 2-element array with the controls for the tip-tilt mirror
@param [out] *image A 2d array holding the wavefront to be tip-tilted
@param [in] res The resolution of the image
@return EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
*/
int modSimTT(gsl_vector_float *ctrl, float *image, coord_t res);

#endif /* FOAM_MODULES_SIM */
