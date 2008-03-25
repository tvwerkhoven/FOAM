/*! 
	@file foam_modules-pgm.c
	@author @authortim
	@date 2008-02-25 

	@brief This file contains functions to read and write pgm files.
	
	\section Info
	This module can be used to read and write SDL_Surfaces from and to files.
	
	\section Functions
	
	The functions provided to the outside world are:
	\li modReadPGM() - Read a PGM file into a SDL_Surface.
	\li modWritePGM() - Write an ASCII PGM file.

	\section Dependencies
	
	This module does not depend on other modules.
	
	This module requires the following libraries:
	\li SDL
	\li SDL_Image

	\section License
	This code is licensed under the GPL, version 2.	
*/

// HEADERS //
/***********/

#include "foam_modules-pgm.h"

// ROUTINES //
/************/

// int modReadPGM(char *fname, SDL_Surface **img) {
// 	
// 	*img = IMG_Load(fname);
// 	if (!*img) {
// 		logWarn("Error in IMG_Load: %s\n", IMG_GetError());
// 		return EXIT_FAILURE;
// 	}
// 	
// 	logDebug(0, "Loaded image %s (%dx%d)", fname, (*img)->w, (*img)->h);
// 
// 	return EXIT_SUCCESS;
// }

int modReadIMGArr(char *fname, float **img, int outres[2]) {
	SDL_Surface *sdlimg;
	int x, y;

	sdlimg = IMG_Load(fname);
	if (!sdlimg) {
		logWarn("Error in IMG_Load: %s\n", IMG_GetError());
		return EXIT_FAILURE;
	}

	// copy image from SDL_Surface to *img
	
	*img = calloc(sdlimg->w * sdlimg->h, sizeof(float));
	if (*img == NULL)
		logErr("Failed to allocate memory in modReadPGMArr().");
		
	outres[0] = sdlimg->w;
	outres[1] = sdlimg->h;
	for (y=0; y < sdlimg->h; y++) {
		for (x=0; x<sdlimg->w; x++) {
		
			(*img)[y*sdlimg->w + x] = (float) getPixel(sdlimg, x, y);
		}
	}

	logDebug(0, "ReadPGMArr Succesfully finished");
	return EXIT_SUCCESS;
}

int modWritePGM(char *fname, SDL_Surface *img) {
	FILE *fd;
	float max, min;
	min = max = getPixel(img, 0, 0);
	int x, y;
	
	fd = fopen(fname,"w+");
	if (!fd) {
		logWarn("Error, cannot open file %s: %s", fname, strerror(errno));
		return EXIT_FAILURE;
	}
	
	// check maximum & min
	for (x=0; x<img->w; x++) {
		for (y=0; y<img->h; y++) {
			pix = getPixel(img, x, y);
			if (pix > max) max = pix;
			else if (pix < min) min = pix;
		}
	}
	
	// We're making ascii
	fprintf(fd, "P2\n");
	fprintf(fd, "%d %d\n", img->w, img->h);
	fprintf(fd, "255\n");

	for (x=0; x<img->w; x++) {
		for (y=0; y<img->h; y++) {
			fprintf(fd, "%d ", (int) 255*(getPixel(img, x, y)-min/(max-min)));
		}
		fprintf(fd, "\n");
	}
	
	fclose(fd);
	return EXIT_SUCCESS;
}

int modWritePNG(char *fname, SDL_Surface *img) {
	FILE *fd;
	float max, min;
	min = max = getPixel(img, 0, 0);
	int x, y;
	int gray[256];
	
	gdImagePtr im;
	
	// allocate image
	im = gdImageCreate(img->w, img->h);
	
	// generate grayscales
	for (i=0; i<256; i++)
		gray[i] = gdImageColorAllocate(im, i, i, i);
	
	// check maximum & min
	for (x=0; x<img->w; x++) {
		for (y=0; y<img->h; y++) {
			pix = getPixel(img, x, y);
			if (pix > max) max = pix;
			else if (pix < min) min = pix;
		}
	}
	
	// set pixels in image
	for (x=0; x<img->w; x++) {
		for (y=0; y<img->h; y++) {
			pix = 255*(getPixel(img, x, y)-min)/(max-min);
			gdImageSetPixel(im, x, y, gray[(int) pix]);
		}
	}
	
	// write image to file as png, wb is necessary under dos, harmless under linux
	fd = fopen("screencap.png", "wb");
	if (!fd) return EXIT_FAILURE
	gdImagePng(im, pngout);
	fclose(fd);
	
	// write image to file as jpg as well
	fd = fopen("screencap.jpg", "wb");
	if (!fd) return EXIT_FAILURE
	gdImageJpeg(im, jpegout, -1);
	fclose(fd);

	// destroy the image
	gdImageDestroy(im);

	return EXIT_SUCCESS;
}

Uint32 getPixel(SDL_Surface *surface, int x, int y) {
    int bpp = surface->format->BytesPerPixel;
    /* Here p is the address to the pixel we want to retrieve */
    Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;
    switch(bpp) {
    case 1:
        return *p;
    case 2:
        return *(Uint16 *)p;
    case 3:
        if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
            return p[0] << 16 | p[1] << 8 | p[2];
        else
            return p[0] | p[1] << 8 | p[2] << 16;
    case 4:
        return *(Uint32 *)p;
    default:
        return 0;       /* shouldn't happen, but avoids warnings */
    }
}
