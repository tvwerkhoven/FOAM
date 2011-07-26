/*
 simseeing.h -- atmosphere/telescope simulator header
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

#ifndef HAVE_SIMSEEING_H
#define HAVE_SIMSEEING_H

#include <stdint.h>
#include <gsl/gsl_matrix.h>

#include "types.h"
#include "config.h"
#include "io.h"
#include "devices.h"

using namespace std;
const string simseeing_type = "simseeing";

/*!
 @brief This class simulates seeing by an atmosphere.
 
 This class is offline (no network connection), and is controlled from the
 SimulCam class.
 
 Configuration parameters:
 - None, works through SimulCam
 
 Network commands:
 - None, offline device
 */
class SimSeeing: public Device {
public:
	typedef enum {
		RANDOM=0,													//!< random walk around the wavefront
		LINEAR,														//!< linearly move over the wavefront 
		DRIFTING													//!< drift over the wavefront, combination of linear and random
	} wind_t;														//!< Possible simulated windtypes
	
private:
	gsl_matrix *wfsrc;                  //!< Holds the wavefront data from SimSeeing::wffile
	Path wffile;                        //!< Source file for wavefront simulation. Should be bigger than crop window size (SimSeeing::cropsize).
	
	gsl_matrix *load_wavefront(const Path &f, const bool norm=true); //!< Load wavefront data from disc. N.B. The returned matrix has to be freed on exit!
	
public:
	coord_t croppos;										//!< Lower-left position to crop out of wavefront
	coord_t cropsize;										//!< Size of the wavefront to return
	
	coord_t windspeed;									//!< Windspeed in pixels/frame
	wind_t windtype;										//!< Windtype used for seeing simulation
	
	double seeingfac;										//!< Multiplicative factor for wavefront screen. Makes seeing better or worse.
	
	SimSeeing(Io &io, foamctrl *const ptc, const string name, const string port, const Path &conffile);
	~SimSeeing();
	
	/* @brief Get a crop of the wavefront, automatically propagating the wind if requested.
	 
	 @param [out] wf_out Matrix to store the wavefront, pre-allocated
	 */
	int get_wavefront(gsl_matrix *wf_out);
	/* @brief Get a crop of the wavefront at specific coordinates
	 
	 @param [out] wf_out Matrix to store the wavefront, pre-allocated
	 @param [in] x0 x-coordinate of the crop-origin
	 @param [in] y0 y-coordinate of the crop-origin
	 @param [in] w width of the crop
	 @param [in] h height of the crop
	 @param [in] fac Multiplicative factor to apply to wavefront
	 */
	int get_wavefront(gsl_matrix *wf_out, const size_t x0, const size_t y0, const size_t w, const size_t h, const double fac=1.0) const;
};

#endif // HAVE_SIMSEEING_H
