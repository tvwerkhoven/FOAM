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
#include <string.h>

#include <string>

#include "io.h"
#include "imgio.h"

extern Io *io;

Imgio::Imgio(std::string fname, int dtype) {
	path = fname;
	img = new img_t;
	img->dtype = dtype;
}

Imgio::~Imgio(void) {
	if (img->data) free(img->data);
}

int Imgio::loadImg() {
	if (img->dtype == IMGIO_FITS) return loadFits(path);
	else if (img->dtype == IMGIO_PGM) return loadPgm(path);
	else return io->msg(IO_ERR, "Unknown datatype");
}

int Imgio::writeImg(int dtype, std::string outpath) {
	if (img->dtype == IMGIO_FITS) return writeFits(outpath);
	else if (img->dtype == IMGIO_PGM) return writePgm(outpath);
	else return io->msg(IO_ERR, "Unknown datatype");
}


int Imgio::loadFits(std::string path) {
	fitsfile *fptr;
	char fits_err[30];
	int stat = 0, naxis;
	long naxes[2], nel = 0, fpixel[] = {1,1};
	int anynul = 0;
	float nulval = 0.0;
	
	fits_open_file(&fptr, path.c_str(), READONLY, &stat);
	fits_get_img_param(fptr, 2, &img->bitpix, &naxis, naxes, &stat);
	
	img->res.x = naxes[0];
	img->res.y = naxes[1];
	nel = img->res.x * img->res.y;
	
	img->data = malloc(nel * img->bitpix/8);
	
	// BYTE_IMG (8), SHORT_IMG (16), LONG_IMG (32), LONGLONG_IMG (64), FLOAT_IMG (-32), and DOUBLE_IMG (-64)
	// TBYTE, TSBYTE, TSHORT, TUSHORT, TINT, TUINT, TLONG, TLONGLONG, TULONG, TFLOAT, TDOUBLE
	
	switch (img->bitpix) {
		case BYTE_IMG: {
			int8_t nulval = 0;
			fits_read_img(fptr, TBYTE, 0, nel, &nulval, \
					(int8_t *) img->data, &anynul, &stat);
			break;
			
		}
		case SHORT_IMG: {
			int16_t nulval = 0;
			fits_read_img(fptr, TSHORT, 0, nel, &nulval, \
					(int16_t *) img->data, &anynul, &stat);					
			break;
		}
		default: {
			strerr = "Unknown FITS datatype.";
			return -1;
		}
	}

	if (stat) {
		fits_get_errstatus(stat, fits_err);
		strerr.assign(fits_err);
	}
	
	return stat;
}

int Imgio::writeFits(std::string outpath) {
	fitsfile *fptr;
	int stat = 0;
	long fpixel = 1, naxis = 2, nelements;
	long naxes[2] = { img->res.x, img->res.y};
	
	nelements = naxes[0] * naxes[1];
	
	fits_create_file(&fptr, path.c_str(), &stat);
	
	switch (img->bitpix) {
		case BYTE_IMG: {
			fits_create_img(fptr, BYTE_IMG, naxis, naxes, &stat);
			fits_write_img(fptr, TBYTE, fpixel, nelements, \
				(int8_t *) img->data, &stat);
			break;
		}
		case SHORT_IMG: {
			fits_create_img(fptr, SHORT_IMG, naxis, naxes, &stat);
			fits_write_img(fptr, TSHORT, fpixel, nelements, \
				(int16_t *) img->data, &stat);
			break;
		}
		default: {
			strerr = "Unknown FITS datatype.";
			return -1;
		}
	}

	fits_close_file(fptr, &stat);

	if (stat) {
		char fits_err[30];
		fits_get_errstatus(stat, fits_err);
		strerr.assign(fits_err);
	}
	
	return stat;
}

int Imgio::loadPgm(std::string path) {
	// see http://netpbm.sourceforge.net/doc/pgm.html
	FILE *fd;
	int n, maxval;
	char magic[2];
	
 	fd = fopen(path.c_str(), "rb");
	if (!fd)
		return io->msg(IO_ERR, "Error opening file '%s'.", path.c_str());
	
	// Read magic
	fread(magic, 2, 1, fd);
	if (ferror(fd))
		return io->msg(IO_ERR, "Error reading PGM file.");

	// Resolution
	img->res.x = readNumber(fd);
	img->res.y = readNumber(fd);
	long nel = img->res.x * img->res.y;
	
	if (img->res.x <= 0 || img->res.y <= 0)
		return io->msg(IO_ERR, "Unable to read image width and height");

  // Maxval
	maxval = readNumber(fd);
	if(maxval <= 0 || maxval > 255)
		return io->msg(IO_ERR, "Unsupported PGM format");
	
	img->dtype = IMGIO_UBYTE;
	img->bitpix = 8;
	img->data = malloc(nel * img->bitpix/8);
	
	// Read the rest
	if (strcmp(magic, "P5")) { // Binary
		n = fread(img->data, nel, 1, fd);
		if (ferror(fd))
			return io->msg(IO_ERR, "Could not read file.");
	}
	else if (strcmp(magic, "P2")) { // ASCII
		for (int p=0; p < nel; p++)
			((uint8_t *) img->data)[p] = readNumber(fd);
	}
	else
		return io->msg(IO_ERR, "Unsupported PGM format");
	
	return 0;
}

int Imgio::writePgm(std::string outpath) {
	FILE *fd;
 	fd = fopen(path.c_str(), "wb+");
	
	if (img->dtype != IMGIO_UBYTE || img->dtype != IMGIO_USHORT)
		return io->msg(IO_ERR, "PGM only supports unsigned 8- or 16-bit integer images.");
	
	int maxval=0;
	switch (img->dtype) {
		case IMGIO_UBYTE: {
			uint8_t *datap = (uint8_t *) img->data;
			for (int p=0; p<img->res.x * img->res.y; p++)
				if (datap[p] > maxval) maxval = datap[p];
			fprintf(fd, "P5\n%d %d\n%d\n", img->res.x, img->res.y, maxval);
			fwrite(datap, img->res.x * img->res.y * img->bitpix, 1, fd);
			break;
		}
		case IMGIO_USHORT: {
			uint16_t *datap = (uint16_t *) img->data;
			for (int p=0; p<img->res.x * img->res.y; p++)
				if (datap[p] > maxval) maxval = datap[p];
			fprintf(fd, "P5\n%d %d\n%d\n", img->res.x, img->res.y, maxval);
			fwrite(datap, img->res.x * img->res.y * img->bitpix, 1, fd);						 
			break;
		}			
		default:
			break;
	}
	
	fclose(fd);
	io->msg(IO_ERR, "Completed");
	
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
