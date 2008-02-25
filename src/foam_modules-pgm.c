/*! 
	@file foam_modules-pgm.c
	@author @authortim
	@date 2008-02-25 

	@brief This file contains functions to read and write pgm files. It uses SDL_Image to do this.
*/

// HEADERS //
/***********/

#include "foam_modules-pgm.h"

int modReadPGM(char *fname, SDL_Surface **img) {
	
	*img = IMG_Load(fname);
	if (!img) {
		logErr("Error in IMG_Load: %s\n", IMG_GetError());
		return EXIT_FAILURE;
	}
	
	logDebug("Loaded image %s (%dx%d)", fname, (*img)->w, (*img)->h);

	return EXIT_SUCCESS;
}

int modWritePGM(char *fname, SDL_Surface *img) {
	FILE *fd;
	float max=getpixel(img, 0, 0);
	int x, y;
	
	fd = fopen(fname,"w+");
	if (!fd) 
		logErr("Error, cannot open file %s: %s", fname, strerror(errno));

	// check maximum
	for (x=0; x<img->w; x++) 
		for (y=0; y<img->h; y++) 
			if (getpixel(img, x, y) > max) max = getpixel(img, x, y);
	
	// We're making ascii
	fprintf(fd, "P2\n");
	fprintf(fd, "%d %d\n", img->w, img->h);
	fprintf(fd, "255\n");

	for (x=0; x<img->w; x++) {
		for (y=0; y<img->h; y++) {
			fprintf(fd, "%d ", (int) (getpixel(img, x, y)*255/max));
		}
		fprintf(fd, "\n");
	}
	
	fclose(fd);
	return EXIT_SUCCESS;
}

