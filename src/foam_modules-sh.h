/*! 
	@file foam_modules-sh.h
	@author @authortim
	@date January 28, 2008

	@brief This header file prototypes functions used in SH wavefront sensing.
	
	\section License
	This file is released under the GPL.
*/

#ifndef FOAM_MODULES_SH
#define FOAM_MODULES_SH

// LIBRABRIES //
/**************/

#include <gsl/gsl_math.h>			// for small integer powers
#include "foam_cs_library.h"

// PROTOTYPES //
/**************/

/*!
@brief Selects suitable subapts to work with

This routine checks all subapertures and sees whether they are useful or not.
It can also 'erode' some apertures away from the edge or enforce a maximum
radius between any subaperture and the reference subaperture.

@param [in] *image The image that we need to look for subapts on
@param [in] res The pixel resolution of the image
@param [in] cells The cell-resolution (lenslet size) of the SH sensor
@param [out] *subc The coordinates of the tracker windows
@param [out] *apcoo The coordinates of the grid associated with a window
@param [out] &totnsubap The number of usable subapertures in the system
@param [in] samini The minimum intensity a useful subaperture should have
@param [in] samxr The maximum radius to enforce if positive, or the amount of subapts to erode if negative.
*/
int modSelSubapts(float *image, coord_t res, int cells[2], int (*subc)[2], int (*apcoo)[2], int *totnsubap, float samini, int samxr);


/*!
@brief Parses output from Shack-Hartmann WFSs.

This function takes the output from the drvReadSensor() routine (if the sensor is a
SH WFS) and preprocesses this sensor output (typically from a CCD) to be further
analysed by modCalcDMVolt(), which calculates the actual driving voltages for the
DM. 

@return EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
@param [in] *wfsinfo wfs_t struct holding relevant information for the current WFS
*/
int modParseSH(wfs_t *wfsinfo);

/*!
@brief Calculates the sum of absolute differences for two subapertures

The two subapertures must be of the same size. sae() will check \a len pixels starting at 
the pointers given to the function.

@param [in] *subapt pointer to the beginning of the subaperture to check
@param [in] *refapt pointer to the beginning of the reference subaperture
@param [in] len number of pixels to check (usually SX*SY, i.e. the total amount of pixels in a subapt)
@return Sum of absolute differences
*/
float sae(float *subapt, float *refapt, int len);

/*!
@brief Calibrates the image for dark- and flatfield, returns some statistics

This function takes the input image from a (SH-)WFS and returns the dark- and flatfield-corrected
image in corrim. It also rearranges the image so that further processing is easier. \a image is expected
in row-major format, so that pixel (x,y) is given by image[i*width + j]. The returned image \a corrim is in row-major
format per subaperture, meaning that subaperture sn starts at corrim[sn*SX*SY] with sn >= 0 and SX, SY the width and height of the
subaperture. 

@param [out] *corrim A pointer to the corrected image. This needs to be allocated (same size as *image)
@param [in] *image Image of the sensor output, row-major format
@param [in] *darkim Darkfield image in \a *corrim format (read above)
@param [in] *flatim Flatfield image in \a *corrim format (read above)
@param [out] *sum The summed intensity over the whole *image
@param [out] *max The maximum of the whole *image
@param [in] res the resolution of the big images (corrim, image, darkim, flatim)
@param [in] window The resolution of the individual subapertures, used to reformat *corrim
*/
void imcal(float *corrim, float *image, float *darkim, float *flatim, float *sum, float *max, coord_t res, coord_t window);

/*!
@brief Tracks the seeing using center of gravity tracking

This calculates the center of gravity of each subaperture. The coordinates will be relative to the center of the
tracking window, i.e. if the center of gravity coordinate is added to the tracking window position, it is again
centered around the real center of gravity. Note that this will only work for star-like images, extended
images will not work.

@param [in] *wfsinfo The wfs_t struct holding relevant info on the current WFS being analysed
@param [out] *aver The average intensity over all subapts wil be stored here
@param [out] *max The maximum intensity of all subapts will be stored here
@param [out] coords will hold the CoG coordinates relative to the center of the subaperture
*/
void modCogTrack(wfs_t *wfsinfo, float *aver, float *max, float coords[][2]);

/*!
@brief Calculates the controls to be sent to the various WFC, given displacements.

TODO: doc
*/
int modCalcCtrl(control_t *ptc, int wfs, int nmodes);

/*!
@brief Search for a maxium within (xc, yc) -> (xc+width, yc+height) in a certain wfs output.
*/
void modCogFind(wfs_t *wfsinfo, int xc, int yc, int width, int height, float samini, float *sum, float *cog);

/*!
@brief Tracks the seeing using correlation tracking (works on extended objects)

Work in progress as of 2008-01-28
*/
//void modCorrTrack(wfs_t *wfsinfo, float *aver, float *max, float coords[][2]);

/*!
@brief Process a reference image stored in *image, old refim *ref
TODO: doc
*/
//void procRef(wfs_t *wfsinfo, float *sharp, float *aver);

/*!
@brief Module to get a (new) reference image 
TODO: doc
*/
//void modGetRef(wfs_t *wfsinfo);	

#endif /* FOAM_MODULES_SH */
