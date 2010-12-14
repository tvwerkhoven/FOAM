/*
 simwfs.cc -- simulate a wavefront sensor
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
#include <stdlib.h>
#include <math.h>
#include <gsl/gsl_matrix.h>

#include "io.h"
#include "foamctrl.h"
#include "simwfs.h"

using namespace std;

SimWfs::SimWfs(Io &io, foamctrl *ptc, string name, string port, Path &conffile):
Device(io, ptc, name, simwfs_type, port),
frame_out(NULL), out_size(0)
{
	io.msg(IO_DEB2, "SimWfs::SimWfs()");
}

SimWfs::~SimWfs() {
	io.msg(IO_DEB2, "SimWfs::~SimWfs()");
	if (frame_out)
		free(frame_out);
}

bool SimWfs::setup(SimSeeing *ref, coord_t &res, coord_t &sasize, coord_t &sapitch, int xoff, coord_t &disp) {	
	// Read configuration file here
	//! @todo do something with SimSeeing ref, load shape from config file
	int nsubap=0;
	mla = Shwfs::gen_mla_grid(res, sasize, sapitch, xoff, disp, Shwfs::CIRCULAR, nsubap);
	
	return true;
}

uint8_t *SimWfs::sim_shwfs(gsl_matrix *wave_in) {
	io.msg(IO_DEB2, "SimWfs::sim_shwfs()");
	//! @todo Given a wavefront, image it through a system and return the resulting intensity pattern (i.e. an image).
	double min=0, max=0, fac;
	gsl_matrix_minmax(wave_in, &min, &max);
	fac = 255.0/(max-min);
	
	// Apply fourier transform to subimages here
	
	(coord_t res, coord_t size, coord_t pitch, int xoff, coord_t disp, int &nsubap);
	// Convert frame to uint8_t, scale properly
	size_t cursize = wave_in->size1 * wave_in->size2  * (sizeof(uint8_t));
	if (out_size != cursize) {
		io.msg(IO_DEB2, "SimWfs::sim_shwfs() reallocing memory, %zu != %zu", out_size, cursize);
		out_size = cursize;
		frame_out = (uint8_t *) realloc(frame_out, out_size);
	}
	
	for (size_t i=0; i<wave_in->size1; i++)
		for (size_t j=0; j<wave_in->size2; j++)
			frame_out[i*wave_in->size2 + j] = (uint8_t) ((wave_in->data[i*wave_in->tda + j] - min)*fac);
	
	return frame_out;
}
