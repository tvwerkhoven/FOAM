/*
 zern-test.cc -- test Zernike:: class.
 
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

#include <gsl/gsl_matrix.h>
#include <math.h>
#include "imgdata.h"

#include "zernike.h"


int main() {
	printf("zern-test.cc:: init\n");
	Io io(8);
	
	int nm=16;
	int size=256;
	
	printf("zern-test.cc:: init Zernike()\n");
	Zernike zerntest(io, nm, size);
	
	gsl_matrix *phi = zerntest.get_phi();
	gsl_matrix *rho = zerntest.get_rho();
	
	printf("zern-test.cc:: phi size1=%zu, size2=%zu, tda=%zu\n", phi->size1, phi->size2, phi->tda);

	
	printf("zern-test.cc:: phi (%d, %d) = %g\n", size/2, size/2, gsl_matrix_get(phi, size/2, size/2));
	printf("zern-test.cc:: phi (0, 0) = %g\n", gsl_matrix_get(phi, 0, 0));
	printf("zern-test.cc:: phi (%d,%d) = %g\n", size/10, 9*size/10, gsl_matrix_get(phi, size/10, 9*size/10));

	printf("zern-test.cc:: rho (%d, %d) = %g\n", size/2, size/2, gsl_matrix_get(rho, size/2, size/2));
	printf("zern-test.cc:: rho (0, 0) = %g\n", gsl_matrix_get(rho, 0, 0));
	printf("zern-test.cc:: rho (%d,%d) = %g\n", size/10, 9*size/10, gsl_matrix_get(rho, size/10, 9*size/10));
	
	ImgData phidat(io, (gsl_matrix *) phi, true);
	phidat.writedata("./zern-test_phidata.fits", ImgData::FITS, true);
	
	printf("zern-test.cc:: imgdata.phi (%d, %d) = %g\n", size/2, size/2, phidat.getpixel(size/2, size/2));
	printf("zern-test.cc:: imgdata.phi (%d, %d) = %g\n", 0, 0, phidat.getpixel(0, 0));
	printf("zern-test.cc:: imgdata.phi (%d, %d) = %g\n", size/10, 9*size/10, phidat.getpixel(size/10, 9*size/10));
	printf("zern-test.cc:: imgdata.phi (%d, %d) = %g\n", 9*size/10, size/10, phidat.getpixel(9*size/10, size/10));
	printf("zern-test.cc:: imgdata.phi dt == FLOAT64: %d\n", phidat.getdtype() == FLOAT64);
	
	ImgData rhodat(io, (gsl_matrix *) rho, true);
	rhodat.writedata("./zern-test_rhodata.fits", ImgData::FITS, true);
	
	return 0;
}
