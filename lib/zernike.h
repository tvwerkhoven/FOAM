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
		_zern_basis(): nmodes(0), size(0), is_calc(false), bfuncs(NULL), rho(NULL), phi(NULL), cropmask(NULL) { }
		int nmodes;												//!< Number of basis functions
		int size;													//!< Resolution of grid (always square, so only one int)
		bool is_calc;											//!< Indicates whether basis functions are computed or not
		gsl_matrix **bfuncs;							//!< Array of basisfunctions
		gsl_matrix *rho;									//!< Matrix with radial coordinates as values
		gsl_matrix *phi;									//!< Matrix with azimuthal coordinates as values
		gsl_matrix *cropmask;							//!< Circular cropmask @todo
	} zern_basis_t; //!< Struct for holding a Zernike set of basis functions.
	
	gsl_matrix *scratch;								//!< Temporary matrix that can be used for calculations

	zern_basis_t basis;									//!< Basis of Zernike functions is stored here, with metadata
	
	void calc_rho(gsl_matrix *mat);			//!< Calculate rho (radial) matrix. Each element gives is the distance to the center of the matrix
	void calc_phi(gsl_matrix *mat);			//!< Calculate phi (azimuthal) matrix. Each element gives the angle wrt the 'x-axis'
	void calc_crop(gsl_matrix *mat);		//!< Calculate cropmask: 1 within a circle, 0 outside
	int calc_basis(bool f=true);				//!< Calculate basis functions, as set up previously

	int zern_rad(gsl_matrix *const mat, const int m, const int n); //!< Generate radial zernike mode.
	
public:
	Zernike(Io &io, int n, int size=128);
	~Zernike();
	
	int setup(const int n, const int size); //!< Allocate memory et cetera
	
	/*! @brief Generate Zernike mode j
	 
	 Noll introduced a single-integer mode counting. See http://oeis.org/A176988

	 @param [in] *outmat Store zernike mode here (pre-allocated)
	 @param [in] j Calculate Zernike mode j
	 @return 0 for ok, !0 for fail
	 */
	int gen_mode(gsl_matrix * const outmat, const int size, const int j);
	
	/*! @brief Generate Zernike mode n, m with n>=m.
	 
	 @param [in] *outmat Store zernike mode here (pre-allocated)
	 @param [in] n Zernike order
	 @param [in] m Zernike mode within order
	 @return 0 for ok, !0 for fail
	 */
	int gen_mode(gsl_matrix * const outmat, const int size, const int m, const int n);
	
	int get_is_calc() { return (basis.is_calc); }
	int get_nmodes() { return (basis.nmodes); }
	int get_size() { return (basis.size); }
	gsl_matrix* get_phi() const { return basis.phi; }
	gsl_matrix* get_rho() const { return basis.rho; }
	
	/*! @brief Return mode j, optionally copy and crop
	 
	 @param [in] j Return this mode. Must be < basis.nmodes
	 @param [in] copy Copy matrix or return reference only
	 @param [in] crop Crop matrix with basis.cropmask (i.e. circle) or not. Implies 'copy=true'
	 @return gsl_matrix* pointing to Zernike mode j
	 */
	gsl_matrix* get_mode(const int j, bool copy=false, bool crop=true);
	
	gsl_matrix* get_modesum(gsl_vector *amplitudes, bool crop=true); //!< Calculate sum of modes
	//gsl_matrix* get_modesum(double *amplitudes, int namp, bool crop=true); //!< Calculate sum of modes
	
};

#endif // HAVE_ZERNIKE_H
