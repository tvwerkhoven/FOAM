/*
 zernike.h -- Generate zernike modes -- header file
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


#ifndef HAVE_ZERNIKE_H
#define HAVE_ZERNIKE_H

#include <gsl/gsl_vector.h>
#include <gsl/gsl_matrix.h>

#include "io.h"
#include "types.h"

/*!
 @brief Generate Zernike modes on a square grid.
 
 */
class Zernike {
private:
	Io &io;
	
	typedef struct _zern_basis {
		_zern_basis(): nmodes(0), res(0,0), basisfuncs(NULL) { }
		int nmodes;												//!< Number of basis functions
		coord_t res;											//!< Resolution of grid
		gsl_matrix **basisfuncs;					//!< Array of basisfunctions
		gsl_matrix *rho;									//!< Matrix with radial coordinates as values
		gsl_matrix *phi;									//!< Matrix with azimuthal coordinates as values
	} zern_basis_t
	
	zern_basis_t basis;
	
	void setup();												//!< Allocate memory etc.
	
	gsl_matrix *zern_rad(int m, int n); //!< Generate radial zernike mode.
	
//  Python:
//	def zernike_rad(m, n, rho):
//	if (N.mod(n-m, 2) == 1):
//	return rho*0.0
//	
//	wf = rho*0.0
//	for k in range((n-m)/2+1):
//	wf += rho**(n-2.0*k) * (-1.0)**k * fac(n-k) / ( fac(k) * fac( (n+m)/2.0 - k ) * fac( (n-m)/2.0 - k ) )
	
public:
	Zernike(Io &io, int n=0, coord_t res=coord_t(128, 128));
	~Zernike();
	
	/*! @brief Generate Zernike mode j
	 
	 Noll introduced a single-integer mode counting. See http://oeis.org/A176988

	 @param [in] *outmat Store zernike mode here
	 @param [in] j Calculate Zernike mode j
	 @return 0 for ok, !0 for fail
	 */
	int gen_mode(gsl_matrix *outmat, coord_t res, int j) {
		io.msg(IO_XNFO, "Zernike::gen_mode(j=%d)", j);
		int n = 0;
		while (j > n) {
			n++;
			j -= n;
		}
		int m = -n + 2*j;
		return gen_mode(outmat, m, n);
	}		

	/*! @brief Generate Zernike mode n, m with n>=m.
	 
	 @param [in] *outmat Store zernike mode here
	 @param [in] n Zernike order
	 @param [in] m Zernike mode within order
	 @return 0 for ok, !0 for fail
	 */
	int gen_mode(gsl_matrix *outmat, coord_t res, int m, int n) { 
		io.msg(IO_XNFO, "Zernike::gen_mode(m=%d, n=%d)", m, n);
			
//		Python:
//		if (m > 0): return zernike_rad(m, n, rho) * N.cos(m * phi)
//		if (m < 0): return zernike_rad(-m, n, rho) * N.sin(-m * phi)
//		return zernike_rad(0, n, rho)
	}
	
	gsl_matrix* get_mode(int j);				//!< Return mode J
	
	gsl_matrix* get_modesum(gsl_vector *amplitudes, int nmax=-1);
	gsl_matrix* get_modesum(double *amplitudes, int nmax=-1);
	
};

#endif // HAVE_ZERNIKE_H
