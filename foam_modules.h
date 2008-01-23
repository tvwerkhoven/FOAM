/*! 
	@file foam_modules.h
	@author Tim van Werkhoven
	@date November 30 2007

	@brief This header file prototypes driver and module functions.
*/
#ifndef FOAM_MODULES
#define FOAM_MODULES

#include <fftw3.h> // we need this for modParseSH()
#include <fitsio.h> // we need this to read FITS files

/*!
@brief Reads out the sensor(s) and outputs to ptc.wfs[n].image.

During simulation, this function takes care of simulating the atmosphere, 
the telescope, the tip-tilt mirror, the DM and the (SH) sensor itself, since
these all occur in sequence.

@return EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
*/
int drvReadSensor();

/*!
@brief This sets the various actuators (WFCs).

@return EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
*/
int drvSetActuator();


/*!
@brief Simulates the SH sensor

TODO: add info
*/
int modSimSH();

/*!
@brief Calculates DM output voltages, which are in turn set by drvSetActuator().

@return EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
*/
int modCalcDMVolt();

/*!
@brief Simulates the DM shape as function of input voltages.

This routine, based on response2.c by C.U. Keller, takes a boundarymask,
an actuatorpattern for the DM being simulated, the number of actuators
and the voltages as input. It then calculates the shape of the mirror
in the output variable image. Additionally, niter can be set to limit the 
amount of iterations (0 is auto).

@params [in] *boundarymask The pgm-file containing the boundary mask (aperture)
@params [in] *actuatorpat The pgm-file containing the DM-actuator pattern
@params [in] nact The number of actuators, must be the same as used in \a *actuatorpat
@params [in] *voltage The voltage array, \a nact long
@params [in] niter The number of iterations, pass 0 for automatic choice
@params [out] *dm The DM wavefront correction in um.
@return EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
*/
int simDM(char *boundarymask, char *actuatorpat, int nact, float *ctrl, float *image, int niter);

/*!
@brief This routine, currently not used, can be used to simulate an object (e.g. wavefront)
*/
int simObj(char *file, float *image);

/*!
@brief \a simAtm() reads a fits file with simulated atmospheric distortion.

This fuction works in wavefront-space.
*/
int simAtm(char *file, long res[2], long origin[2], float *image);

/*!
@brief \a simTel() simulates the effect of the telescope (aperture) on the output of \a simAtm().

This fuction works in wavefront-space, and basically multiplies the aperture function with
the wavefront from \a simAtm().
*/
int simTel(char *file, long res[2], float *image);

/*!
@brief \a simWFC() simulates a certain wave front corrector, like a TT or a DM.

This fuction works in wavefront-space.
*/
int simWFC(int wfcid, int nact, float *ctrl, float *image);



#endif /* FOAM_MODULES */

