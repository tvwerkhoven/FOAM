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
#include <gsl/gsl_linalg.h>			// for vectors and stuff
#include "foam_cs_library.h"

// DATATYPES //
/*************/

/*!
 @brief This stores information on SH tracking
 
 The things prefixed by '(user)' must be supplied by the user immediately (i.e.
 hardcode it in or read some configuration file). The (mod) things will be calculcated
 by this or other modules. (runtime) is something the user can change during runtime.
 */
typedef struct {
	int nsubap;						//!< (mod) amount of subapertures used (coordinates stored in subc)
	int skipframes;					//!< (user) amount of frames to skip before measuring WFC influence
	int measurecount;				//!< (user) amount of measurements to average for WFC influence
	
	coord_t cells;					//!< (user) number of cells in this SH WFS (i.e. lenslet resolution)
	coord_t shsize;					//!< (user) pixel resolution per cell
	coord_t track;					//!< (user) tracker window resolution in pixels (i.e. 1/2 of shsize per definition)
	int samxr;					//!< (user) use this for edge erosion or to force maximum distance from reference subapt
	float samini;					//!< (user) use this value as a minimum intensity for valid subapertures
	
	gsl_vector_float *singular;		//!< (mod) stores singular values from SVD (nact big)
	gsl_matrix_float *dmmodes;		//!< (mod) stores dmmodes from SVD (nact*nact big)
	gsl_matrix_float *wfsmodes;		//!< (mod) stores wfsmodes from SVD (nact*nsubap*2 big)
	
	coord_t *subc;					//!< (mod) this will hold the coordinates of each sub aperture
	coord_t *gridc;					//!< (mod) this will hold the grid origin for a certain subaperture
	gsl_vector_float *refc;			//!< (mod) reference displacements (i.e. definition of the origin)
	gsl_vector_float *disp;			//!< (mod) measured displacements (compare with refence for actual shift)
	
	fcoord_t stepc;					//!< (runtime) add this to the reference displacement during correction
	
	char *pinhole;					//!< (user) base filename to store the pinhole calibration (stored in *refc)
	char *influence;				//!< (user) base filename to store the influence matrix (stored in singular/dmmodes/wfsmodes)
} mod_sh_track_t;

// PROTOTYPES //
/**************/

/*!
 @brief Initialize the SH module for a certain configuration.

 This routine allocates some data for you so you don't have to :) 
 Call this before using any routines in this module.
 @param [in] *shtrack A pre-filled mod_sh_track_t struct with the SH sensor configuration
 */
int modInitSH(mod_sh_track_t *shtrack);

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
 @param [out] *totnsubap The number of usable subapertures in the system
 @param [in] samini The minimum intensity a useful subaperture should have
 @param [in] samxr The maximum radius to enforce if positive, or the amount of subapts to erode if negative.
 */
int modSelSubapts(float *image, coord_t res, int cells[2], int (*subc)[2], int (*apcoo)[2], int *totnsubap, float samini, int samxr);
 
/*!
 @brief Selects suitable subapts to work with (works with bytes)
 
 This is an updated version of the routine modSelSubapts(), works on byte data only.
 
 @param [in] *image The image that we need to look for subapts on
 @param [in] *shtrack The SH sensor configuration for this WFS
 @param [in] *shwfs The wavefront sensor configuration
 @param [out] *totnsubap The number of usable subapertures in the system
 */
int modSelSubaptsByte(uint8_t *image, mod_sh_track_t *shtrack, wfs_t *shwfs);

/*!
 @brief Parses output from Shack-Hartmann WFSs.
 
 This function takes the output from the drvReadSensor() routine (if the sensor is a
 SH WFS) and preprocesses this sensor output (typically from a CCD) to be further
 analysed by modCalcDMVolt(), which calculates the actual driving voltages for the
 DM. 
 
 @return EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
 @param [in] *image The image to parse through either CoG or correlation tracking
 @param [in] *subc The starting coordinates of the tracker windows
 @param [in] *gridc The starting coordinates of the grid cells
 @param [in] nsubap The number of subapertures (i.e. the length of the subc[] array)
 @param [in] track The size of the tracker windows
 @param [out] *disp The displacement vector wrt the reference displacements
 @param [in] *refc The reference displacements (i.e. after pinhole calibration)
 */
int modParseSH(gsl_matrix_float *image, int (*subc)[2], int (*gridc)[2], int nsubap, coord_t track, gsl_vector_float *disp, gsl_vector_float *refc);

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
 tracking window. Note that this will only work for star-like images, extended
 images will not work.
 
 @param [in] *image The image to parse through CoG tracking
 @param [in] *subc The starting coordinates of the tracker windows
 @param [in] nsubap The number of subapertures (i.e. the length of the subc[] array)
 @param [in] track The size of the tracker windows
 @param [out] *aver The average pixel intensity (good for CCD saturation)
 @param [out] *max The maximum pixel intensity
 @param [out] coords A list of coordinates the spots were found at wrt the center of the tracker windows
 */
void modCogTrack(gsl_matrix_float *image, int (*subc)[2], int nsubap, coord_t track, float *aver, float *max, float coords[][2]);

/*!
 @brief Calculates the controls to be sent to the various WFC, given displacements.
 
 TODO: doc
 */
int modCalcCtrl(control_t *ptc, mod_sh_track_t *shtrack, const int wfs, int nmodes);

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
