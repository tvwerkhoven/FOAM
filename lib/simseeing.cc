/*
 simseeing.cc -- atmosphere/telescope simulator
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

#include <string>
#include <gsl/gsl_matrix.h>

#include "imgdata.h"
#include "io.h"
#include "types.h"

#include "foamctrl.h"
#include "simseeing.h"

/*
 *  Constructors / destructors
 */

SimSeeing::SimSeeing(Io &io, foamctrl *ptc, string name, string port, Path &conffile):
Device(io, ptc, name, SimSeeing_type, port),
file(""), croppos(0,0), cropsize(0,0), windspeed(10,10)
{
	io.msg(IO_DEB2, "SimSeeing::SimSeeing()");	
}

SimSeeing::~SimSeeing() {
	io.msg(IO_DEB2, "SimSeeing::~SimSeeing()");
	gsl_matrix_free(wfsrc);
}

/*
 *  Private methods
 */

gsl_matrix *SimSeeing::load_wavefront() {
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
	return wftmp.as_GSL(true);
}

/*
 *  Public methods
 */

bool SimSeeing::setup(Path &f, coord_t size, coord_t wspeed) {
	io.msg(IO_INFO, "SimSeeing::setup() Loading wavefront.");
	
	cropsize = size;
	windspeed = wspeed;
	file = f;
	
	// Data is alloced in load_wavefront(), ImgData.as_GSL() only once, does not need to be freed
	wfsrc = load_wavefront();
	
	if (!wfsrc)
		return false;
	
	if (wfsrc->size1 < cropsize.x || wfsrc->size2 < cropsize.y) {
		io.msg(IO_WARN, "SimSeeing::setup() wavefront smaller than requested cropsize (%dx%d vs %dx%d), reducing size to wavefront size.", 
					 wfsrc->size1, wfsrc->size2, cropsize.x, cropsize.y);
		cropsize.x = wfsrc->size1;
		cropsize.y = wfsrc->size2;
	}
		
	return true;
}

gsl_matrix_view SimSeeing::get_wavefront() {
	// Update new crop position
	croppos.x += (drand48()-0.5) * windspeed.x;
	croppos.y += (drand48()-0.5) * windspeed.y;
	
	// Check bounds
	croppos.x = clamp(croppos.x, (int) 0, (int) wfsrc->size1 - cropsize.x);
	croppos.y = clamp(croppos.y, (int) 0, (int) wfsrc->size2 - cropsize.y);
			
	return get_wavefront(croppos.x, croppos.y, cropsize.x, cropsize.y);
}

gsl_matrix_view SimSeeing::get_wavefront(size_t x0, size_t y0, size_t w, size_t h) {
	io.msg(IO_DEB2, "SimSeeing::get_wavefront(%zu, %zu, %zu, %zu)", x0, y0, w, h);
	gsl_matrix_view tmp = gsl_matrix_submatrix(wfsrc, x0, y0, w, h);
	return tmp;
}

