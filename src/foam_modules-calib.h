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
#include <sys/stat.h>

// DEPENDENCIES //
/****************/
extern int drvFilterWheel(control_t *ptc, fwheel_t mode);
extern int drvSetActuator();

// PROTOTYPES //
/**************/

/*!
@brief Measure the WFC influence function and invert it.

This function measures the influence function for a single
WFS for all WFCs and stores this in an influence matrix. After
that, this matrix is inverted using SVD and the seperate matrices
are stored in seperate files. These can later be read to memory
and using these SVD'd matrices, the control vectors for all WFCs
for a given WFS can be calculated.
*/
int modCalWFC(control_t *ptc, int wfs);

/*!
@brief Checks whether modCalWFC has been performed.
*/
int modCalWFCChk(control_t *ptc, int wfs);

/*!
@brief Measure the reference displacement and store it.

This function measures the reference displacement when all WFC's
are set to zero for a certain WFS. These coordinates are
then stored and used as a reference coordinate when correcting
the wavefront.
*/
int modCalPinhole(control_t *ptc, int wfs);

/*!
@brief Checks whether modCalPinhole has been performed.
*/

int modCalPinholeChk(control_t *ptc, int wfs);

/*!
@brief Used to SVD the influence matrix and store the result to file.

This routine uses singular value decomposition as provided by GSL
to calculate the inverse of the influence matrix. We need this
inverse matrix to calculate the control vectors for the WFC's, given
the displacements measured on a certain WFS.
*/
int modSVDGSL(control_t *ptc, int wfs);

/*!
@brief This function tests the linearity of the WFCs.
*/
//int modLinTest(control_t *ptc, int wfs);

#endif
