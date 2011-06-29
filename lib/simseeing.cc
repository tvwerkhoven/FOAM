/*
 simseeing.cc -- atmosphere/telescope simulator
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

#include <string>
#include <gsl/gsl_matrix.h>

#include "io.h"
#include "types.h"
#include "imgdata.h"
#include "foamctrl.h"

#include "simseeing.h"

/*
 *  Constructors / destructors
 */

SimSeeing::SimSeeing(Io &io, foamctrl *const ptc, const string name, const string port, const Path &conffile):
Device(io, ptc, name, simseeing_type, port, conffile, false),
file(""), croppos(0,0), cropsize(0,0), windspeed(10,10), windtype(LINEAR)
{
	io.msg(IO_DEB2, "SimSeeing::SimSeeing()");
	
	// Setup seeing parameters
	file = ptc->confdir + cfg.getstring("wavefront_file");
	
	if (cfg.exists("windspeed"))
		windspeed.x = windspeed.y = cfg.getint("windspeed");
	else {
		windspeed.x = cfg.getint("windspeed.x", 16);
		windspeed.y = cfg.getint("windspeed.y", 16);
	}

	string windstr = cfg.getstring("windtype", "random");
	if (windstr == "linear")
		windtype = SimSeeing::LINEAR;
	else if (windstr == "random")
		windtype = SimSeeing::RANDOM;
	else
		windtype = SimSeeing::DRIFTING;

	if (cfg.exists("cropsize"))
		cropsize.x = cropsize.y = cfg.getint("cropsize");
	else {
		cropsize.x = cfg.getint("cropsize.x");
		cropsize.y = cfg.getint("cropsize.y");
	}
	
	seeingfac = cfg.getdouble("seeingfac", 1.0);


	// Load & scale wavefront
	wfsrc = load_wavefront(file);
		
	// Check if cropsize makes sense.
	// N.B. This can lead to problems if the SimulCam resolution is higher than
	// the wavefront source file, the crop size will be smaller than the camera 
	// and the program will break.
	if (wfsrc->size1 < (size_t) cropsize.x || wfsrc->size2 < (size_t) cropsize.y) {
		io.msg(IO_WARN, "SimSeeing::setup() wavefront smaller than requested cropsize (%dx%d vs %dx%d), reducing size to half the wavefront size.", 
					 wfsrc->size1, wfsrc->size2, cropsize.x, cropsize.y);
		cropsize.x = 0.5*wfsrc->size1;
		cropsize.y = 0.5*wfsrc->size2;
	}
	// Check if windspeed makes sense
	if (windspeed.x >= cropsize.x || windspeed.y >= cropsize.y) {
		io.msg(IO_WARN, "SimSeeing::setup() windspeed (%d, %d) bigger than cropsize (%d, %d), reducing to half the cropsize.", 
					 windspeed.x, windspeed.y, cropsize.x, cropsize.y);
		windspeed.x = 0.5 * cropsize.x;
		windspeed.y = 0.5 * cropsize.y;
	}
}

SimSeeing::~SimSeeing() {
	io.msg(IO_DEB2, "SimSeeing::~SimSeeing()");
	gsl_matrix_free(wfsrc);
}

/*
 *  Private methods
 */

gsl_matrix *SimSeeing::load_wavefront(const Path &f, const bool norm) {
	file = f;
	io.msg(IO_DEB2, "SimSeeing::load_wavefront(), file=%s", file.c_str());
	
	if (!file.r())
		throw exception("SimSeeing::load_wavefront() cannot read wavefront file: " + file.str() + "!");
	
	ImgData wftmp(io, file, ImgData::AUTO);
	if (wftmp.err) {
		io.msg(IO_ERR, "SimSeeing::load_wavefront() ImgData returned an error: %d", wftmp.err);
		return NULL;
	}
	
	io.msg(IO_XNFO, "SimSeeing::load_wavefront() got wavefront: %zux%zux%d", wftmp.getwidth(), wftmp.getheight(), wftmp.getbpp());
	
	// We own the data now, we need to free it as well
	gsl_matrix *wf = wftmp.as_GSL(true);
	if (!wf)
		throw std::runtime_error("SimSeeing::load_wavefront() Could not load wavefront.");

	if (norm) {
		double min,max;
		gsl_matrix_minmax(wf, &min, &max);
		gsl_matrix_add_constant(wf, -min);
		gsl_matrix_scale(wf, 1.0/(max-min));
	}
	
	return wf;
}

/*
 *  Public methods
 */

int SimSeeing::get_wavefront(gsl_matrix *wf_out) {
	// Update new crop position (i.e. simulate wind)
	switch (windtype) {
		case RANDOM:
			croppos.x += (simple_rand()-0.5) * windspeed.x;
			croppos.y += (simple_rand()-0.5) * windspeed.y;
			// Check bounds
			croppos.x = clamp(croppos.x, (int) 0, (int) wfsrc->size2 - cropsize.x);
			croppos.y = clamp(croppos.y, (int) 0, (int) wfsrc->size1 - cropsize.y);
			break;
		case DRIFTING:
			// Random drift, use a slowly changing crop vector
			//! @todo randomness is hardcoded here, need to fix as a configuration
			windspeed.x += (simple_rand()-0.5) * 10.0;
			windspeed.y += (simple_rand()-0.5) * 10.0;
			
		case LINEAR:
		default:
			// Check bounds, revert wind if necessary
			if (croppos.x + windspeed.x >= (int) wfsrc->size2 - cropsize.x ||
					croppos.x + windspeed.x <= 0)
				windspeed.x *= -1;
			if (croppos.y + windspeed.y >= (int) wfsrc->size1 - cropsize.y ||
					croppos.y + windspeed.y <= 0)
				windspeed.y *= -1;
			
			// Apply wind
			croppos.x += windspeed.x;
			croppos.y += windspeed.y;			
			break;
	}

	return get_wavefront(wf_out, croppos.x, croppos.y, cropsize.x, cropsize.y, seeingfac);
}

int SimSeeing::get_wavefront(gsl_matrix *wf_out, const size_t x0, const size_t y0, const size_t w, const size_t h, const double fac) const {
//	io.msg(IO_DEB2, "SimSeeing::get_wavefront(%p, %zu, %zu, %zu, %zu, %g)", wf_out, x0, y0, w, h, fac);
	// Get crop from wavefront as submatrix 
	gsl_matrix_view tmp = gsl_matrix_submatrix(wfsrc, y0, x0, h, w);
	// Copy this to wf_out
//	io.msg(IO_DEB2, "SimSeeing::get_wavefront() cpy from %p: %zu*%zu to %p: %zu*%zu", 
//				 &(tmp.matrix), tmp.matrix.size1, tmp.matrix.size2,
//				 wf_out, wf_out->size1, wf_out->size2);
	gsl_matrix_memcpy(wf_out, &(tmp.matrix));
	
	// Apply scaling if requested (unequal to one)
	if (fac != 1.0)
		gsl_matrix_scale(wf_out, fac);

	return 0;
}

