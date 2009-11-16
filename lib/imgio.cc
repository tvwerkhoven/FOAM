/*
 imgio.cc -- image file input/output routines
 Copyright (C) 2008-2009 Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)
 
 This file is part of FOAM.
 
 FOAM is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 FOAM is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with FOAM.	If not, see <http://www.gnu.org/licenses/>.

 */
/*! 
 @file imgio.cc 
 @author Tim van Werkhoven
 @brief input/output routines for image files

*/

#include <stdio.h>
#include <fitsio.h>

#include <string>

#include "io.h"
#include "imgio.h"

extern Io *io;

Imgio::Imgio(std::string fname, int dtype) {
	path = fname;
	dtype = dtype;
}

int Imgio::loadImg() {
	if (dtype == IMGIO_FITS) return loadFits(path);
	else if (dtype == IMGIO_PGM) return loadPgm(path);
	else return -1;
}

int Imgio::writeImg(int dtype, std::string outpath) {
	if (dtype == IMGIO_FITS) return writeFits(outpath);
	else if (dtype == IMGIO_PGM) return writePgm(outpath);
	else return -1;
}


Imgio::~Imgio(void) {
	if (data) free(data);
}

int Imgio::loadFits(std::string path) {
	fitsfile *fptr;
	char fits_err[30];
	int stat = 0, naxis;
	long naxes[2], fpixel[] = {1,1};
	int anynul = 0;
	float nulval = 0.0;
	
	fits_open_file(&fptr, path.c_str(), READONLY, &stat);
	fits_get_img_param(fptr, 2, &bitpix, &naxis, naxes, &stat);
	
	res.x = naxes[0];
	res.y = naxes[1];
	
	data = malloc(res.x * res.y * bitpix/8);
	
	// BYTE_IMG (8), SHORT_IMG (16), LONG_IMG (32), LONGLONG_IMG (64), FLOAT_IMG (-32), and DOUBLE_IMG (-64)
	// TBYTE, TSBYTE, TSHORT, TUSHORT, TINT, TUINT, TLONG, TLONGLONG, TULONG, TFLOAT, TDOUBLE
	
	switch (bitpix) {
		case BYTE_IMG: {
			int8_t nulval = 0;
			fits_read_img(fptr, TBYTE, 0, res.x * res.y, &nulval, \
					(int8_t *) data, &anynul, &stat);
			break;
			
		}
		case SHORT_IMG: {
			int16_t nulval = 0;
			fits_read_img(fptr, TSHORT, 0, res.x * res.y, &nulval, \
					(int16_t *) data, &anynul, &stat);					
			break;
		}
		default: {
			err = "Unknown FITS datatype."
			return -1;
		}
	}

	if (stat) {
		fits_get_errstatus(stat, fits_err);
		err.assign(fits_err);
	}
	
	return stat;
}

int Imgio::writeFits(std::string outpath) {
	fitsfile *fptr;
	int status, ii, jj;
	long fpixel = 1, naxis = 2, nelements, exposure;
	long naxes[2] = { res.x, res.y };
	
	stat = 0;
	nelements = naxes[0] * naxes[1];
	
	fits_create_file(&fptr, path.c_str(), &stat);
	
	switch (bitpix) {
		case BYTE_IMG: {
			fits_create_img(fptr, BYTE_IMG, naxis, naxes, &stat);
			fits_write_img(fptr, TBYTE, fpixel, nelements, \
				(int8_t *) data, &stat);
			break;
		}
		case SHORT_IMG: {
			fits_create_img(fptr, SHORT_IMG, naxis, naxes, &stat);
			fits_write_img(fptr, TSHORT, fpixel, nelements, \
				(int16_t *) data, &stat);
			break;
		}
		default: {
			err = "Unknown FITS datatype."
			return -1;
		}
	}

	fits_close_file(fptr, &stat);

	if (stat) {
		fits_get_errstatus(stat, fits_err);
		err.assign(fits_err);
	}
	
	return stat;
}

int Imgio::loadPgm(std::string path) {
	// see http://netpbm.sourceforge.net/doc/pgm.html
	FILE *fd;
 	fd = fopen(path.c_str(), "rb");
	int n, maxval;
	char magic[2];
	
	// Read magic
	if (fread(magic, 2, 1, fd) != 2)
		return io->msg(IO_ERR, "Error reading PGM file.");

	// Resolution
	res.x = readNumber(fd);
	res.y = readNumber(fd);
	
	if (res.x <= 0 || res.y <= 0)
		return io->msg(IO_ERR, "Unable to read image width and height");

  // Maxval
	maxval = readNumber(fd);
	if(maxval <= 0 || maxval > 255)
		return io->msg(IO_ERR, "Unsupported PGM format");
	
	dtype = IMGIO_UBYTE;
	
	// Read the rest
	if (strcmp(magic, "P5")) { // Binary
		n = fread(data, res.x * res.y, 1, fd);
		if (n != res.x * res.y)
			return io->msg(IO_ERR, "Could not read file.");
	}
	else if (strcmp(magic, "P2")) { // ASCII
		for (int p=0; p < res.x * res.y; p++)
			(uint8_t *data)[p] = readNumber(fd);
	}
	else
		return io->msg(IO_ERR, "Unsupported PGM format");
	
	return 0;
}

static int readNumber(FILE *fd) {
	int number;
	unsigned char ch;

	number = 0;

	do {
		if (!fread(&ch, 1, 1, fd)) return -1;
		if ( ch == '#' ) {  // Comment is '#' to end of line
			do {
				if (!fread(&ch, 1, 1, fd)) return -1;
			} while ( (ch != '\r') && (ch != '\n') );
		}
	} while ( isspace(ch) );

	// Read number
	do {
		number *= 10;
		number += ch-'0';

		if (!fread(&ch, 1, 1, fd)) return -1;
	} while ( isdigit(ch) );

	return number;
}