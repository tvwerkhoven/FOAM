/*! 
	@file foam_modules-calib.h
	@author @authortim
	@date 2008-02-12

	@brief This header file prototypes functions used in calibration
*/

#ifndef FOAM_MODULES_CALIB
#define FOAM_MODULES_CALIB

// LIBRABRIES //
/**************/

#include "foam_cs_library.h"
#include "foam_modules-sh.h"
#include <sys/stat.h>

// DEPENDENCIES //
/****************/
extern int drvSetupHardware(control_t *ptc, aomode_t mode, calmode_t calmode);
extern int drvSetActuator(control_t *ptc, int wfc);

// PROTOTYPES //
/**************/

/*!
@brief Measure the WFC influence function and decompose it using SVD.

This function measures the influence function for a single
WFS for all WFCs and stores this in an influence matrix. After
that, this matrix is inverted using SVD and the seperate matrices
are stored in seperate files. These can later be read to memory
such that recalibration is not always necessary. Using this decomposition,
the control vectors for all WFCs for a given WFS can be calculated.
 
 This routine only makes sense for Shack-Hartmann wavefront sensors.
 
 @param [in] *ptc Pointer to the AO configuration
 @param [in] wfs The wfs to calibrate for
 @param [in] *shtrack Pointer to a SH tracker configuration
 @return EXIT_FAILURE upon failure, EXIT_SUCCESS otherwise 
*/
int calibWFC(control_t *ptc, int wfs, mod_sh_track_t *shtrack);

/*!
@brief Checks whether influence function calibration has been performed.
 
 This routine tries to read the file that calibWFC() should have written it's
 calibration to check whether this calibration has been performed.
 
 @param [in] *ptc Pointer to the AO configuration
 @param [in] wfs The wfs to calibrate for
 @param [in] *shtrack Pointer to a SH tracker configuration
 @return EXIT_FAILURE upon failure, EXIT_SUCCESS otherwise
*/
int calibWFCChk(control_t *ptc, int wfs, mod_sh_track_t *shtrack);

/*!
@brief Measure the reference displacement and store it.

 This function measures the reference displacement when all WFC's
 are set to zero for a certain WFS. These coordinates are
 then stored and used as a reference coordinate when correcting
 the wavefront. Make sure you are sending a flat wavefront to the Shack
 Hartmann wavefront sensor for this, i.e. by using a pinhole.
 
 @param [in] *ptc Pointer to the AO configuration
 @param [in] wfs The wfs to calibrate for
 @param [in] *shtrack Pointer to a SH tracker configuration
 @return EXIT_FAILURE upon failure, EXIT_SUCCESS otherwise
*/
int calibPinhole(control_t *ptc, int wfs, mod_sh_track_t *shtrack);

/*!
@brief Checks whether pinhole has been performed, and loads the calibration.
 
 @param [in] *ptc Pointer to the AO configuration
 @param [in] wfs The wfs to calibrate for
 @param [in] *shtrack Pointer to a SH tracker configuration
 @return EXIT_FAILURE upon failure, EXIT_SUCCESS otherwise 
*/
int calibPinholeChk(control_t *ptc, int wfs, mod_sh_track_t *shtrack);

/*!
@brief Used to SVD the influence matrix and store the result to file.

This routine uses singular value decomposition as provided by a GSL interface
 to BLAS to calculate the inverse of the influence matrix. We need this
 inverse matrix to calculate the control vectors for the WFC's, given
 the displacements measured on a certain WFS.
 
 @param [in] *ptc Pointer to the AO configuration
 @param [in] wfs The wfs to calibrate for
 @param [in] *shtrack Pointer to a SH tracker configuration
 @return EXIT_FAILURE upon failure, EXIT_SUCCESS otherwise 
*/
int calibSVDGSL(control_t *ptc, int wfs, mod_sh_track_t *shtrack);

#endif
