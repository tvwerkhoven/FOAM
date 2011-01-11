/*
 zernike.cc -- Generate zernike modes
 Copyright (C) 2011 Tim van Werkhoven <t.i.m.vanwerkhoven@xs4all.nl>
 
 This file is part of FOAM.
 
 FOAM is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 2 of the License, or
 (at your option) any later version.
 
 FOAM is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with FOAM.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <string>
#include <gsl/gsl_vector.h>
#include <gsl/gsl_matrix.h>
#include <strings.h>
#include <math.h>

#include "io.h"
#include "types.h"

#include "zernike.h"

Zernike::Zernike(Io &io, int n, int size):
io(io) {
	io.msg(IO_DEB2, "Zernike::Zernike(n=%d, size=%d)", n, size);
	
	setup(n, size);
	
}

Zernike::~Zernike() {
	io.msg(IO_DEB2, "Zernike::~Zernike()");
}


int Zernike::setup(int n, int size) {
	io.msg(IO_DEB2, "Zernike::setup(n=%d, size=%d)", n, size);
	
	if (n <= 0 || size <= 0)
		return 0;
	
	// Allocate pointer-to-gsl_matrix array. Realloc such that if basisfuncs is already alloc'ed, it only allocates the extra memory
	basis.bfuncs = (gsl_matrix **) realloc(basis.bfuncs, n * sizeof *(basis.bfuncs));
	//! @todo bzero() proper function for clearing memory?
	bzero((void *) basis.bfuncs, n * sizeof *(basis.bfuncs));
	
	// Allocate coordinate grids (free first in case it was previously allocated)
	gsl_matrix_free(basis.rho);
	basis.rho = (gsl_matrix *) gsl_matrix_alloc(size, size);
	gsl_matrix_free(basis.phi);
	basis.phi = (gsl_matrix *) gsl_matrix_alloc(size, size);
	
	// Calculate coordinate grids
	calc_rho(basis.rho);
	calc_phi(basis.phi);
	
	// Allocate individual matrices
	for (int i=0; i<n; i++) {
		gsl_matrix_free((basis.bfuncs)[i]);
		(basis.bfuncs)[i] = (gsl_matrix *) gsl_matrix_alloc(size, size);
		if (!(basis.bfuncs)[i])
			return io.msg(IO_ERR, "Zernike::setup(): error allocating memory");
	}
	
	basis.is_calc = false;
	basis.nmodes = n;
	basis.size = size;
	
	io.msg(IO_ERR, "Zernike::setup(): allocate ok");
	
	return 0;
}

void Zernike::calc_rho(gsl_matrix *mat) {
	// *mat should be square. Center is at:
	int c = mat->size1/2;
	io.msg(IO_DEB2, "Zernike::calc_rho(c=(%d,%d))", c, c);
	
	// every pixel will be ((i-c)**2 + (j-c)**2 )**0.5
	
	for (size_t i=0; i<mat->size1; i++)
		for (size_t j=0; j<mat->size2; j++)
			gsl_matrix_set(mat, i, j, sqrt(pow( ( (double) i-c ) /c, 2.0) + pow( ( (double) j-c ) /c, 2.0)));
}

void Zernike::calc_phi(gsl_matrix *mat) {
	// *mat should be square. Center is at:
	int c = mat->size1/2;
	io.msg(IO_DEB2, "Zernike::calc_phi(c=(%d,%d))", c, c);

	// every pixel will be atan2(-(i-c), (j-c))
	
	for (size_t i=0; i<mat->size1; i++)
		for (size_t j=0; j<mat->size2; j++)
			gsl_matrix_set(mat, i, j, atan2(-1.0*((double)i-c)/c, 1.0*((double)j-c)/c));
}
