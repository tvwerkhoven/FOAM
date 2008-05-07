/*! 
 @file foam_modules-img.h
 @author @authortim
 @date 2008-02-25
 
 @brief This header file prototypes the image functions.
 */
#ifndef FOAM_MODULES_IMG
#define FOAM_MODULES_IMG

#include "foam_cs_library.h"		// we link to the main program here (i.e. we use common (log) functions)
#include "SDL_image.h"				// we need this to read image files
#include <gd.h>						// we need this to write png/jpg files

/*!
 @brief Reads a pgm file from disk into an SDL_Surface
 
 @param [in] *fname the filename to read
 @param [out] **img the SDL_Surface that will hold the image
 */
// int modReadPGM(char *fname, SDL_Surface **img);

/*!
 @brief Reads a image file from disk into an array
 
 @param [in] *fname the filename to read
 @param [out] **img the array that will hold the image (does not have to be allocated yet)
 @param [out] *outres this will hold the resolution of img
 */
int modReadIMGArr(char *fname, float **img, coord_t *outres);

/*!
 @brief Writes a 8-bit ASCII PGM file from memory to disk
 
 @param [in] *fname the filename to write
 @param [in] *img the SDL_Surface that will be written to disk
 */
int modWritePGM(char *fname, SDL_Surface *img);

/*!
 @brief Writes a PNG file from an SDL_Surface to disk
 
 @param [in] *fname the filename to write
 @param [in] *img the SDL_Surface that will be written to disk
 @param [in] maxval the maximum value to scale the image with (0--65536), 0 for no scaling
 @param [in] pgmtype 0 for ascii pgm file, or 1 for binary pgm file
 */
int modWritePGMSurf(char *fname, SDL_Surface *img, int maxval, int pgmtype);

/*!
 @brief Writes a 8-bit PNG file from an float array to disk
 
 @param [in] *fname the filename to write
 @param [in] *imgc the float array that holds the image
 @param [in] res the resolution of img
 @param [in] datatype the datatype of *img, 0 for float, 1 for unsigned char
 */
int modWritePNGArr(char *fname, void *imgc, coord_t res, int datatype);

/*!
 @brief Writes a 8-bit PNG file for an float image to disk
 
 This routine writes out the image stored in an array to disk, naming the files like:\n
 foam_capture-YYYYMMDD_HHMMSS_CCCCC_WFSx.png\n
 with:
 \li YYYYMMDD the date 
 \li HHMMSS the time
 \li CCCCC the sequence number given to the function
 
 @param [out] *filename The base filename that was used to write to disk.
 @param [in] *post String to append to the filename (but before .png)
 @param [in] seq Sequence number to add to the filename 
 @param [in] *img Float array which holds the image
 @param [in] res Resolution of the image
 */
int modStorPNGArr(char *filename, char *post, int seq, float *img, coord_t res);

/*!
 @brief Writes a 8-bit PNG file for an SDL surface to disk
 
 See modStorPNGArr() for details.
 
 @param [out] *filename The base filename that was used to write to disk.
 @param [in] *post String to append to the filename (but before .png)
 @param [in] seq Sequence number to add to the filename 
 @param [in] *img SDL_Surface which holds the image
 */
int modStorPNGSurf(char *filename, char *post, int seq, SDL_Surface *img);

/*!
 @brief Get the value of a specific pixel off a SDL_Surface
 
 @param [in] *surface SDL_Surface to read from
 @param [in] x x-coordinate to read
 @param [in] y y-coordinate to read
 */
Uint32 getPixel(SDL_Surface *surface, int x, int y);

#endif /* FOAM_MODULES_IMG */
