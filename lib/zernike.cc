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

#include "io.h"
#include "types.h"

#include "zernike.h"

Zernike::Zernike(Io &io, int n):
io(io), {
	io.msg(IO_DEB2, "Zernike::Zernike(n=%d)", n);
	
}

Zernike::~Zernike() {
	io.msg(IO_DEB2, "Zernike::~Zernike()");
}


Zernike::setup() {
	io.msg(IO_DEB2, "Zernike::setup() for %d modes", basis.nmodes);
}