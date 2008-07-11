/*! 
 @file foam_modules-img.h
 @author @authortim
 @date 2008-02-25
 
 @brief This header file prototypes the image functions.
 */
#ifndef FOAM_MODULES_IMG
#define FOAM_MODULES_IMG

#ifdef FOAM_MODIMG_ALONE
#define FOAM_DEBUG 1
#endif

#include "foam_cs_library.h"		// we link to the main program here (i.e. we use common (log) functions)
#include "SDL_image.h"				// we need this to read image files
#include <gd.h>						// we need this to write png/jpg files

/*!
 @brief Reads image files from disk into an SDL_Surface

 This routine uses the SDL_Image library which provides the routine
 IMG_Load which can read a variety of image formats from disk into
 memory. For more information, I refer to the SDL_Image documentation
 at http://www.libsdl.org/projects/docs/SDL_image/
 
 @param [in] *fname the filename to read
 @param [out] **surf the SDL_Surface that will hold the image
 */
int modReadIMGSurf(char *fname, SDL_Surface **surf);

/*!
 @brief Reads a image file from disk into a uint8_t array
 
 This routine internally uses modReadIMGSurf() and copies the data from an SDL_Surface to
 the byte array.
 
 @param [in] *fname the filename to read
 @param [out] **img the array that will hold the image (does not have to be allocated yet)
 @param [out] *outres this will hold the resolution of img
 */
int modReadIMGArrByte(char *fname, uint8_t **img, coord_t *outres);

/*!
 @brief Writes an 8-bit ASCII or binary PGM file from memory to disk
 
 @param [in] *fname the filename to write
 @param [in] *img the image that will be written to disk
 @param [in] datatype the datatype of the image
 @param [in] res the image resolution
 @param [in] maxval the maximum value to scale the image with (0--65536), 0 for no scaling
 @param [in] pgmtype 0 for ascii pgm file, or 1 for binary pgm file 
 */
int modWritePGMArr(char *fname, void *img, foam_datat_t datatype, coord_t res, int maxval, int pgmtype);

/*!
 @brief Writes an 8-bit ASCII or binary PGM file from an SDL_Surface to disk
 
 @param [in] *fname the filename to write
 @param [in] *img the SDL_Surface that will be written to disk
 @param [in] maxval the maximum value to scale the image with (0--65536), 0 for no scaling
 @param [in] pgmtype 0 for ascii pgm file, or 1 for binary pgm file
 */
int modWritePGMSurf(char *fname, SDL_Surface *img, int maxval, int pgmtype);

/*!
 @brief Calculate some statistics on an image, and return this in stats.
 
 This routine accepts DATA_UINT8, DATA_UINT16 and DATA_GSL_M_F datatypes. Besides
 accepting these dataformats, it can either work with a (rectangular) resolution,
 or with the total number of pixels in the image. The last is only applicable to the
 first two datatypes and can be used if the data is stored per subaperture instead 
 of a rectangular image, i.e. if the image contains data on 7 subapertures, each
 8*8 pixels big, the first 8*8=64 pixels are subapt 1, the second 64 pixels are subapt
 2, etc. 
 
 If size is used, pixels must be -1. If pixels is used, size must be NULL.
 
 @param [in] *img The image to analyse
 @param [in] data The datatype the image is stored in
 @param [in] *size The resolution of the image, or NULL if pixels is used used
 @param [in] pixels The number  of pixels in the image, or -1 if *size is used
 @param [out] *stats Will hold an array of {min, max, mean}
*/
void imgGetStats(void *img, foam_datat_t data, coord_t *size, int pixels, float *stats);

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
