/*! 
	@file foam_modules-img.c
	@author @authortim
	@date 2008-02-25 

	@brief This file contains functions to read and write image files.
	
	\section Info
	This module can be used to read and write SDL_Surfaces from and to files.
	
	\section Functions
	
	The functions provided to the outside world are:
	\li modReadIMGSurf() - Read an img (png, pgm, jpg, etc.) to an SDL_Surface.
	\li modReadIMGArr() - Read an img (png, pgm, jpg, etc.) to an array.
	\li modWritePGMSurf() - Write an 8-bit ASCII PGM file from an SDL_Surface.
	\li modWritePNGArr() - Write an 8-bit ASCII PGM file from a float image.
	\li modWritePNGSurf() - Write an 8-bit grayscale PNG file from an SDL_Surface.

	\section Dependencies
	
	This module does not depend on other modules.
	
	This module requires the following libraries:
	\li SDL
	\li SDL_Image
	\li gd
	\li libpng
 
	\section History
 
	\li 2008-04-14 updated documentation

	\section License
	This code is licensed under the GPL, version 2 or higher
*/

// HEADERS //
/***********/

#include "foam_modules-img.h"

//void logErr(const char *msg, ...) {
//	va_list ap;
//	va_start(ap, msg);
//	vfprintf(stderr, msg, ap);
//	fprintf(stderr, "\n");
//}
//void logWarn(const char *msg, ...) {
//	va_list ap;
//	va_start(ap, msg);
//	vfprintf(stderr, msg, ap);
//	fprintf(stderr, "\n");
//}
//void logInfo(int flg, const char *msg, ...) {
//	va_list ap;
//	va_start(ap, msg);
//	vfprintf(stderr, msg, ap);
//	fprintf(stderr, "\n");
//}
//void logDebug(int flg, const char *msg, ...) {
//	va_list ap;
//	va_start(ap, msg);
//	vfprintf(stderr, msg, ap);
//	fprintf(stderr, "\n");
//}


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

int modReadIMGArr(char *fname, float **img, coord_t *outres) {
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
	*img = malloc(sdlimg->w * sdlimg->h * sizeof(float));
	if (*img == NULL)
		logErr("Failed to allocate memory in modReadIMGArr().");
		
	outres->x = sdlimg->w;
	outres->y = sdlimg->h;
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

int modWritePGMSurf(char *fname, SDL_Surface *img, int maxval, int pgmtype) {
	FILE *fd;
	int x, y;
	int linew, chars;
	int val;
	float max, min, pix;
	min = max = (float) getPixel(img, 0, 0);
	
	fd = fopen(fname,"wb+");
	if (!fd) {
		logWarn("Error, cannot open file %s: %s", fname, strerror(errno));
		return EXIT_FAILURE;
	}
	
	if (maxval > 65535 || maxval < 0) {
		logWarn("Cannot write PGM file with maximum value %d (must be 0--65536)", maxval);
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
	
	// chars is the maxixmum width of one pixel plus a space in ascii
	// the maximum value determines how wide the pixel will be (how many
	// digits). we add 1 to account for the space
	if (maxval == 0)
		chars = 1 + (int) ceilf(log10f((float) max));
	else
		chars = 1 + (int) ceilf(log10f((float) maxval));
	
	// HEADER //
	////////////
	if (pgmtype == 0) // We're making ascii
		fprintf(fd, "P2\n");
	else // We're making binary
		fprintf(fd, "P5\n");

	fprintf(fd, "%d %d\n", img->w, img->h);
	fprintf(fd, "%d\n", maxval);
	
	// WRITE IMAGE //
	/////////////////
	for (y=0; y<img->h; y++) {
		linew = 0;
		for (x=0; x<img->w; x++) {
			if (maxval == 0) // if maxval == 0, we do not scale the values
				val = (int) getPixel(img, x, y);
			else
				val = (int) (maxval * (getPixel(img, x, y)-min) / (max-min));
				
			if (pgmtype == 0) { // ASCII
				fprintf(fd, "%d ", val);
				linew += chars;
				// ASCII PGM lines cannot be longer than 70 characters :P
				if (linew + chars > 70) fprintf(fd, "\n");
			}
			else { // binary
				if (maxval > 255) // 2 byte pixels
					fwrite(&val, 2, 1, fd);
				else // 1 byte pixels
					fwrite(&val, 1, 1, fd);
			
			}
		}
		if (pgmtype == 0) fprintf(fd, "\n");
	}
	
	fclose(fd);
	return EXIT_SUCCESS;
}

int modWritePGMArr(char *fname, void *img, int datatype, coord_t res, int maxval, int pgmtype) {
	FILE *fd;
	int x, y;
	int linew, chars;
	int val;
	float max, min, pix;

	fd = fopen(fname,"wb+");
	if (!fd) {
		logWarn("Error, cannot open file %s: %s", fname, strerror(errno));
		return EXIT_FAILURE;
	}
	
	if (maxval > 65535 || maxval < 1) {
		logWarn("Cannot write PGM file with maximum value %d (must be 0--65536)", maxval);
		return EXIT_FAILURE;
	}

	// Find maximum minimum //
	//////////////////////////
	if (datatype == 0) { // float!
		float *imgc = (float*) img;
		max = min = (float) imgc[0];
		// check maximum & min
		for (x=0; x< res.x * res.y; x++) {
			pix = (float) imgc[x];
			if (pix > max) max = pix;
			else if (pix < min) min = pix;
		}
	}
	else if (datatype == 1) { // char!
 		char *imgc = (char*) img;
		max = min = (float) imgc[0];
		// check maximum & min
		for (x=0; x< res.x * res.y; x++) {
			pix = (float) imgc[x];
			if (pix > max) max = pix;
			else if (pix < min) min = pix;
		}
	}
	else
		return EXIT_FAILURE;
	
	// chars is the maxixmum width of one pixel plus a space in ascii
	// the maximum value determines how wide the pixel will be (how many
	// digits). we add 1 to account for the space
	if (maxval == 0)
		chars = 1 + (int) ceilf(log10f((float) max));
	else
		chars = 1 + (int) ceilf(log10f((float) maxval));
	
	// Write header //
	//////////////////
	if (pgmtype == 0) // We're making ascii
		fprintf(fd, "P2\n");
	else // We're making binary
		fprintf(fd, "P5\n");
	
	fprintf(fd, "%d %d\n", res.x, res.y);
	fprintf(fd, "%d\n", maxval);

	// Write image body //
	//////////////////////
	if (datatype == 0) { // float!
		float *imgc = (float *) img;
		for (x=0; x < res.x * res.y; x++) {
			linew = 0;
			val = (int) (maxval * (imgc[x]-min) / (max-min));
			
			if (pgmtype == 0) { // ASCII
				fprintf(fd, "%d ", val);
				linew += chars;
				// ASCII PGM lines cannot be longer than 70 characters :P
				if (linew + chars > 70) fprintf(fd, "\n");
				// print a newline every row of the image
				if (x % res.x == 0) fprintf(fd, "\n");
			}
			else { // binary
				if (maxval > 255) // 2 byte pixels
					fwrite(&val, 2, 1, fd);
				else // 1 byte pixels
					fwrite(&val, 1, 1, fd);
				
			} // end ascii / binary if-else
		} // end loop over all pixels
	}

	if (datatype == 0) { // char!
		char *imgc = (char *) img;
		for (x=0; x < res.x * res.y; x++) {
			linew = 0;
			chars = 1 + (int) ceilf(log10f((float) maxval));
			val = (int) (maxval * (imgc[x]-min) / (max-min));
			
			if (pgmtype == 0) {
				// ASCII
				fprintf(fd, "%d ", val);
				linew += chars;
				// ASCII PGM lines cannot be longer than 70 characters :P
				if (linew + chars > 70) fprintf(fd, "\n");
				// print a newline every row of the image
				if (x % res.x == 0) fprintf(fd, "\n");
			}
			else {
				// binary
				if (maxval > 255) // 2 byte
					fwrite(&val, 2, 1, fd);
				else // 1 byte
					fwrite(&val, 1, 1, fd);
			} // end ascii / binary if-else
		} // end loop over all pixels
	}	
	
	fclose(fd);
	return EXIT_SUCCESS;
}

int modWritePNGArr(char *fname, void *imgc, coord_t res, int datatype) {
	FILE *fd;
	float max, min, pix;
	int x, y, i;
	int gray[256];

	gdImagePtr im;
	
	// allocate image
	im = gdImageCreate(res.x, res.y);
	
	// generate grayscales
	for (i=0; i<256; i++)
		gray[i] = gdImageColorAllocate(im, i, i, i);

	// Begin branching depending on datatype //
	///////////////////////////////////////////
	if (datatype == 0) {
		//imgc = (float *) imgc;
		float *img = (float *) imgc;
		min = max = (float) img[0];
		
		// check maximum & min
		for (x=0; x< res.x*res.y; x++) {
			pix = (float) img[x];
			if (pix > max) max = pix;
			else if (pix < min) min = pix;
		}
		
		// set pixels in image
		for (y=0; y< res.y; y++) {
			for (x=0; x< res.x; x++) {
				pix = 255*((float) img[y*res.x + x]-min)/(max-min);
				gdImageSetPixel(im, x, y, gray[(int) pix]);
			}
		}		
	}
	else if (datatype == 1) {
		unsigned char *img = (unsigned char *) imgc;
		
		min = max = (float) img[0];
		// check maximum & min
		for (x=0; x< res.x*res.y; x++) {
			pix = (float) img[x];
			if (pix > max) max = pix;
			else if (pix < min) min = pix;
		}
		
		// set pixels in image
		for (y=0; y< res.y; y++) {
			for (x=0; x< res.x; x++) {
				pix = 255*((float) img[y*res.x + x]-min)/(max-min);
				gdImageSetPixel(im, x, y, gray[(int) pix]);
			}
		}		
	}
	else 
		return EXIT_FAILURE;
	
	///////////////////
	// End Branching //
	
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
    // Here p is the address to the pixel we want to retrieve
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
			return 0;       // shouldn't happen, but avoids warnings
    }
}

#ifdef FOAM_MODIMG_ALONE
int main(int argc, char *argv[]) {
	if (argc < 2) {
		printf("Please call me as: <script> <image file>\n");
		return -1;
	}
	
	printf("Testing img module...\n");
	
	printf("Trying to read image '%s' to SDL_Surface\n", argv[0]);
	SDL_Surface *image;
	modReadIMGSurf(argv[0], &image);
	printf("Trying to write image just read to 8 bit binary PGM file 'modimg-test1-8bin.pgm'\n");
	modWritePGMSurf("modimg-test2-8bin.pgm", image, 255, 1);
	printf("Trying to write image just read to 16 bit binary PGM file 'modimg-test1-16bin.pgm'\n");
	modWritePGMSurf("modimg-test2-16bin.pgm", image, 65535, 1);

	printf("Trying to write image just read to 8 bit ascii PGM file 'modimg-test1-8ascii.pgm'\n");
	modWritePGMSurf("modimg-test2-8ascii.pgm", image, 255, 0);
	printf("Trying to write image just read to 16 bit ascii PGM file 'modimg-test1-16ascii.pgm'\n");
	modWritePGMSurf("modimg-test2-16ascii.pgm", image, 65535, 0);

	printf("Trying to read image '%s' to array\n", argv[0]);
	float *img;
	coord_t res;
	modReadIMGArr(argv[0], &img, &res);

	printf("Trying to write image just read to 8 bit binary PGM file 'modimg-test2-8bin.pgm'\n");
	modWritePGMArr("modimg-test2-8bin.pgm", img, 0, res, 255, 1);
	printf("Trying to write image just read to 16 bit binary PGM file 'modimg-test2-16bin.pgm'\n");
	modWritePGMArr("modimg-test2-16bin.pgm", img, 0, res, 65535, 1);
	
	printf("Trying to write image just read to 8 bit ascii PGM file 'modimg-test2-8ascii.pgm'\n");
	modWritePGMArr("modimg-test2-8ascii.pgm", img, 0, res, 255, 0);
	printf("Trying to write image just read to 16 bit ascii PGM file 'modimg-test2-16ascii.pgm'\n");
	modWritePGMArr("modimg-test2-16ascii.pgm", img, 0, res, 65535, 0);
	
	printf("Testing complete! Check files in the current directory to see if everything worked\n");
	
	return 0;
}
#endif
