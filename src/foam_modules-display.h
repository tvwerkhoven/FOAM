/*! 
	@file foam_modules-display.h
	@author @authortim
	@date 2008-03-12

	@brief This is the headerfile for graphics routines	
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

Do not forget to call modBeginDraw()/modFinishDraw().

@param [in] *wfsinfo wfs_t struct with info on the current wfs
@param [in] *screen SDL_Surface to draw on
*/
int modDrawSubapts(wfs_t *wfsinfo, SDL_Surface *screen);

/*!
@brief This draws vectors from the center of the grid to the detected center of gravity

Do not forget to call modBeginDraw()/modFinishDraw().

@param [in] *wfsinfo wfs_t struct with info on the current wfs
@param [in] *screen SDL_Surface to draw on
*/
int modDrawVecs(wfs_t *wfsinfo, SDL_Surface *screen);

/*!
@brief This draws a grid on the screen

Do not forget to call modBeginDraw()/modFinishDraw().

@param [in] gridres The grid resolution to draw on the screen (i.e. 8x8)
@param [in] *screen SDL_Surface to draw on
*/
int modDrawGrid(int gridres[2], SDL_Surface *screen);

/*!
@brief This displays an image img with resolution res.

This displays an image stored in row-major format in *img on the screen
*screen. The resolution of the image to be drawn must also be specified, and
this must not be bigger than the resolution of the SDL_Surface. Do not forget
to call modBeginDraw()/modFinishDraw().

@param [in] *img pointer to the image
@param [in] res resolution of the image
@param [in] *screen SDL_Surface to draw on
*/
int modDisplayImg(float *img, coord_t res, SDL_Surface *screen);

/*!
@brief This draws one RGB pixel at a specific coordinate

@param [in] *screen SDL_Surface to draw on
@param [in] x x-coordinate to draw on
@param [in] y y-coordinate to draw on
@param [in] R red component of the colour
@param [in] G green component of the colour
@param [in] B blue component of the colour
*/
void drawPixel(SDL_Surface *screen, int x, int y, Uint8 R, Uint8 G, Uint8 B);

/*!
@brief Draw useful stuff on the screen

This routine draws the sensor output, the lenslet grid,
the tracker windows and the displacement vectors on the screen.
You do \e not need to call modBeginDraw()/modFinishDraw() when using
this routine.
*/
void modDrawStuff(control_t *ptc, int wfs, SDL_Surface *screen);

/*!
@brief Draw sensor output to screen

This routine only draws the sensor output to the screen.
You do \e not need to call modBeginDraw()/modFinishDraw() when using
this routine.
*/
void modDrawSens(control_t *ptc, int wfs, SDL_Surface *screen);
/*!
@brief Finish drawing (unlock the screen)
*/
void modFinishDraw(SDL_Surface *screen);

/*!
@brief Lock the screen, if necessary
*/
void modBeginDraw(SDL_Surface *screen);

#endif /* FOAM_MODULES_DISPLAY */
