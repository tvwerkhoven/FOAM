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
#include <gsl/gsl_sf_gamma.h>
#include <strings.h>
#include <math.h>

#include "io.h"
#include "types.h"

#include "zernike.h"

Zernike::Zernike(Io &io, int n, int size):
io(io), scratch(NULL) {
	io.msg(IO_DEB2, "Zernike::Zernike(n=%d, size=%d)", n, size);
	
	setup(n, size);
	
	calc_basis();
}

Zernike::~Zernike() {
	io.msg(IO_DEB2, "Zernike::~Zernike()");
	// Free used memory
	gsl_matrix_free(basis.rho);
	gsl_matrix_free(basis.phi);
	gsl_matrix_free(basis.cropmask);
	for (int i=0; i<basis.nmodes; i++)
		gsl_matrix_free((basis.bfuncs)[i]);
	
	free(basis.bfuncs);
	
	gsl_matrix_free(scratch);
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
	
	// Allocate scratch matrix
	gsl_matrix_free(scratch);
	scratch = (gsl_matrix *) gsl_matrix_alloc(size, size);
	
	// Allocate cropmask
	gsl_matrix_free(basis.cropmask);
	basis.cropmask = (gsl_matrix *) gsl_matrix_alloc(size, size);
	
	// Calculate coordinate grids
	calc_rho(basis.rho);
	calc_phi(basis.phi);
	calc_crop(basis.cropmask);
	
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
	
	io.msg(IO_XNFO, "Zernike::setup(): allocate ok");
	
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

void Zernike::calc_crop(gsl_matrix *mat) {
	// *mat should be square. Center is at:
	int c = mat->size1/2;
	io.msg(IO_DEB2, "Zernike::calc_crop(c=(%d,%d))", c, c);
	
	// every pixel will be 1 within the largest circle that fits within mat, 0 outside.
	for (size_t i=0; i<mat->size1; i++) {
		for (size_t j=0; j<mat->size2; j++) {
			if (sqrt(pow( ( (double) i-c ) /c, 2.0) + pow( ( (double) j-c ) /c, 2.0))<1)
				gsl_matrix_set(mat, i, j, 1.0);
			else
				gsl_matrix_set(mat, i, j, 0);
		}
	}
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

int Zernike::calc_basis(bool force) { 
	io.msg(IO_DEB2, "Zernike::calc_basis(%d)", force);

	if (!basis.nmodes || !basis.rho || !basis.phi)
		return io.msg(IO_ERR, "Zernike::calc_basis(): error: basis not configured properly"); 
	
	if (basis.is_calc && !force)
		return 0;
	
	for (int j=0; j<basis.nmodes; j++)
		if (gen_mode((basis.bfuncs)[j], basis.size, j))
			return io.msg(IO_ERR, "Zernike::calc_basis(): error while calculating mode j=%d", j); 
	
	basis.is_calc = true;
	return 0;
}

int Zernike::zern_rad(gsl_matrix * const mat, const int m, const int n) {
	//  Python:
	//	def zernike_rad(m, n, rho):
	//	if (N.mod(n-m, 2) == 1):
	//	return rho*0.0
	//	
	//	wf = rho*0.0
	//	for k in range((n-m)/2+1):
	//	wf += rho**(n-2.0*k) * (-1.0)**k * fac(n-k) / ( fac(k) * fac( (n+m)/2.0 - k ) * fac( (n-m)/2.0 - k ) )
	
	
	// If the difference in n and m is odd, give a flat matrix
	if ((n-m) % 2 == 1)
		return 0;

	gsl_matrix *tmp = scratch;

	for (int k=0; k <= (n-m)/2; k++) {
		// Calculate fixed factors for this 'k'
		double nom = pow(-1.0, k) * gsl_sf_fact(n-k);
		double denom = ( gsl_sf_fact(k) * gsl_sf_fact( (n+m)/2.0 - k ) * gsl_sf_fact( (n-m)/2.0 - k ) );
		
		// Loop over tmp matrix, calculate rho**(n-2.0*k)
		for (size_t i=0; i<mat->size1; i++)
			for (size_t j=0; j<mat->size2; j++)
				gsl_matrix_set(tmp, i, j, pow(gsl_matrix_get(basis.rho, i, j), (n-2.0*k)));
		
		// Multiply tmp matrix with scale factor and add to output matrix
		gsl_matrix_scale(tmp, nom/denom);
		gsl_matrix_add(mat, tmp);
	}
}

int Zernike::gen_mode(gsl_matrix * const outmat, const int size, int j) {
	io.msg(IO_XNFO, "Zernike::gen_mode(j=%d)", j);
	int n = 0;
	while (j > n) {
		n++;
		j -= n;
	}
	int m = -n + 2*j;
	return gen_mode(outmat, size, m, n);
}

int Zernike::gen_mode(gsl_matrix * const outmat, const int size, const int m, const int n) { 
	io.msg(IO_XNFO, "Zernike::gen_mode(size=%d, m=%d, n=%d, out=%p, scratch=%p)", size, m, n, outmat, scratch);

	//		Python:
	//		if (m > 0): return zernike_rad(m, n, rho) * N.cos(m * phi)
	//		if (m < 0): return zernike_rad(-m, n, rho) * N.sin(-m * phi)
	//		return zernike_rad(0, n, rho)
	
	if (m > 0) {
		// Calculate radial component
		zern_rad(outmat, m, n);
		
		gsl_matrix *tmp = scratch;
		for (size_t i=0; i<tmp->size1; i++)
			for (size_t j=0; j<tmp->size2; j++)
				gsl_matrix_set(tmp, i, j, cos((double)m * gsl_matrix_get(basis.phi, i, j)));
		
		return gsl_matrix_mul_elements (outmat, tmp);
	} 
	else if (m < 0) {
		// Calculate radial component
		zern_rad(outmat, -m, n);
		
		// Calculate azimuthal component
		gsl_matrix *tmp = scratch;
		for (size_t i=0; i<tmp->size1; i++)
			for (size_t j=0; j<tmp->size2; j++)
				gsl_matrix_set(tmp, i, j, sin((double)-1.0*m * gsl_matrix_get(basis.phi, i, j)));
		
		// Combine the two above
		return gsl_matrix_mul_elements (outmat, tmp);
	} 
	else {
		return zern_rad(outmat, 0, n);
	}
	return -1;
}

gsl_matrix* Zernike::get_mode(const int j, bool copy, bool crop) { 
	if (!basis.is_calc)
		return NULL;

	if (crop)
		copy = true;
	
	if (!copy)
		return (basis.bfuncs)[j]; 
	else {
		gsl_matrix *out = gsl_matrix_alloc(basis.size, basis.size);
		gsl_matrix_memcpy(out, (basis.bfuncs)[j]);
		if (crop)
			gsl_matrix_mul_elements(out, basis.cropmask);
		
		return out;
	}
}

gsl_matrix* Zernike::get_modesum(gsl_vector *amplitudes, bool crop) {
	if (!basis.is_calc)
		return NULL;
	
	// Allocate output matrix, set to 0
	gsl_matrix *out = gsl_matrix_calloc(basis.size, basis.size);
	
	if (amplitudes->size > basis.nmodes)
		io.msg(IO_WARN, "Zernike::get_modesum(): len(amp_vector) > basis.nmodes!");
	
	// Calculate maximum mode to return
	int maxmode = min((int) amplitudes->size, basis.nmodes);
	
	for (int m=0; m<maxmode; m++) {
		// Copy mode m to scratch matrix, multiply with amplitude. Skip if 0
		if (gsl_vector_get(amplitudes, m) == 0)
			continue;
		
		// Copy mode, scale, add to output
		gsl_matrix_memcpy(scratch, (basis.bfuncs)[m]);
		gsl_matrix_scale(scratch, gsl_vector_get(amplitudes, m));
		gsl_matrix_add(out, scratch);
	}
	
	if (crop)
		gsl_matrix_mul_elements(out, basis.cropmask);
	
	return out;
}
