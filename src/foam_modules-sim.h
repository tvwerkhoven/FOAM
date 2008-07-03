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
#include "foam_modules-dm.h"		// we want the DM subroutines here too
#include "foam_modules-calib.h"		// we want the calibration
#include "foam_modules-img.h"		// we want image IO


// These are defined in foam_cs_library.c
extern control_t ptc;
//extern config_t cs_config;

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

	fftw_complex *shin;		//!< (foam) input for fft algorithm
	fftw_complex *shout;	//!< (foam) output for fft (but shin can be used if fft is inplace)
	fftw_plan plan_forward; //!< (foam) plan, one time calculation on how to calculate ffts fastest
	char *wisdomfile;		//!< (user) filename to store the FFTW wisdom
} mod_sim_t;

// PROTOTYPES //
/**************/


int simInit(mod_sim_t *simparams);

/*!
 @brief Simulates wind by chaning the origin that simAtm uses
  // !!!:tim:20080703 add docs
 */
int simWind(mod_sim_t *simparams);

/*!
 @brief simAtm() crops a part of the source wavefront
  // !!!:tim:20080703 add docs
 */
int simAtm(mod_sim_t *simparams);

/*!
@brief Simulates sensor(s) read-out

During simulation, this function takes care of simulating the atmosphere, 
the telescope, any WFCs the wavefront passes along and the (SH) sensor itself, since
these all occur in sequence.
 
 // !!!:tim:20080703 add docs
@return EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
*/
int simSensor(mod_sim_t *simparams, mod_sh_track_t *shwfs);

/*!
 @brief Simulates the SH WFS sensor
 
 // !!!:tim:20080703 add docs
 */
int simSHWFS(mod_sim_t *simparams, mod_sh_track_t *shwfs);

/*!
 @brief \a simTel() simulates the effect of the telescope (aperture) on the output of \a simAtm().
 
 This fuction works in wavefront-space, and basically multiplies the aperture function with
 the wavefront from \a simAtm().
 */
int simTel(mod_sim_t *simparams);

// !!!:tim:20080703 codedump below here, not used
#if (0)


/*!
@brief This simulates errors using a certain WFC, useful for performance testing

TODO: document
*/
void modSimError(int wfc, int method, int verbosity);


/*!
@brief \a simWFC() simulates a certain wave front corrector, like a TT or a DM.

This fuction works in wavefront-space.
*/
int simWFC(control_t *ptc, int wfcid, int nact, gsl_vector_float *ctrl, float *image);

#endif /* #if (0) */
#endif /* FOAM_MODULES_SIM */
