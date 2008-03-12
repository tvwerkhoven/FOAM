/*! 
	@file foam_modules-display.c
	@author @authortim
	@date 2008-02-06 

	@brief This file contains some functions to display stuff.
	
	\section Info
	This module can be used to display images, lines, rectangles etc. To draw everything in one go,
	use modDrawStuff(). Otherwise, the seperate functions can be used. Make sure though, that before
	drawing anything (except for modDrawStuff()), modBeginDraw() is called, and that drawing is finalized
	with modFinishDraw().
	
	\section Functions
	
	The functions provided to the outside world are:
	\li modBeginDraw() - Begin drawing/displaying data on a surface
	\li modFinishDraw() - Finish drawing/displaying data on a surface and display
	\li modDisplayImg() - Displays an image to an SDL_Surface
	\li modDrawGrid() - Displays a square grid showing the lenslet array
	\li modDrawSubapts() - Displays the current location of the subaperture tracking windows
	\li modDrawVecs() - Draw vectors between the center of the Grid and the center of the tracker window
	\li modDrawStuff() - Draw all of the above.

	\section Dependencies
	
	This module does not depend on other modules.

	\section License
	
	This code is licensed under the GPL, version 2.
*/

// HEADERS //
/***********/

#include "foam_modules-display.h"

// ROUTINES //
/************/

void drawRect(int coord[2], int size[2], SDL_Surface *screen) {
	// lower line
	drawLine(coord[0], coord[1], coord[0] + size[0] + 1, coord[1], screen);
	// top line
	drawLine(coord[0], coord[1] + size[1], coord[0] + size[0], coord[1] + size[1], screen);
	// left line
	drawLine(coord[0], coord[1], coord[0], coord[1] + size[1], screen);
	// right line
	drawLine(coord[0] + size[0], coord[1], coord[0] + size[0], coord[1] + size[1], screen);
	// done

}

void drawLine(int x0, int y0, int x1, int y1, SDL_Surface *screen) {
	int step = abs(x1-x0);
	int i;
	if (abs(y1-y0) > step) step = abs(y1-y0); // this can be done faster?

	float dx = (x1-x0)/(float) step;
	float dy = (y1-y0)/(float) step;

	drawPixel(screen, x0, y0, 255, 255, 255);
	for(i=0; i<step; i++) {
		x1 = x0+i*dx; // since x1 is an integer, we can't just increment this, steps of 0.7 pixels wouldn't work...
		y1 = y0+i*dy;
		drawPixel(screen, x1, y1, 255, 255, 255); // draw directly to the screen in white
	}
}

void drawDash(int x0, int y0, int x1, int y1, SDL_Surface *screen) {
	int step = abs(x1-x0);
	int i;
	if (abs(y1-y0) > step) step = abs(y1-y0); // this can be done faster?

	float dx = (x1-x0)/(float) step;
	float dy = (y1-y0)/(float) step;

	drawPixel(screen, x0, y0, 255, 255, 255);
	for(i=0; i<step; i++) {
		if ((i / 10) % 2 == 1)
			continue;
			
		x1 = x0+i*dx; // since x1 is an integer, we can't just increment this, steps of 0.7 pixels wouldn't work...
		y1 = y0+i*dy;
		drawPixel(screen, x1, y1, 255, 255, 255); // draw directly to the screen in white
	}
}

void drawDeltaLine(int x0, int y0, int dx, int dy, SDL_Surface *screen) {
	drawLine(x0, y0, x0+dx, y0+dy, screen);
}

int modDisplayImg(float *img, coord_t res, SDL_Surface *screen) {
	// ONLY does float images as input
	int x, y, i;
	float max=img[0];
	float min=img[0];
	
	// we need this loop to check the maximum and minimum intensity. TODO: Do we need that? can't SDL do that?	
	for (x=0; x < res.x*res.y; x++) {
		if (img[x] > max)
			max = img[x];
		if (img[x] < min)
			min = img[x];
	}
	
	logDebug("Displaying image, min: %f, max: %f.", min, max);
							
	Uint32 color;
	// trust me, i'm not proud of this code either ;) TODO
	switch (screen->format->BytesPerPixel) {
		case 1: { // Assuming 8-bpp
				Uint8 *bufp;

				for (x=0; x<res.x; x++) {
					for (y=0; y<res.y; y++) {
						i = (int) ((img[y*res.x + x]-min)/(max-min)*255);
						color = SDL_MapRGB(screen->format, i, i, i);
						bufp = (Uint8 *)screen->pixels + y*screen->pitch + x;
						*bufp = color;
					}
				}
			}
			break;
		case 2: {// Probably 15-bpp or 16-bpp
				Uint16 *bufp;

				for (x=0; x<res.x; x++) {
					for (y=0; y<res.y; y++) {
						i = (int) ((img[y*res.x + x]-min)/(max-min)*255);
						color = SDL_MapRGB(screen->format, i, i, i);
						bufp = (Uint16 *)screen->pixels + y*screen->pitch/2 + x;
						*bufp = color;
					}
				}
			}
			break;
		case 3: { // Slow 24-bpp mode, usually not used
				Uint8 *bufp;
				
				for (x=0; x<res.x; x++) {
					for (y=0; y<res.y; y++) {
						i = (int) ((img[y*res.x + x]-min)/(max-min)*255);
						color = SDL_MapRGB(screen->format, i, i, i);
						bufp = (Uint8 *)screen->pixels + y*screen->pitch + x * 3;
						if(SDL_BYTEORDER == SDL_LIL_ENDIAN) {
							bufp[0] = color;
							bufp[1] = color >> 8;
							bufp[2] = color >> 16;
						} else {
							bufp[2] = color;
							bufp[1] = color >> 8;
							bufp[0] = color >> 16;
						}
					}
				}
			}
			break;
		case 4: { // Probably 32-bpp
				Uint32 *bufp;

				// draw the image itself
				for (x=0; x<res.x; x++) {
					for (y=0; y<res.y; y++) {
						i = (int) ((img[y*res.x + x]-min)/(max-min)*255);
						color = SDL_MapRGB(screen->format, i, i, i);
						bufp = (Uint32 *)screen->pixels + y*screen->pitch/4 + x;
						*bufp = color;
					}
				}
			}
			break;
	}
	
	return EXIT_SUCCESS;
}

void drawPixel(SDL_Surface *screen, int x, int y, Uint8 R, Uint8 G, Uint8 B) {
	Uint32 color = SDL_MapRGB(screen->format, R, G, B);
	if (x<0) x=0;
	else if (x>screen->w) x=screen->w;
	
	if (y<0) y=0;
	else if (y>screen->h) y=screen->h;
		
	switch (screen->format->BytesPerPixel) {
		case 1: // Assuming 8-bpp
			{
				Uint8 *bufp;
				bufp = (Uint8 *)screen->pixels + y*screen->pitch + x;
				*bufp = color;
			}
			break;
		case 2: // Probably 15-bpp or 16-bpp
			{
				Uint16 *bufp;
				bufp = (Uint16 *)screen->pixels + y*screen->pitch/2 + x;
				*bufp = color;
			}
			break;
		case 3: // Slow 24-bpp mode, usually not used
			{
				Uint8 *bufp;
				bufp = (Uint8 *)screen->pixels + y*screen->pitch + x * 3;
				if(SDL_BYTEORDER == SDL_LIL_ENDIAN)
				{
					bufp[0] = color;
					bufp[1] = color >> 8;
					bufp[2] = color >> 16;
				} else {
					bufp[2] = color;
					bufp[1] = color >> 8;
					bufp[0] = color >> 16;
				}
			}
			break;
		case 4: { // Probably 32-bpp
				Uint32 *bufp;
				bufp = (Uint32 *)screen->pixels + y*screen->pitch/4 + x;
				*bufp = color;
			}
			break;
	}
}

int modDrawSubapts(wfs_t *wfsinfo, SDL_Surface *screen) {
	if (wfsinfo->nsubap == 0)
		return EXIT_SUCCESS;	// if there's nothing to draw, don't draw (shouldn't happen)
		
	int *shsize;			// size of whole image, nr of cells. will hold an int[2] array
	int (*subc)[2] = wfsinfo->subc;		// lower-left coordinates of the subapts

	shsize = wfsinfo->shsize;
	int subsize[2] = {wfsinfo->shsize[0]/2, wfsinfo->shsize[1]/2};
		
	int sn=0;
		
	// we draw the reference subaperture rectangle bigger than the rest, with lower left coord:
	int refcoord[] = {subc[0][0]-shsize[0]/4, subc[0][1]-shsize[1]/4};
	drawRect(refcoord, shsize, screen);
	
	for (sn=1; sn< wfsinfo->nsubap; sn++) {
		// subapt with lower coordinates (subc[sn][0],subc[sn][1])
		// first subapt has size (shsize[0],shsize[1]),
		// the rest are (shsize[0]/2,shsize[1]/2)
		drawRect(subc[sn], subsize, screen);
	}
	
	return EXIT_SUCCESS;
}

int modDrawVecs(wfs_t *wfsinfo, SDL_Surface *screen) {
	if (wfsinfo->nsubap == 0)
		return EXIT_SUCCESS;	// if there's nothing to draw, don't draw (shouldn't happen)
	
	int (*gridc)[2] = wfsinfo->gridc;	// lower-left coordinates of the grid in which the subapt resides
	int *shsize = wfsinfo->shsize;
	int sn=0;
	

	for (sn=0; sn<wfsinfo->nsubap; sn++)
		drawDeltaLine( gridc[sn][0] + (shsize[0]/2), gridc[sn][1] + (shsize[1]/2), \
			(int) gsl_vector_float_get(wfsinfo->disp, sn*2+0), (int) gsl_vector_float_get(wfsinfo->disp, sn*2+1), \
			screen);
	
	return EXIT_SUCCESS;
}

int modDrawGrid(wfs_t *wfsinfo, SDL_Surface *screen) {
	if (wfsinfo->nsubap == 0)
		return EXIT_SUCCESS;			// if there's nothing to draw, don't draw (shouldn't happen)
		
	int *shsize = wfsinfo->shsize; 		// size of the grid
	int *cells = wfsinfo->cells; 		// number of cells
	int xc, yc;
		
			
	for (xc=1; xc < cells[0]; xc++)
		drawDash(xc*shsize[0], 0, xc*shsize[0], cells[1]*shsize[1], screen);
	
	for (yc=1; yc< cells[1]; yc++)
		drawDash(0, yc*shsize[1], cells[0]*shsize[0], yc*shsize[1], screen);
			
	return EXIT_SUCCESS;
}

void modDrawStuff(control_t *ptc, int wfs, SDL_Surface *screen) {
	modBeginDraw(screen);
	modDisplayImg(ptc->wfs[wfs].image, ptc->wfs[wfs].res, screen);
	modDrawGrid(&(ptc->wfs[wfs]), screen);
	modDrawSubapts(&(ptc->wfs[0]), screen);
	modDrawVecs(&(ptc->wfs[0]), screen);
	modFinishDraw(screen);
}

void modBeginDraw(SDL_Surface *screen) {
	if ( SDL_MUSTLOCK(screen) )	{
		if ( SDL_LockSurface(screen) < 0 )
			return;
	}
}

void modFinishDraw(SDL_Surface *screen) {
	if ( SDL_MUSTLOCK(screen) )
		SDL_UnlockSurface(screen);
	
	SDL_Flip(screen);
}