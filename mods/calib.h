/*
 Copyright (C) 2008 Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)
 
 This file is part of FOAM.
 
 FOAM is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 FOAM is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with FOAM.  If not, see <http://www.gnu.org/licenses/>.

 $Id$
 */
/*! 
	@file foam_modules-calib.h
	@author Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)
	@date 2008-07-15
 
	@brief This header file prototypes functions used in calibration
*/

#ifndef FOAM_MODULES_CALIB
#define FOAM_MODULES_CALIB

// LIBRABRIES //
/**************/

#include <sys/stat.h>
#include "types.h"
#include "foam.h"
#include "sh.h"

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
that, this matrix is inverted using SVD and the separate matrices
are stored in separate files. These can later be read to memory
such that recalibration is not always necessary. Using this decomposition,
the control vectors for all WFCs for a given WFS can be calculated.
 
 This routine only makes sense for Shack-Hartmann wavefront sensors.
 
 See notes in the preamble of foam_modules-calib.c on this function.
 
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
 Hartmann wavefront sensor for this, i.e. by using a pinhole somewhere.
 
 See notes in the preamble of foam_modules-calib.c on this function.
 
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
