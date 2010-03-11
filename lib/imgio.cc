/*
 imgio.cc -- image file input/output routines
 Copyright (C) 2008--2010 Tim van Werkhoven <t.i.m.vanwerkhoven@xs4all.nl>
 
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

Imgio::~Imgio(void) {
	io->msg(IO_DEB2, "Imgio::~Imgio(void)");
	if (data) free(data);
}

int Imgio::init(std::string fname, imgtype_t imgtype) {
	io->msg(IO_DEB2, "Imgio::init()");
	path = fname;
	imgt = imgtype;	
	data = NULL;
	res.x = res.y = -1;
	bpp = -1;
	range[0] = range[1] = 0;
	sum = 0L;
	return 0;
}

int Imgio::getPixel(int x, int y) {
	if (!data) return -1;
	if (dtype == DATA_UINT8) return ((uint8_t *)data)[y * res.x + x];
	else if (dtype == DATA_UINT16) return ((uint16_t *)data)[y * res.x + x];
	else return -1;
}

int Imgio::loadImg() {
	if (imgt == Imgio::FITS) return loadFits(path);
	else if (imgt == Imgio::PGM) return loadPgm(path);
	else return io->msg(IO_ERR, "Imgio::loadImg(): Unknown datatype");
}

int Imgio::writeImg(imgtype_t imgtype, std::string outpath) {
	if (imgtype == Imgio::FITS) return writeFits(outpath);
	else if (imgtype == Imgio::PGM) return writePgm(outpath);
	else return io->msg(IO_ERR, "Imgio::writeImg(): Unknown datatype");
}

int Imgio::calcRange() {
	switch (dtype) {
		case DATA_UINT8: {
			uint8_t *tmpdata = (uint8_t *) data;
			for (int p=0; p < res.x * res.y; p++) {
				if (tmpdata[p] > range[1]) range[1] = tmpdata[p];
				if (tmpdata[p] < range[0]) range[0] = tmpdata[p];
				sum += tmpdata[p];
			}
			break;
		}
		case DATA_UINT16: {
			uint16_t *tmpdata = (uint16_t *) data;
			for (int p=0; p<res.x * res.y; p++) {
				if (tmpdata[p] > range[1]) range[1] = tmpdata[p];
				if (tmpdata[p] < range[0]) range[0] = tmpdata[p];
				sum += tmpdata[p];
			}
			break;
		}
		default: {
			return io->msg(IO_ERR, "Imgio::calcRange(): Unknown datatype");
		}
	}
	return 0;
}

int Imgio::loadFits(std::string path) {
	fitsfile *fptr;
	char fits_err[30];
	int stat = 0, naxis;
	long naxes[2], nel = 0, fpixel[] = {1,1};
	int anynul = 0;
	float nulval = 0.0;
	
	fits_open_file(&fptr, path.c_str(), READONLY, &stat);
	if (stat) {
		fits_get_errstatus(stat, fits_err);
		return io->msg(IO_ERR, "fits error: %s", fits_err);
	}
			
	fits_get_img_param(fptr, 2, &bpp, &naxis, naxes, &stat);
	if (stat) {
		fits_get_errstatus(stat, fits_err);
		return io->msg(IO_ERR, "fits error: %s", fits_err);
	}
		
	res.x = naxes[0];
	res.y = naxes[1];
	
	nel = res.x * res.y;
	
	data = (void *) malloc(nel * bpp/8);
	
	// BYTE_IMG (8), SHORT_IMG (16), LONG_IMG (32), LONGLONG_IMG (64), FLOAT_IMG (-32), and DOUBLE_IMG (-64)
	// TBYTE, TSBYTE, TSHORT, TUSHORT, TINT, TUINT, TLONG, TLONGLONG, TULONG, TFLOAT, TDOUBLE
	
	switch (bpp) {
		case BYTE_IMG: {
			uint8_t nulval = 0;
			fits_read_img(fptr, TBYTE, 1, nel, &nulval, \
					(uint8_t *) data, &anynul, &stat);
			dtype = DATA_UINT8;
			break;
			
		}
		case SHORT_IMG: {
			uint16_t nulval = 0;
			fits_read_img(fptr, TUSHORT, 1, nel, &nulval, \
					(uint16_t *) data, &anynul, &stat);					
			dtype = DATA_UINT16;
			break;
		}
		default: {
			return io->msg(IO_ERR, "Unknown FITS datatype");
		}
	}
	
	calcRange();
	
	fits_close_file(fptr, &stat);

	if (stat) {
		fits_get_errstatus(stat, fits_err);
		return io->msg(IO_ERR, "fits error: %s", fits_err);
	}	
	
	return stat;
}

int Imgio::writeFits(std::string path) {
	fitsfile *fptr;
	int stat = 0;
	long fpixel = 1, naxis = 2, nelements;
	long naxes[2] = { res.x, res.y};
	
	nelements = naxes[0] * naxes[1];
	char fits_err[30];
	
	fits_create_file(&fptr, path.c_str(), &stat);
	
	if (stat) {
		fits_get_errstatus(stat, fits_err);
		return io->msg(IO_ERR, "fits error: %s", fits_err);
	}

	switch (bpp) {
		case BYTE_IMG: {
			fits_create_img(fptr, BYTE_IMG, naxis, naxes, &stat);
			fits_write_img(fptr, TBYTE, fpixel, nelements, \
				(uint8_t *) data, &stat);
			break;
		}
		case SHORT_IMG: {
			fits_create_img(fptr, USHORT_IMG, naxis, naxes, &stat);
			fits_write_img(fptr, TUSHORT, fpixel, nelements, \
				(uint16_t *) data, &stat);
			break;
		}
		default: {
			return io->msg(IO_ERR, "Unknown datatype");
		}
	}
	
	if (stat) {
		fits_get_errstatus(stat, fits_err);
		return io->msg(IO_ERR, "fits error: %s", fits_err);
	}	

	fits_close_file(fptr, &stat);

	if (stat) {
		fits_get_errstatus(stat, fits_err);
		return io->msg(IO_ERR, "fits error: %s", fits_err);
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
	res.x = readNumber(fd);
	res.y = readNumber(fd);
	long nel = res.x * res.y;
	
	if (res.x <= 0 || res.y <= 0)
		return io->msg(IO_ERR, "Unable to read image width and height");

  // Maxval
	maxval = readNumber(fd);
	if(maxval <= 0 || maxval > 65536)
		return io->msg(IO_ERR, "Unsupported PGM format");
	
	if (maxval <= 255) {
		dtype = DATA_UINT8;
		bpp = 8;
		data = malloc(nel * bpp/8);		
	}
	else {		
		dtype = DATA_UINT16;
		bpp = 16;
		data = malloc(nel * bpp/8);		
	}
	
	// Read the rest
	if (!strncmp(magic, "P5", 2)) { // Binary
		n = fread(data, bpp/8, nel, fd);
		if (ferror(fd))
			return io->msg(IO_ERR, "Could not read file.");
	}
	else if (!strncmp(magic, "P2", 2)) { // ASCII
		if (dtype == DATA_UINT8)
			for (int p=0; p < nel; p++)
				((uint8_t *) data)[p] = readNumber(fd);
		else
			for (int p=0; p < nel; p++)
				((uint16_t *) data)[p] = readNumber(fd);
			
	}
	else
		return io->msg(IO_ERR, "Unsupported PGM format");
	
	calcRange();
	
	return 0;
}

int Imgio::writePgm(std::string path) {
	FILE *fd;
 	fd = fopen(path.c_str(), "wb+");
	
	if (dtype != DATA_UINT8 && dtype != DATA_UINT16)
		return io->msg(IO_ERR, "PGM only supports unsigned 8- or 16-bit integer images.");
	
	int maxval=0;
	switch (dtype) {
		case DATA_UINT8: {
			uint8_t *datap = (uint8_t *) data;
			for (int p=0; p<res.x * res.y; p++)
				if (datap[p] > maxval) maxval = datap[p];
			fprintf(fd, "P5\n%d %d\n%d\n", res.x, res.y, maxval);
			fwrite(datap, res.x * res.y * bpp/8, 1, fd);
			break;
		}
		case DATA_UINT16: {
			uint16_t *datap = (uint16_t *) data;
			for (int p=0; p<res.x * res.y; p++)
				if (datap[p] > maxval) maxval = datap[p];
			fprintf(fd, "P5\n%d %d\n%d\n", res.x, res.y, maxval);
			fwrite(datap, res.x * res.y * bpp/8, 1, fd);
			break;
		}			
		default:
			break;
	}
	
	fclose(fd);
	
	return 0;
}

int readNumber(FILE *fd) {
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
