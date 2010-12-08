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
	free(frame_out);
}

bool SimWfs::setup(SimSeeing *ref) {	
	return true;
}

uint8_t *SimWfs::sim_shwfs(gsl_matrix *wave_in) {
	io.msg(IO_DEB2, "SimWfs::sim_shwfs()");
	//! @todo Given a wavefront, image it through a system and return the resulting intensity pattern (i.e. an image).
	// Process frame here...
	double min=0, max=0, fac;
	gsl_matrix_minmax(wave_in, &min, &max);
	fac = 255.0/(max-min);
	
	// Convert frame to uint8_t
	size_t cursize = wave_in->size1 * wave_in->size2  * (sizeof(uint8_t));
	if (out_size != cursize) {
		io.msg(IO_DEB2, "SimWfs::sim_shwfs() realloc, %zu != %zu", out_size, cursize);
		out_size = cursize;
		frame_out = (uint8_t *) realloc(frame_out, out_size);
	}
	
	for (size_t i=0; i<wave_in->size1; i++)
		for (size_t j=0; j<wave_in->size2; j++)
			frame_out[i*wave_in->size2 + j] = (uint8_t) ((wave_in->data[i*wave_in->tda + j] - min)*fac);
	
	return frame_out;
}

Shwfs::sh_simg_t *SimWfs::gen_mla_grid(coord_t res, coord_t size, coord_t pitch, int xoff, coord_t disp, int &nsubap) {

	// How many subapertures would fit in the requested size 'res':
	int sa_range_y = (int) ((res.y/2)/pitch.x + 1);
	int sa_range_x = (int) ((res.x/2)/pitch.y + 1);
	
	nsubap = 0;
	Shwfs::sh_simg_t *pattern = NULL;
	
	coord_t sa_c;
	// Loop over potential subaperture positions 
	for (int sa_y=-sa_range_y; sa_y < sa_range_y; sa_y++) {
		for (int sa_x=-sa_range_x; sa_x < sa_range_x; sa_x++) {
			// Centroid position of current subap is 
			sa_c.x = sa_x * pitch.x;
			sa_c.y = sa_y * pitch.y;
			
			// Offset odd rows: 'sa_y % 2' gives 0 for even rows and 1 for odd rows. 
			// Use this to apply a row-offset to even and odd rows
			sa_c.x -= (sa_y % 2) * xoff * pitch.x;
			
			if ((fabs(sa_c.x) < res.x/2) && (fabs(sa_c.y) < res.y/2)) {
				io.msg(IO_DEB2, "SimWfs::gen_mla_grid(): Found subap within bounds.");
				nsubap++;
				pattern = (Shwfs::sh_simg_t *) realloc((void *) pattern, sizeof (Shwfs::sh_simg_t));
				pattern[nsubap-1].pos.x = sa_c.x;
				pattern[nsubap-1].pos.y = sa_c.y;
				pattern[nsubap-1].llpos.x = sa_c.x - size.x/2;
				pattern[nsubap-1].llpos.y = sa_c.y - size.y/2;
				pattern[nsubap-1].size.x = size.x;
				pattern[nsubap-1].size.y = size.y;
			}
		}
	}
	
	return pattern;
}