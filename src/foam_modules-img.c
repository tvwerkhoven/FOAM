/*! 
	@file foam_modules-img.c
	@author @authortim
	@date 2008-02-25 

	@brief This file contains functions to read and write image files.
	
	\section Info
	This module can be used to read and write SDL_Surfaces from and to files.
	
	\section Functions
	
	The functions provided to the outside world are:
	\li modReadIMGArr() - Read an img (png, pgm, jpg, etc.) to an array.
	\li modWritePNGSurf() - Write an 8-bit ASCII PGM file from an SDL_Surface.
	\li modWritePNGArr() - Write an 8-bit ASCII PGM file from a float image.
	\li modWritePNGSurf() - Write an 8-bit grayscale PNG file from an SDL_Surface.

	\section Dependencies
	
	This module does not depend on other modules.
	
	This module requires the following libraries:
	\li SDL
	\li SDL_Image
	\li gd

	\section License
	This code is licensed under the GPL, version 2.	
*/

// HEADERS //
/***********/

#include "foam_modules-img.h"

// ROUTINES //
/************/

int modReadIMGSurf(char *fname, SDL_Surface **surf) {
	*surf = NULL;
	
	*surf = IMG_Load(fname);
	if (!*surf) {
		logWarn("Error in IMG_Load: %s\n", IMG_GetError());
		return EXIT_FAILURE;
	}
	
	return EXIT_SUCCESS;
}

int modReadIMGArr(char *fname, float **img, int outres[2]) {
	SDL_Surface *sdlimg;
	int x, y;
	
	if (modReadIMGSurf(fname, &sdlimg) != EXIT_SUCCESS)
		return EXIT_FAILURE;

	// !!!:tim:20080326 update we export this part to modReadIMGSurf such that 
//	sdlimg = IMG_Load(fname);
//	if (!sdlimg) {
//		logWarn("Error in IMG_Load: %s\n", IMG_GetError());
//		return EXIT_FAILURE;
//	}

	// copy image from SDL_Surface to *img
	*img = calloc(sdlimg->w * sdlimg->h, sizeof(float));
	if (*img == NULL)
		logErr("Failed to allocate memory in modReadIMGArr().");
		
	outres[0] = sdlimg->w;
	outres[1] = sdlimg->h;
	for (y=0; y < sdlimg->h; y++) {
		for (x=0; x<sdlimg->w; x++) {
			// beware: pointer trickery begins
			(*img)[y*sdlimg->w + x] = (float) getPixel(sdlimg, x, y);
		}
	}
	
	SDL_FreeSurface(sdlimg);

	logDebug(0, "ReadIMGArr Succesfully finished");
	return EXIT_SUCCESS;
}

int modWritePGMSurf(char *fname, SDL_Surface *img) {
	FILE *fd;
	float max, min, pix;
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
			// ???:tim:20080325 does this work? uint32 -> float conversion?
			// !!!:tim:20080326 yes, this works (tested in pngtest.c)
			pix = (float) getPixel(img, x, y);
			if (pix > max) max = pix;
			else if (pix < min) min = pix;
		}
	}
	
	// We're making ascii
	fprintf(fd, "P2\n");
	fprintf(fd, "%d %d\n", img->w, img->h);
	fprintf(fd, "255\n");

	for (y=0; y<img->h; y++) {
		for (x=0; x<img->w; x++) {
			fprintf(fd, "%d ", (int) (255 * (getPixel(img, x, y)-min) / (max-min)) );
		}
		fprintf(fd, "\n");
	}
	
	fclose(fd);
	return EXIT_SUCCESS;
}

int modWritePNGArr(char *fname, void *img, coord_t res, int type) {
	FILE *fd;
	
	switch (type) {
		case 0:
			float *imgc;
			imgc = (float *) img;
			break;
		case 1:
			unsigned char *imgc;
			imgc = (unsigned char *) img;
			break;
		case 2:
			int  *imgc;
			imgc = (int *) img;
			break;
		case 3:
			return EXIT_FAILURE;
			break;
	}
	
	float max, min, pix;
	min = max = (float) imgc[0];
	int x, y, i;
	int gray[256];
	
	gdImagePtr im;
	
	// allocate image
	im = gdImageCreate(res.x, res.y);
	
	// generate grayscales
	for (i=0; i<256; i++)
		gray[i] = gdImageColorAllocate(im, i, i, i);
	
	// check maximum & min
	for (x=0; x< res.x*res.y; x++) {
		pix = (float) imgc[x];
		if (pix > max) max = pix;
		else if (pix < min) min = pix;
	}
	
	// set pixels in image
	for (y=0; y< res.y; y++) {
		for (x=0; x< res.x; x++) {
			pix = 255*((float) imgc[y*res.x + x]-min)/(max-min);
			gdImageSetPixel(im, x, y, gray[(int) pix]);
		}
	}
	
	// write image to file as png, wb is necessary under dos, harmless under linux
	fd = fopen(fname, "wb");
	if (!fd) return EXIT_FAILURE;
	gdImagePng(im, fd);
	fclose(fd);
	
	// write image to file as jpg as well
	//	fd = fopen("screencap.jpg", "wb");
	//	if (!fd) return EXIT_FAILURE;
	//	gdImageJpeg(im, fd, -1);
	//	fclose(fd);
	
	// destroy the image
	gdImageDestroy(im);
	
	return EXIT_SUCCESS;
	
}

int modWritePNGSurf(char *fname, SDL_Surface *img) {
	FILE *fd;
	float max, min, pix;
	min = max = getPixel(img, 0, 0);
	int x, y, i;
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
	for (y=0; y<img->h; y++) {
		for (x=0; x<img->w; x++) {
			pix = 255*(getPixel(img, x, y)-min)/(max-min);
			gdImageSetPixel(im, x, y, gray[(int) pix]);
		}
	}
	
	// write image to file as png, wb is necessary under dos, harmless under linux
	fd = fopen(fname, "wb");
	if (!fd) return EXIT_FAILURE;
	gdImagePng(im, fd);
	fclose(fd);
	
	// write image to file as jpg as well
//	fd = fopen("screencap.jpg", "wb");
//	if (!fd) return EXIT_FAILURE;
//	gdImageJpeg(im, fd, -1);
//	fclose(fd);

	// destroy the image
	gdImageDestroy(im);

	return EXIT_SUCCESS;
}

int modStorPNGArr(char *filename, char *post, int seq, float *img, coord_t res) {
//	int i;
	char date[64];
	struct tm *loctime;
	time_t curtime;
	
	curtime = time(NULL);
	loctime = localtime(&curtime);
	strftime (date, 64, "%Y%m%d_%H%M%S", loctime);

	
//	for (i = 0; i < ptc.wfs_count; i++) {
	snprintf(filename, COMMANDLEN, "foam_capture-%s_%05d-%s.png", date, seq, post);
	logDebug(0, "Storing capture to %s", filename);
	modWritePNGArr(filename, img, res, 0);
//	}
	
	return EXIT_SUCCESS;
		
}

int modStorPNGSurf(char *filename, char *post, int seq, SDL_Surface *img) {
	char date[64];
	struct tm *loctime;
	time_t curtime;
	
	curtime = time(NULL);
	loctime = localtime(&curtime);
	strftime (date, 64, "%Y%m%d_%H%M%S", loctime);

	snprintf(filename, COMMANDLEN, "foam_capture-%s_%05d-%s.png", date, seq, post);
	logDebug(0, "Storing capture to %s", filename);
	modWritePNGSurf(filename, img);
	
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
