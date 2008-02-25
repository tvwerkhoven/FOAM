/*! 
	@file foam_modules-pgm.h
	@author @authortim
	@date 2008-02-25

	@brief This header file prototypes the pgm functions.
*/
#ifndef FOAM_MODULES_PGM
#define FOAM_MODULES_PGM

#include "foam_cs_library.h"		// we link to the main program here (i.e. we use common (log) functions)
#include "SDL_image.h"				// we need this to read PGM files


/*!
@brief Reads a pgm file from disk into memory

@param [in] *fname the filename to read
@param [out] **img the SDL_Surface that will hold the image
*/
int modReadPGM(char *fname, SDL_Surface **img);

/*!
@brief Writes a 8-bit ASCII PGM file from memory to disk

@param [in] *fname the filename to write
@param [out] *img the SDL_Surface that will be written to disk
*/
int modWritePGM(char *fname, SDL_Surface *img);

#endif /* FOAM_MODULES_PGM */
