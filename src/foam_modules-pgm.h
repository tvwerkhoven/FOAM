/*! 
	@file foam_modules-pgm.h
	@author @authortim
	@date 2008-02-25

	@brief This header file prototypes the pgm functions.
*/
#ifndef FOAM_MODULES_IMG
#define FOAM_MODULES_IMG

#include "foam_cs_library.h"		// we link to the main program here (i.e. we use common (log) functions)
#include "SDL_image.h"				// we need this to read image files
#include "gd.h"						// we need this to write png/jpg files

/*!
@brief Reads a pgm file from disk into an SDL_Surface

@param [in] *fname the filename to read
@param [out] **img the SDL_Surface that will hold the image
*/
// int modReadPGM(char *fname, SDL_Surface **img);

/*!
@brief Reads a pgm file from disk into an array

@param [in] *fname the filename to read
@param [out] **img the array that will hold the image (does not have to be allocated yet)
@param [out] outres this will hold the resolution of img
*/
int modReadPGMArr(char *fname, float **img, int outres[2]);

/*!
@brief Writes a 8-bit ASCII PGM file from memory to disk

@param [in] *fname the filename to write
@param [out] *img the SDL_Surface that will be written to disk
*/
int modWritePGM(char *fname, SDL_Surface *img);

/*!
 @brief Writes a 8-bit PNG file from memory to disk
 
 @param [in] *fname the filename to write
 @param [out] *img the SDL_Surface that will be written to disk
*/
int modWritePNG(char *fname, SDL_Surface *img);

/*!
@brief Get the value of a specific pixel off a SDL_Surface

@param [in] *surface SDL_Surface to read from
@param [in] x x-coordinate to read
@param [in] y y-coordinate to read
*/
Uint32 getPixel(SDL_Surface *surface, int x, int y);

#endif /* FOAM_MODULES_IMG */
