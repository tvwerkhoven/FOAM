/*! 
@file foam_modules-display.h
@brief This is the headerfile for graphics routines
@author @authortim
*/

#ifndef FOAM_MODULES_DISPLAY
#define FOAM_MODULES_DISPLAY

#include "foam_cs_library.h"
#include "SDL.h" 	// most portable way according to 
					//http://www.libsdl.org/cgi/docwiki.cgi/FAQ_20Including_20SDL_20Headers



/*!
@brief This draws a rectangle starting at {coord[0], coord[1]} with size {size[0], size[1]} on screen *screen

@param [in] coord Lower left coordinate of the rectangle to be drawn
@param [in] size Size of the rectangle to be drawn 
@param [in] *screen SDL_Surface to draw on
*/
void drawRect(int coord[2], int size[2], SDL_Surface*screen);

/*!
@brief This draws a line from {x0, y0} to {x1, y1} without any aliasing

@param [in] x0 starting x-coordinate
@param [in] y0 starting y-coordinate
@param [in] x1 end x-coordinate
@param [in] y1 end y-coordinate
@param [in] *screen SDL_Surface to draw on
*/
void drawLine(int x0, int y0, int x1, int y1, SDL_Surface*screen);

/*!
@brief This draws all subapertures for a certain wfs

@param [in] *wfsinfo wfs_t struct with info on the current wfs
@param [in] *screen SDL_Surface to draw on
*/
int modDrawSubapts(wfs_t *wfsinfo, SDL_Surface *screen);

/*!
@brief This draws vectors from the center of the grid to the detected center of gravity

@param [in] *wfsinfo wfs_t struct with info on the current wfs
@param [in] *screen SDL_Surface to draw on
*/
int modDrawVecs(wfs_t *wfsinfo, SDL_Surface *screen);

/*!
@brief This draws the subaperture grid

@param [in] *wfsinfo wfs_t struct with info on the current wfs
@param [in] *screen SDL_Surface to draw on
*/
int modDrawGrid(wfs_t *wfsinfo, SDL_Surface *screen);

/*!
@brief This displays an image img with resolution res

@param [in] *img pointer to the image
@param [in] res resolution of the image
@param [in] *screen SDL_Surface to draw on
*/
int displayImg(float *img, coord_t res, SDL_Surface *screen);
void DrawPixel(SDL_Surface *screen, int x, int y, Uint8 R, Uint8 G, Uint8 B);
void Sulock(SDL_Surface *screen);
void Slock(SDL_Surface *screen);

void drawStuff(control_t *ptc, int wfs, SDL_Surface *screen);

Uint32 getpixel(SDL_Surface *surface, int x, int y);

#endif /* FOAM_MODULES_DISPLAY */
