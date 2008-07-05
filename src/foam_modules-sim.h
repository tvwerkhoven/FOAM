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

#define	SOR_LIM	  (1.e-8)			// limit value for SOR iteration in simDM()

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

	fftw_complex *shin;		//!< (foam) input for fft algorithm
	fftw_complex *shout;	//!< (foam) output for fft (but shin can be used if fft is inplace)
	fftw_plan plan_forward; //!< (foam) plan, one time calculation on how to calculate ffts fastest
	char *wisdomfile;		//!< (user) filename to store the FFTW wisdom
} mod_sim_t;

// PROTOTYPES //
/**************/

/*!
 @brief Initialize the simulation module with a pre-filled mod_sim_t struct
 */
int simInit(mod_sim_t *simparams);

/*!
 @brief Simulates wind by chaning the origin that simAtm uses
 */
int simWind(mod_sim_t *simparams);

/*!
 @brief Crop a part of the source wavefront and copy it to the sensor image
 */
int simAtm(mod_sim_t *simparams);

/*!
 @brief Generate a flat wavefront with a certain intensity
 */
int simFlat(mod_sim_t *simparams, int intensity);

/*!
 @brief Add noise to an image (i.e after generating a flat wavefront)
 */
int simNoise(mod_sim_t *simparams, int var);

/*!
 @brief Simulate a tip-tilt mirror, i.e. generate a sloped wavefront
 
 ctrl should range from -1 to 1, be linear, and contain 2 elements.
 This routine updates the image stored in simparams->currimg, which must be
 allocated.
 
 The simulationitself is done as follows: a slope is generated over the whole
 image, which ranges from -amp to amp. After that, this slope is multiplied by
 the values stored in *ctrl (ctrl[0] for x, ctrl[1] for y). Once this slope has
 been calculated, it is either appended to the image (mode==1) or it is
 overwritten (mode==0).
 
 @param [in] *ctrl A 2-element array with the controls for the tip-tilt mirror
 @param [in] *simparams The simulation parameters
 @param [in] mode The mode of simulation to be used. 1=append tip-tilt image, 0=overwrite
 @return EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
 */

int simTT(mod_sim_t *simparams, gsl_vector_float *ctrl, int mode);

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

/*!
 @brief Simulates a SH WFS sensor, subdivide the wavefront in subapertures and image these
 */
int simSHWFS(mod_sim_t *simparams, mod_sh_track_t *shwfs);

/*!
 @brief simTel() simulates the effect of a telescope aperture
 
 This fuction works in wavefront-space, and multiplies the aperture function with
 the wavefront from simAtm().
 */
int simTel(mod_sim_t *simparams);

/*!
 @brief Simulate a WFC, a wrapper for simTT() and simDM(), can be extended in the future
 */
int simWFC(wfc_t *wfc, mod_sim_t *simparams);

#endif /* FOAM_MODULES_SIM */
