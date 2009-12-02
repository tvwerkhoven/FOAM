/*
 *  imgio-test.cc
 *  foam
 *
 *  Created by Tim on 20091127.
 *  Copyright 2009 Tim van Werkhoven. All rights reserved.
 *
 */

#include <stdio.h>
#define __STDC_LIMIT_MACROS
#include <stdint.h>
#include <stdlib.h>
#include "imgio.h"
#include "io.h"

Io *io = new Io();

int main(int argc, char *argv[]) {
	int ret;
	
	// ########################################################################
	printf("Generating image.\n");
	int w=256;
	int h=128;
	
	uint8_t *img = (uint8_t *) malloc(w * h * sizeof(*img));
	
	for (int p=0; p<w*h; p++)
		img[p] = p*UINT8_MAX/(w*h);
	
	// ########################################################################
	printf("Save as FITS.\n");
	
	Imgio *saveimg = new Imgio();
	saveimg->data = (void *) img;
	saveimg->dtype = DATA_UINT8;
	saveimg->res.x = 256;
	saveimg->res.y = 128;
	saveimg->bitpix = 8;
	
	ret = saveimg->writeImg(Imgio::FITS, "/tmp/" __FILE__ "-fits.fits");
	
	// ########################################################################
	printf("Save as PGM.\n");
	
	ret = saveimg->writeImg(Imgio::PGM, "/tmp/" __FILE__ "-pgm.pgm");
	
	// ########################################################################
	printf("Save as nonsense.\n");
	
	ret = saveimg->writeImg(Imgio::UNDEF, "/tmp/" __FILE__ "-invalid.invalid");
	
	// ########################################################################
	
	long int s1, s2, diff;
	uint8_t *tmpimg;
	
	printf("Load FITS.\n");
	
	Imgio *fitsimg = new Imgio("/tmp/" __FILE__ "-fits.fits", Imgio::FITS);
	
	if (saveimg->res.x != fitsimg->res.x || saveimg->res.y != fitsimg->res.y) 
		printf("ERR: sizes don't match (%dx%d vs %dx%d)\n", saveimg->res.x, saveimg->res.y, fitsimg->res.x, fitsimg->res.y);
	if (fitsimg->getDtype() != saveimg->getDtype()) 
		printf("ERR: dtypes don't match. (%d vs %d)\n", saveimg->dtype, fitsimg->dtype);
	if (fitsimg->getBitpix() != saveimg->getBitpix()) 
		printf("ERR: bitpix's don't match (%d vs %d)\n", saveimg->bitpix, fitsimg->bitpix);
	
	diff = s1 = s2 = 0;
	tmpimg = (uint8_t *) fitsimg->data;
	for (int p=0; p<w*h; p++) {
		s1 += img[p];
		s2 += tmpimg[p];
	}
	diff = s1-s2;
		
	if (diff != 0) printf("ERR: img not equal! diff = %ld, (%ld vs %ld).\n", diff, s1, s2);
				
	// ########################################################################
	printf("Load PGM.\n");	

	Imgio *pgmimg = new Imgio("/tmp/" __FILE__ "-pgm.pgm", Imgio::PGM);
	
	if (saveimg->res.x != pgmimg->res.x || saveimg->res.y != pgmimg->res.y) 
		printf("ERR: sizes don't match.\n");
	if (pgmimg->getDtype() != saveimg->getDtype()) 
		printf("ERR: dtypes don't match.\n");
	if (pgmimg->getBitpix() != saveimg->getBitpix()) 
		printf("ERR: bitpix's don't match.\n");
	
	diff = s1 = s2 = 0;
	tmpimg = (uint8_t *) pgmimg->data;
	
	for (int p=0; p<w*h; p++) {
		s1 += img[p];
		s2 += tmpimg[p];
		if (img[p] != tmpimg[p]) printf("diff @ %d: %ld vs %ld\n", p, img[p], tmpimg[p]);
	}
	diff = s1-s2;
	
	if (diff != 0) printf("ERR: img not equal! diff = %ld, (%ld vs %ld).\n", diff, s1, s2);

	printf("Done.\n");
	
	return 0;
}

