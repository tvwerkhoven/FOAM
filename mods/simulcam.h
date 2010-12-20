/*
 simulcam.h -- atmosphere/telescope simulator camera header
 Copyright (C) 2010 Tim van Werkhoven <t.i.m.vanwerkhoven@xs4all.nl>
 
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

#include <stdint.h>

#include "config.h"
#include "io.h"

#include "camera.h"
#include "simseeing.h"
#include "shwfs.h"

using namespace std;

const string simulcam_type = "simulcam";

/*!
 @brief Simulation class for seeing + camera
 
 SimulCam is derived from Camera. Given a static input wavefront, it simulates
 a Shack-Hartmann wavefront sensor (i.e. the CCD).
 
 Configuration parameters:
 - wavefront_file: static FITS file which shows some wavefront
 - windspeed.x,y: windspeed by which the wavefront moves
 - windtype: 'random' or 'linear', method of scanning over the wavefront
 - noise: random additive noise for frames
 - seeingfac: factor to multiply wavefront image with
 */
class SimulCam: public Camera {
private:
	SimSeeing seeing;										//!< This class simulates the atmosphere
	Shwfs *shwfs;												//!< Reference to WFS we simulate (i.e. for configuration)
	
	size_t out_size;										//!< Size of frame_out
	uint8_t *frame_out;									//!< Frame to store simulated image

	double telradius;										//!< Telescope radius (fraction of CCD)
	gsl_matrix *telapt;									//!< Telescope aperture mask
	
	double noise;												//!< Noise factor for CCD
	double seeingfac;										//!< Multiplicative factor for wavefront screen.
	
public:
	SimulCam(Io &io, foamctrl *ptc, string name, string port, Path &conffile, bool online=true);
	~SimulCam();
	
	void gen_telapt();									//!< Generate telescope aperture with radius telradius. Inside this radius the mask has value 'seeingfac', outside it's 0.

	void simul_telescope(gsl_matrix *wave_in); //!< Multiply input wavefront with telescope aperture mask from gen_telapt().
	void simul_wfs(gsl_matrix *wave_in); //!< Simulate wavefront sensor optics given an input wavefront.
	uint8_t *simul_capture(gsl_matrix *frame_in);	//!< Simulate CCD frame capture (exposure, offset, etc.)
	
	void set_shwfs(Shwfs *ref) { shwfs = ref; } //!< Assign Shwfs object to SimulCam. The parameters of this Shwfs will be used for simulation.
		
	// From Camera::
	void cam_handler();
	void cam_set_exposure(double value);
	double cam_get_exposure();
	void cam_set_interval(double value);
	double cam_get_interval();
	void cam_set_gain(double value);
	double cam_get_gain();
	void cam_set_offset(double value);
	double cam_get_offset();
	
	void cam_set_mode(mode_t newmode);
	void do_restart();
	
	void on_message(Connection*, std::string);
};

#endif // HAVE_SIMULCAM_H
