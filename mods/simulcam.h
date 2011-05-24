/*
 simulcam.h -- atmosphere/telescope simulator camera header
 Copyright (C) 2010--2011 Tim van Werkhoven <t.i.m.vanwerkhoven@xs4all.nl>
 
 This file is part of FOAM.
 
 FOAM is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 2 of the License, or
 (at your option) any later version.
 
 FOAM is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with FOAM.	If not, see <http://www.gnu.org/licenses/>. 
 */


#ifndef HAVE_SIMULCAM_H
#define HAVE_SIMULCAM_H

#ifdef HAVE_CONFIG_H
#include "autoconfig.h"
#endif

#include <stdint.h>
#include <math.h>
#include <fftw3.h>
#include <gsl/gsl_vector.h>

#include "config.h"
#include "io.h"

#include "camera.h"
#include "simseeing.h"
#include "simulwfc.h"
#include "shwfs.h"

using namespace std;

const string simulcam_type = "simulcam";

/*!
 @brief Simulation class for seeing + camera
 
 SimulCam is derived from Camera. Given a static input wavefront, it simulates
 a Shack-Hartmann wavefront sensor (i.e. the CCD).
 
 Configuration parameters:
 - noise: fraction of CCD pixels covered with noise
 - noiseamp: nosie amplitude as fraction of maximum
 - seeingfac: factor to multiply wavefront image with
 (for SimSeeing:)
 - wavefront_file: static FITS file which shows some wavefront
 - windspeed.x,y: windspeed by which the wavefront moves
 - windtype: 'random' or 'linear', method of scanning over the wavefront
 - cropsize.x,y: size to crop wavefront field to (should be same as simulated camera)
 
 Network commands:
 - get/set noise: see above
 - get/set noiseamp: see above
 - get/set seeingfac: see above
 - get/set windspeed: see above
 - get/set telapt_fill: subaperture should have at least this fraction of light
 - set simwf/simtel/simmla: enable (1) or disable (0) the simulation of the 
   wavefront, telescope or microlens array respectively.
*/
class SimulCam: public Camera {
private:
	SimSeeing seeing;										//!< This class simulates the atmosphere
	SimulWfc simwfc;										//!< This class simulates a wavefront corrector
	
	size_t out_size;										//!< Size of frame_out
	uint8_t *frame_out;									//!< Frame to store simulated image

	double telradius;										//!< Telescope radius (fraction of CCD)
	gsl_matrix *telapt;									//!< Telescope aperture mask
	double telapt_fill;									//!< How much subaperture should be within the telescope aperture to be processed
	
	double noise;												//!< Noise fill factor for CCD
	double noiseamp;										//!< Noise amplitude for CCD
	double seeingfac;										//!< Multiplicative factor for wavefront screen.
	bool simtel;												//!< Simulate telescope aperture or not
	bool simmla;												//!< Simulate microlens array or not
	
public:
	Shwfs shwfs;												//!< Reference to WFS we simulate (i.e. for configuration)
	SimulCam(Io &io, foamctrl *const ptc, const string name, const string port, Path const &conffile, const bool online=true);
	~SimulCam();
	
	void gen_telapt();									//!< Generate telescope aperture with radius telradius. Inside this radius the mask has value 'seeingfac', outside it's 0.

	gsl_matrix *simul_seeing();					//!< Simulate seeing: get wavefront and apply seeing factor.
	void simul_telescope(gsl_matrix *wave_in) const; //!< Multiply input wavefront with telescope aperture mask from gen_telapt().
	void simul_wfs(gsl_matrix *wave_in) const; //!< Simulate wavefront sensor optics given an input wavefront.
	uint8_t *simul_capture(gsl_matrix *frame_in);	//!< Simulate CCD frame capture (exposure, offset, etc.)
	
	// From Camera::
	void cam_handler();
	void cam_set_exposure(const double value);
	double cam_get_exposure();
	void cam_set_interval(const double value);
	double cam_get_interval();
	void cam_set_gain(const double value);
	double cam_get_gain();
	void cam_set_offset(const double value);
	double cam_get_offset();
	
	void cam_set_mode(const mode_t newmode);
	void do_restart();
	
	// From Devices::
	void on_message(Connection *const conn, string);
};

#endif // HAVE_SIMULCAM_H

/*!
 \page dev_cam_simulcam Simulation camera devices
 
 The SimulCam class is an end-to-end simulation camera.
 
 */
