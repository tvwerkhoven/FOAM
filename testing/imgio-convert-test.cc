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
#include "format.h"


int main(int argc, char *argv[]) {
	int ret;
	
	if (argc < 2) {
		printf("Syntax: %s <file>.\n", argv[0]);
		printf("Will convert <file> to <file>.fits.\n");
		return -1;		
	}
	
	Io io;
	
	// ########################################################################
	printf("Loading image.\n");

	Imgio *pgmimg = new Imgio(io, argv[1], Imgio::PGM);
	if (pgmimg->loadImg()) {
		printf("ERR: could not load image.\n");
		return -1;
	}
	
	printf("Image is %dx%dx%d\n", pgmimg->getWidth(), pgmimg->getHeight(), pgmimg->getBPP());
	
//	printf("Save as PGM.\n");
//	pgmimg->writeImg(Imgio::PGM, format("%s.pgm", argv[1]) );
	
	printf("Save as FITS.\n");
	pgmimg->writeImg(Imgio::FITS, format("%s.fits", argv[1]) );
	
	printf("Done.\n");
	
	return 0;
}

