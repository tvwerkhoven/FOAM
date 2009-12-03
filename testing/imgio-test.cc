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
	
	uint16_t *img = (uint16_t *) malloc(w * h * sizeof(*img));
	
	for (int p=0; p<w*h; p++)
		img[p] = p*UINT16_MAX/(w*h);
	
	// ########################################################################
	printf("Save as FITS.\n");
	
	Imgio *saveimg = new Imgio();
	saveimg->data = (void *) img;
	saveimg->dtype = DATA_UINT16;
	saveimg->res.x = 256;
	saveimg->res.y = 128;
	saveimg->bpp = 16;
	
	ret = saveimg->writeImg(Imgio::FITS, "/tmp/" __FILE__ "-fits.fits");
	
	// ########################################################################
	printf("Save as PGM.\n");
	
	ret = saveimg->writeImg(Imgio::PGM, "/tmp/" __FILE__ "-pgm.pgm");
	if (!ret) printf("Success!\n");
	
	// ########################################################################
	printf("Save as nonsense.\n");
	
	ret = saveimg->writeImg(Imgio::UNDEF, "/tmp/" __FILE__ "-invalid.invalid");
	if (!ret) printf("Success!\n");
	
	// ########################################################################
	
	long int s1, s2, diff;
	uint16_t *tmpimg;
	
	printf("Load FITS.\n");
	
	Imgio *fitsimg = new Imgio("/tmp/" __FILE__ "-fits.fits", Imgio::FITS);

	if (fitsimg->loadImg()) {
		printf("ERR: could not load imaage.\n");
	}
	else {		
		if (saveimg->res.x != fitsimg->res.x || saveimg->res.y != fitsimg->res.y) 
			printf("ERR: sizes don't match (%dx%d vs %dx%d)\n", saveimg->res.x, saveimg->res.y, fitsimg->res.x, fitsimg->res.y);
		if (fitsimg->getDtype() != saveimg->getDtype()) 
			printf("ERR: dtypes don't match. (%d vs %d)\n", saveimg->dtype, fitsimg->dtype);
		if (fitsimg->getBitpix() != saveimg->getBitpix()) 
			printf("ERR: bitpix's don't match (%d vs %d)\n", saveimg->getBitpix(), fitsimg->getBitpix());
		
		diff = s1 = s2 = 0;
		tmpimg = (uint16_t *) fitsimg->data;
		for (int p=0; p<w*h; p++) {
			s1 += img[p];
			s2 += tmpimg[p];
			if (img[p] != tmpimg[p]) printf("diff @ %d: %d vs %d\n", p, img[p], tmpimg[p]);
		}
		diff = s1-s2;
			
		if (diff != 0) printf("ERR: img not equal! diff = %ld, (%ld vs %ld).\n", diff, s1, s2);
		
		printf("Got %dx%dx%d, sum=%lld", fitsimg->res.x, fitsimg->res.y, fitsimg->bpp, fitsimg->sum);
		printf(" (%ld, %ld, diff=%ld), range=%d--%d\n", s1, s2, diff, fitsimg->range[0], fitsimg->range[1]);
	}
				
	// ########################################################################
	printf("Load PGM.\n");	

	Imgio *pgmimg = new Imgio("/tmp/" __FILE__ "-pgm.pgm", Imgio::PGM);
	if (pgmimg->loadImg()) {
		printf("ERR: could not load imaage.\n");
	}
	else {		
		if (saveimg->res.x != pgmimg->res.x || saveimg->res.y != pgmimg->res.y) 
			printf("ERR: sizes don't match.\n");
		if (pgmimg->getDtype() != saveimg->getDtype()) 
			printf("ERR: dtypes don't match.\n");
		if (pgmimg->getBitpix() != saveimg->getBitpix()) 
			printf("ERR: bitpix's don't match.\n");
		
		diff = s1 = s2 = 0;
		tmpimg = (uint16_t *) pgmimg->data;
		
		for (int p=0; p<w*h; p++) {
			s1 += img[p];
			s2 += tmpimg[p];
			if (img[p] != tmpimg[p]) printf("diff @ %d: %d vs %d\n", p, img[p], tmpimg[p]);
		}
		diff = s1-s2;
		
		if (diff != 0) printf("ERR: img not equal! diff = %ld, (%ld vs %ld).\n", diff, s1, s2);
		
		printf("Got %dx%dx%d, sum=%lld",fitsimg->res.x, fitsimg->res.y, fitsimg->bpp, fitsimg->sum);
		printf(" (%ld, %ld, diff=%ld), range=%d--%d\n", s1, s2, diff, fitsimg->range[0], fitsimg->range[1]);
	}

	delete pgmimg;
	delete fitsimg;
	delete saveimg;
	
	printf("Done.\n");
	
	return 0;
}

