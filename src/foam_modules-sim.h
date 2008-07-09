/*! 
	@file foam_modules-sim.h
	@author @authortim
	@date November 30 2007

	@brief This header file prototypes simulation functions.
*/
#ifndef FOAM_MODULES_SIM
#define FOAM_MODULES_SIM

#include <fftw3.h> 					// we need this for modSimSH()
#include "foam_cs_library.h"		// we link to the main program here (i.e. we use common (log) functions)
#include "foam_modules-calib.h"		// we want the calibration
#include "foam_modules-sh.h"		// we want image IO

/*!
 @brief This struct is used to characterize seeing conditions
 */
typedef struct {
	coord_t wind; 			//!< (user) 'windspeed' in pixels/frame
	float seeingfac;		//!< (user) factor to worsen seeing (2--20)
	coord_t currorig; 		//!< (foam) current origin in the simulated wavefront image

	uint8_t *currimg; 		//!< (user) pointer to a crop of the simulated wavefront
	coord_t currimgres;		//!< (user) resolution of the crop

	char *wf;				//!< (user) wavefront image to use (pgm)
	uint8_t *wfimg;			//!< (foam) wavefront image itself
	coord_t wfres;			//!< (foam) wavefront resolution
	
	char *apert;			//!< (user) telescope aperture image to use (pgm)
	uint8_t *apertimg;		//!< (foam) aperture image itself
	coord_t apertres;		//!< (foam) aperture image resolution
	
	char *actpat;			//!< (user) actuator pattern to use (pgm)
	uint8_t *actpatimg;		//!< (foam) actuator patter image
	coord_t actpatres;		//!< (foam) actuator patter image resolution
	
	uint8_t *dmresp;		//!< (foam) deformable mirror response

	fftw_complex *shin;		//!< (foam) input for FFT algorithm
	fftw_complex *shout;	//!< (foam) output for FFT (but shin can be used if FFT is in place)
	fftw_plan plan_forward; //!< (foam) plan, one time calculation on how to calculate FFTs fastest
	char *wisdomfile;		//!< (user) filename to store the FFTW wisdom
} mod_sim_t;

// PROTOTYPES //
/**************/

/*!
 @brief Initialize the simulation module with a pre-filled mod_sim_t struct
 
 This routine initializes the simulation module, reading in the several files
 necessary for simulation (i.e. simulated wavefront, DM actuator pattern etc),
 and allocating memory for the simulated image. It also does some sanity checks
 to see if the settings make sense.
 
 @param [in] *simparams A mod_sim_t struct with the (user) fields filled in
 @return EXIT_SUCCESS on success, EXIT_FAILURE otherwise
 */
int simInit(mod_sim_t *simparams);

/*!
 @brief Simulates wind by changing the origin that simAtm() uses
 
 The wavefront simulation works by taking a big simulated wavefront file 
 (simparams->wfimg), and cropping a portion of this (simparams->currimgres)
 and using this as wavefront input. If wfimg is larger than currimgres
 (which is strongly recommended), simWind() can simulated wind by scanning
 over the bigger wavefront image and taking different crops every time.
 
 For example, suppose wfimg is 1024x1024, the simulated image is
 256x256 pixels and simparams->wind is {10,0}, then simWind() will move the cropping
 origin from {0,0} to {10,0} to {20,0} up and till {760,0} after which it will move to
 {750,0} and back to {0,0} (note that 760 + 10 (wind) + 256 (sensor) > 1024 (source image)).
 
 simWind() only moves the origin of cropping, simAtm() does the actual cropping.
 
 @param [in] *simparams A mod_sim_t struct passed through simInit()
 */
int simWind(mod_sim_t *simparams);

/*!
 @brief Crop a part of the source wavefront and copy it to the sensor image
 
 As the cropping origin is moved by simWind(), simAtm() crops a part of the
 bigger wavefront image and copies this to the simulated image output.
 
 @param [in] *simparams A mod_sim_t struct passed through simInit()
 */
int simAtm(mod_sim_t *simparams);

/*!
 @brief Generate a flat wavefront with a certain intensity
 
 This routine can be used to generate a flat image and copy it to the 
 simulated image location, simparams->currimg. This can be used to fake
 dark- and flat-fields, or to generate a flat wavefront which simulates
 a pinhole somewhere, e.g. for calibration.

 @param [in] *simparams A mod_sim_t struct passed through simInit()
 @return EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
 */
int simFlat(mod_sim_t *simparams, int intensity);

/*!
 @brief Add noise to an image (i.e after generating a flat wavefront)
 
 This routine adds random noise (drand48()) with amplitude var to the 
 simulated image. Can be used to simulate sensornoise or something.
 
 @param [in] *simparams A mod_sim_t struct passed through simInit()
 @return EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
 */
int simNoise(mod_sim_t *simparams, int var);

/*!
 @brief Simulate a tip-tilt mirror, i.e. generate a sloped wavefront
 
 ctrl should range from -1 to 1, be linear, and contain 2 elements.
 This routine updates the image stored in simparams->currimg, which must be
 allocated.
 
 The simulation itself is done as follows: a slope is generated over the whole
 image, which ranges from -amp to amp. After that, this slope is multiplied by
 the values stored in *ctrl (ctrl[0] for x, ctrl[1] for y). Once this slope has
 been calculated, it is either appended to the image (mode==1) or it is
 overwritten (mode==0).
 
 @param [in] *ctrl A 2-element array with the controls for the tip-tilt mirror
 @param [in] *simparams A mod_sim_t struct passed through simInit()
 @param [in] mode The mode of simulation to be used. 1=append tip-tilt image, 0=overwrite
 @return EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
 */

int simTT(mod_sim_t *simparams, gsl_vector_float *ctrl, int mode);

/*!
 @brief Simulates a SH WFS sensor, subdivide the wavefront in subapertures and image these
 
 This routine simulates a Shack-Hartmann wavefront sensor, and therefore needs the
 Shack-Hartmann FOAM module.
 
 Taking a simulated wavefront from simparams->currimg, this routines
 processes the selected subapertures one by one. Taking the subapertures
 locations stored in shwfs->subc, it crops out a part the size of
 shwfs->track and puts this in the center of a temporary array twice the size of 
 shwfs->track, setting the border pixels to zero.
 
 Once the subaperture is copied to this temporary location, the phase
 is used to generate a complex wave using exp ( i * phi ) = {cos(phi) , i sin(phi)},
 and this is then imaged using a forward fourier transform through FFTW.
 Note that the border of the temporary image with value 0 will get a complex
 value of {1,0}. 
 
 After fourier transforming this complex wave, the absolute value of the 
 complex components squared and added is calculated as
 amp = abs(R^2 + I^2) with R and I the real and complex components, respectively.
 The relevant portion of this image is copied back to the subaperture in the big 
 sensorimage.
 
 The parameter simparams->seeingfac can be used to modify the seeing
 strength as it is used to multiply the incoming wavefront with. If this
 factor is 0, irregardless of the wavefront the image output will be circular
 dots.
 
 The border around the subaperture image is used to generate artificial 
 blurring caused by the telescope.
 
 @param [in] *simparams A mod_sim_t struct passed through simInit()
 @param [in] *shwfs The SH WFS configuration from the SH module
 */
int simSHWFS(mod_sim_t *simparams, mod_sh_track_t *shwfs);

/*!
 @brief simTel() simulates the effect of a telescope aperture
 
 This fuction works in wavefront-space, and multiplies the aperture function with
 the wavefront from simAtm(). The aperture is taken from simparams->apertimg.

 @param [in] *simparams A mod_sim_t struct passed through simInit()
 */
int simTel(mod_sim_t *simparams);

/*!
 @brief Simulate a WFC, a wrapper for simTT() and simDM()
 
 This routine can be used to simulate one of the several actuators in the system.
 
 @param [in] *simparams A mod_sim_t struct passed through simInit()
 @param [in] wfc The WFC to simulate
 */
int simWFC(wfc_t *wfc, mod_sim_t *simparams);

/*!
 @brief Simulate a WFC error with one of the WFCs in the system
 
 This routine simulates a WFC error such that when simulating closed-loop
 adaptive optics, this error can be completely corrected. Useful for
 checking whether the system works or not, as it is the easiest
 error to correct (i.e. it can be corrected perfectly)
 
 There are two modi this routine has, one (method == 1) simulates a regular
 sawtooth drift in all control actuators simultaneously. This means
 that the eroror ranges from -1 to 1 over a 'period' frames. The other mode
 (method == 2) simulates a random drift of the errors, with different
 values for each actuator.
 
 The first method is very regular and easy to verify by eye that it is
 corrected. The second is a more natural error and slightly more challenging
 to correct.
 
 @param [in] *simparams A mod_sim_t struct passed through simInit()
 @param [in] wfc The WFC to simulate
 @param [in] method The method to use, sawtooth or random
 @param [in] period The period for the sawtooth error, if used
 */
int simWFCError(mod_sim_t *simparams, wfc_t *wfc, int method, int period);

#if (0) // simDM is broken, ignore for the time being
/*!
 @brief Simulate a deformable mirror and generate the associated wavefront (BROKEN)
 
 This routine, based on response2.c by C.U. Keller, takes a boundarymask,
 an actuatorpattern for the DM being simulated, the number of actuators
 and the voltages as input. It then calculates the shape of the mirror
 and adds or overwrites this to simparams->currimg, which must be allocated before
 calling this function. Additionally, niter can be set to limit the amount of 
 iterations (set to 0 if unsure).
 
 @param [in] *simparams The number of actuators, must be the same as used in \a *actuatorpat
 @param [in] nact The number of actuators, must be the same as used in \a *actuatorpat
 @param [in] *ctrl The control commands array, \a nact long
 @param [in] mode Add to the wavefront (mode==1) or overwrite? (mode==0)
 @param [in] niter The number of iterations, 0 for automatic choice
 @return EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
 */
int simDM(mod_sim_t *simparams, int nact, gsl_vector_float *ctrl, int mode, int niter);
#endif

#endif /* FOAM_MODULES_SIM */
