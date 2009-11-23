/*
 Copyright (C) 2008-2009 Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)
 
 This file is part of FOAM.
 
 FOAM is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 FOAM is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with FOAM.  If not, see <http://www.gnu.org/licenses/>.

 $Id$
 */
/*! 
	@file shtrack.ccc
	@author Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)
	@date 2008-07-15

	@brief This file contains modules and functions related to Shack-Hartmann 
	wavefront sensing.
	
*/

// HEADERS //
/***********/

#include "shtrack.h"
#include "io.h"

// ROUTINES //
/************/
extern Io *io;

Shtrack::Shtrack(coord_t grid) {
	Shtrack(grid.x, grid.y);
}

Shtrack::Shtrack(int x, int y) {
	sh = new shwfs_t;
	sh->cells.x = x;
	sh->cells.y = y;
	sh->ns = 0;
	sh->subc = (coord_t*) calloc(1, x * y * sizeof *sh->subc);
	sh->cellc = (coord_t*) calloc(1, x * y * sizeof *sh->cellc);
	phfile = inffile = "";
}

Shtrack::~Shtrack(void) {
	if (sh->subc) free(sh->subc);
	if (sh->cellc) free(sh->cellc);
	gsl_vector_float_free(sh->refc);
	gsl_vector_float_free(sh->disp);
}

static int _cog(void *img, dtype_t dtype, coord_t res, int stride, int samini, float *cog) {
	float fi, csum = 0;
	cog[0] = cog[1] = 0.0;
	switch (dtype) {
		case DATA_UINT8: {
			for (int iy=0; iy < res.x; iy++) { // loop over all pixels
				for (int ix=0; ix < res.y; ix++) {
					fi = ((uint8_t *)img)[ix + iy*stride];
					if (fi > samini) {
						csum += fi;
						cog[0] += fi * ix;	// center of gravity of subaperture intensity 
						cog[1] += fi * iy;
					}
				}
			}
		}
		case DATA_UINT16: {
			for (int iy=0; iy < res.x; iy++) {
				for (int ix=0; ix < res.y; ix++) {
					fi = ((uint16_t *)img)[ix + iy*stride];
					if (fi>samini) {
						csum += fi;
						cog[0] += fi * ix;
						cog[1] += fi * iy;
					}
				}
			}
		}
	}
	cog[0] /= csum;
	cog[1] /= csum;
	return csum;
}

int Shtrack::selSubaps(wfs_t *wfs) {
	int csum;
	float cog[2], savec[2] = {0};
	int apmap[sh->cells.x][sh->cells.y];
	
	io->msg(IO_INFO, __FILE__ ": __FILE__: Selecting subapertures now...");

	for (int isy=0; isy < sh->cells.y; isy++) { // loops over all grid cells
		for (int isx=0; isx < sh->cells.x; isx++) {
			csum = _cog(wfs->image, wfs->dtype, wfs->res, wfs->res.x, sh->samini, cog);
			if (csum > 0) {
				apmap[isx][isy] = 1;
				sh->cellc[sh->ns].x = isx * sh->shsize.x; // Subap position
				sh->cellc[sh->ns].y = isy * sh->shsize.y;
				savec[0] += sh->subc[sh->ns].x; // Sum all subap positions
				savec[1] += sh->subc[sh->ns].y;
				sh->ns++;
			}
			else
				apmap[isx][isy] = 1;
		}
	}
	
	// *Average* subaperture position (wrt the image origin)
	savec[0] /= sh->ns;
	savec[1] /= sh->ns;
	
	io->msg(IO_INFO, __FILE__ "Found %d subaps with I > %d.", sh->ns, sh->samini);
	
	// Find central aperture by minimizing (subap position - average position)
	int csa = 0;
	float dist, rmin;
	rmin = sqrtf( ((sh->cellc[csa].x - savec[0]) * (sh->cellc[csa].x - savec[0])) 
		+ ((sh->cellc[csa].y - savec[1]) * (sh->cellc[csa].y - savec[1])));
	for (int i=1; i < sh->ns; i++) {
		dist = sqrt(
			((sh->cellc[i].x - savec[0]) * (sh->cellc[i].x - savec[0]))
			+ ((sh->cellc[i].y - savec[1]) * (sh->cellc[i].y - savec[1])));
		if (dist < rmin) {
			rmin = dist;
			csa = i; 									// new best guess for central subaperture
		}
	}

	io->msg(IO_INFO, __FILE__ "Central subaperture #%d at (%d,%d)", csa,
		sh->cellc[csa].x, sh->cellc[csa].y);
	
	return sh->ns;
}

int Shtrack::cogFind(wfs_t *wfs) {
  return 0;
}
