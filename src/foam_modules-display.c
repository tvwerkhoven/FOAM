/*! 
	@file foam_modules-display.c
	@author @authortim
	@date 2008-02-06 

	@brief This file contains some functions to display stuff.
*/

// HEADERS //
/***********/

#include "foam_modules-display.h"

SDL_Surface *screen;	// Global surface to draw on
SDL_Event event;		// Global SDL event struct to catch user IO

void drawRect(int coord[2], int size[2], SDL_Surface *screen) {

	// lower line
	drawLine(coord[0], coord[1], coord[0] + size[0], coord[1], screen);
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

	DrawPixel(screen, x0, y0, 255, 255, 255);
	for(i=0; i<step; i++) {
		x0 = round(x0+dx); // round because integer casting would floor, resulting in an ugly line (?)
		y0 = round(y0+dy);
		DrawPixel(screen, x0, y0, 255, 255, 255); // draw directly to the screen in white
	}
}


int displayImg(float *img, int res[2], SDL_Surface *screen) {
	// ONLY does float images
	int x, y, i;
	float max=img[0];
	float min=img[0];
	
	
	// we need this loop to check the maximum and minimum intensity. Do we need that? can't SDL do that?	
	for (x=0; x < res[0]*res[1]; x++) {
		if (img[x] > max)
			max = img[x];
		if (img[x] < min)
			min = img[x];
	}
	
	logDebug("Displaying image, min: %f, max: %f.", min, max);
	Slock(screen);
	for (x=0; x<res[0]; x++) {
		for (y=0; y<res[1]; y++) {
			i = (int) ((img[y*res[0] + x]-min)/(max-min)*255);
			DrawPixel(screen, x, y, \
				i, \
				i, \
				i);
		}
	}
	Sulock(screen);
	SDL_Flip(screen);
	
	return EXIT_SUCCESS;
}

void Slock(SDL_Surface *screen) {
	if ( SDL_MUSTLOCK(screen) )	{
		if ( SDL_LockSurface(screen) < 0 )
			return;
	}
}

void Sulock(SDL_Surface *screen) {
	if ( SDL_MUSTLOCK(screen) )
		SDL_UnlockSurface(screen);
}

void DrawPixel(SDL_Surface *screen, int x, int y, Uint8 R, Uint8 G, Uint8 B) {
	Uint32 color = SDL_MapRGB(screen->format, R, G, B);
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
	Slock(screen);
		
	// we draw the reference subaperture rectangle bigger than the rest, with lower left coord:
	int refcoord[] = {subc[0][0]-shsize[0]/4, subc[0][1]-shsize[1]/4};
	drawRect(refcoord, shsize, screen);
	
	for (sn=1; sn< wfsinfo->nsubap; sn++) {
		// subapt with lower coordinates (subc[sn][0],subc[sn][1])
		// first subapt has size (shsize[0],shsize[1]),
		// the rest are (shsize[0]/2,shsize[1]/2)
		drawRect(subc[sn], subsize, screen);
	}
	
	Sulock(screen);
	SDL_Flip(screen);	
	return EXIT_SUCCESS;
}

