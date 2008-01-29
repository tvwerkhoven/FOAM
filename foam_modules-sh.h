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

#include "foam_cs_library.h"

// These are defined in foam_cs_library.c
extern control_t ptc;
extern config_t cs_config;
extern SDL_Surface *screen;	// Global surface to draw on

// DEFINITIONS //
/***************/

/*!
@brief Selects suitable subapts to work with

This routine checks all subapertures and sees whether they are useful or not.
It can also 'erode' some apertures away from the edge or enforce a maximum
radius between any subaperture and the reference subaperture.

@param [in] *image The sensor output image (i.e. SH camera).
@param [in] samini The minimum intensity a useful subaperture should have
@param [in] samxr The maximum radius to enforce if positive, or the amount of subapts to erode if negative.
@param [in] wfs The wavefront sensor to apply this to.
*/
void selectSubapts(float *image, float samini, int samxr, int wfs);


/*!
@brief Parses output from Shack-Hartmann WFSs.

This function takes the output from the drvReadSensor() routine (if the sensor is a
SH WFS) and preprocesses this sensor output (typically from a CCD) to be further
analysed by modCalcDMVolt(), which calculates the actual driving voltages for the
DM. 

@return EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
*/
int modParseSH(int wfs);

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
@param [in] wfs The WFS identifier to read the image from
@param [out] *sum The summed intensity over the whole *image
@param [out] *max The maximum of the whole *image
@param [in] window The size of the individual subapertures, used to reformat *corrim
*/
void imcal(float *corrim, float *image, float *darkim, float *flatim, int wfs, float *sum, float *max, int window[2]);

/*!
@brief Tracks the seeing using center of gravity tracking

This calculates the center of gravity of each subaperture. The coordinates will be relative to the center of the
tracking window, i.e. if the center of gravity coordinate is added to the tracking window position, it is again
centered around the real center of gravity. Note that this will only work for star-like images, extended
images will not work.

@param [in] wfs The WFS identifier to track the CoG of 
@param [out] *aver The average intensity over all subapts wil be stored here
@param [out] *max The maximum intensity of all subapts will be stored here
@param [out] coords will hold the CoG coordinates relative to the center of the subaperture
*/
void cogTrack(int wfs, float *aver, float *max, float coords[][2]);

/*!
@brief Tracks the seeing using correlation tracking (works on extended objects)

Work in progress as of 2008-01-28
*/
void corrTrack(int wfs, float *aver, float *max, float coords[][2]);

/*!
@brief Process a reference image stored in *image, old refim *ref
TODO: doc
*/
void procRefim(float *image, float *ref, float *sharp, float *aver);
/*!
@brief This draws all subapertures for a certain wfs

@param [in] wfs the wfs identifies to draw subapts for
@param *screen SDL_Surface to draw on
*/
int drawSubapts(int wfs, SDL_Surface *screen);

#endif /* FOAM_MODULES_SH */
