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
 \li modInitDraw() - Initialize this module (setup SDL etc)
 \li modBeginDraw() - Begin drawing/displaying data on a surface
 \li modFinishDraw() - Finish drawing/displaying data on a surface and display
 \li modDisplayImg() - Displays an image to an SDL_Surface
 \li modDrawSens() - Display sensor output to the screen. (only ifdef FOAM_MODULES_DISLAY_SHSUPPORT)
 \li modDrawGrid() - Displays a square grid showing the lenslet array
 \li modDrawSubapts() - Displays the current location of the subaperture tracking windows
 \li modDrawVecs() - Draw vectors between the center of the Grid and the center of the tracker window (only ifdef FOAM_MODULES_DISLAY_SHSUPPORT)
 \li modDrawStuff() - Draw all of the above.
 
 \section Dependencies
 
 This module does not depend on other modules.
 
 \section License
 
 This code is licensed under the GPL, version 2.
 */

// HEADERS //
/***********/

#include "foam_modules-display.h"

// Include the OpenGL backend if available. 
#ifdef FOAM_OPENGL
#include "foam_modules-dispgl.h"
#else
#include "foam_modules-dispsdl.h"
#endif

// GLOBAL VARIABLES //
/********************/


// this will be used to temporarily store an image if displaying a GSL matrix
// since this needs conversion from the GSL datatype to a simple row-major
// matrix, we need a place to store it, and that's right here. Note that the
// data will only be allocated when necessary, and will only be as big the 
// wavefront sensor.
static uint8_t *tmpimg_b=NULL;

// ROUTINES //
/************/

public routines:
displayInit();
displayDraw();
-> should be able to draw:
 raw images (byte)
 dark imaes
 flat images
-> and toggle 
 grid (SH)
 subap (SH)
 vec (SH)
displayEvent();
displayFinish();

// in hardware routines: displayInit()
int modInitDraw(mod_display_t *disp);



// to SDL lib:
void drawRect(coord_t coord, coord_t size, SDL_Surface *screen) 
void drawLine(int x0, int y0, int x1, int y1, SDL_Surface *screen)
void drawDash(int x0, int y0, int x1, int y1, SDL_Surface *screen)
void drawDeltaLine(int x0, int y0, int dx, int dy, SDL_Surface *screen)
int modDisplayImgFloat(float *img, mod_display_t *disp)
int modDisplayGSLImg(gsl_matrix_float *gslimg, mod_display_t *disp, int doscale) {
	int i, j;
	float min, max;
	// we want to display a GSL image, copy it over the normal image 
	if (tmpimg_b == NULL)
		tmpimg_b = malloc(disp->res.x * disp->res.y);

	if (doscale == 1) {
		gsl_matrix_float_minmax(gslimg, &min, &max);
		gsl_matrix_float_add_constant(gslimg, -min);
		gsl_matrix_float_scale(gslimg, 255/(max-min));
	}

	for (i=0; i < disp->res.x; i++) {
		for (j=0; j < disp->res.y; j++) {
			tmpimg_b[i*disp->res.x + j] = (uint8_t) gsl_matrix_float_get(gslimg, i, j);
		}
	}
	return modDisplayImgByte(tmpimg_b, disp);
}

int modDisplayImg(wfs_t *wfsinfo, mod_display_t *disp) {
    // !!!:tim:20080507 this is probably not very clean (float is not 16 bpp)
    // databpp == 16 
    // databpp == 8 char (uint8_t)
	if (disp->dispsrc == DISPSRC_RAW) {
		if (wfsinfo->bpp == 8) {
			uint8_t *imgc = (uint8_t *) wfsinfo->image;
			modDisplayImgByte(imgc, disp);
		}
		else if (wfsinfo->bpp == 16) {
			float *imgc = (float *) wfsinfo->image;
			modDisplayImgFloat(imgc, disp);
		}
	}
	else if (disp->dispsrc == DISPSRC_DARK) {
		modDisplayGSLImg(wfsinfo->darkim, disp, 1);
	}
	else if (disp->dispsrc == DISPSRC_FLAT) {
		modDisplayGSLImg(wfsinfo->flatim, disp, 1);
	}
	else if (disp->dispsrc == DISPSRC_CALIB) {
		modDisplayGSLImg(wfsinfo->corrim, disp, 1);
	}
    
    return EXIT_FAILURE;
}


void drawPixel(SDL_Surface *screen, int x, int y, Uint8 R, Uint8 G, Uint8 B) {
	Uint32 color = SDL_MapRGB(screen->format, R, G, B);
	
	// crop the pixel coordinates if they go out of bound
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

#ifdef FOAM_MODULES_DISLAY_SHSUPPORT

// !!!:tim:20080414 shortcut for SH display routines
int modDrawSubapts(mod_sh_track_t *shtrack, SDL_Surface *screen) {
	if (shtrack->nsubap == 0)
		return EXIT_SUCCESS;			// if there's nothing to draw, don't draw (shouldn't happen)
    
    //	int *shsize;						// size of whole image, nr of cells. will hold an int[2] array
    //	int (*subc)[2] = shtrack->subc;		// lower-left coordinates of the subapts
    
    //	int shsize[2] = {shtrack->shsize.x, shtrack->shsize.y};
    //	int subsize[2] = {shtrack->track.x, shtrack->track.y};
    
	int sn=0;
    
	// we draw the reference subaperture rectangle bigger than the rest to indicate 
    // that this subapt is the reference subapt. Subapertures are shtrack->track
    // big, and have the lower left coordinate at subc[sn].x, subc[sn].y. We draw
    // the reference subapt shtrack->shsize big, so we need subc[sn] - (shsize - track)/2
	coord_t refcoord;
    refcoord.x = shtrack->subc[0].x - (shtrack->shsize.x - shtrack->track.x)/2;
    refcoord.y = shtrack->subc[0].y - (shtrack->shsize.y - shtrack->track.y)/2;
    
	drawRect(refcoord, shtrack->shsize, screen);
	//logDebug(0, "Drawing reference subapt now (%d,%d) -> (%d,%d)", refcoord.x, refcoord.y, shtrack->shsize.x, shtrack->shsize.y);
	
	for (sn=1; sn< shtrack->nsubap; sn++) {
		// subapt with lower coordinates (subc[sn][0],subc[sn][1])
		// first subapt has size (shsize[0],shsize[1]),
		// the rest are (subsize[0],subsize[1])
		//logDebug(LOG_NOFORMAT, "%d (%d,%d) ", sn, shtrack->subc[sn].x, shtrack->subc[sn].y);
		drawRect(shtrack->subc[sn], shtrack->track, screen);
	}
	//logDebug(LOG_NOFORMAT, "\n");
	
	return EXIT_SUCCESS;
}

// !!!:tim:20080414 shortcut for SH display routines
int modDrawVecs(mod_sh_track_t *shtrack, SDL_Surface *screen) {
	if (shtrack->nsubap == 0)
		return EXIT_SUCCESS;	// if there's nothing to draw, don't draw (shouldn't happen)
	
    //	int (*gridc)[2] = wfsinfo->gridc;	// lower-left coordinates of the grid in which the subapt resides
    //	int *shsize = wfsinfo->shsize;
	int sn=0;
	
    // To draw the displacement vector relative to the center of the lenslet grid,
    // we take gridc, which are lower left coordinates of the grid cells, and 
    // add half the grid size to get to the center, and then use the disp vector
    // to draw the vector itself.
	for (sn=0; sn < shtrack->nsubap; sn++)
		drawDeltaLine(shtrack->gridc[sn].x + (shtrack->shsize.x/2), \
                      shtrack->gridc[sn].y + (shtrack->shsize.y/2), \
                      (int) gsl_vector_float_get(shtrack->disp, sn*2+0), \
                      (int) gsl_vector_float_get(shtrack->disp, sn*2+1), \
                      screen);
	
	return EXIT_SUCCESS;
}

int modDrawGrid(coord_t gridres, SDL_Surface *screen) {
	int xc, yc;
	int gridw = screen->w/gridres.x;
	int gridh = screen->h/gridres.y;
    
	for (xc=1; xc < gridres.x; xc++)
		drawDash(xc*gridw, 0, xc*gridw, screen->h, screen);
	
	for (yc=1; yc< gridres.y; yc++)
		drawDash(0, yc*gridh, screen->w, yc*gridh, screen);
    
	return EXIT_SUCCESS;
}

void modDrawStuff(wfs_t *wfsinfo, mod_display_t *display, mod_sh_track_t *shtrack) {
	modBeginDraw(display->screen);

	// display image (raw, calibrated, dark, flat)
	modDisplayImg(wfsinfo, display);

	// display overlays (grid, subapts, vectors)
	if (display->dispover & DISPOVERLAY_GRID) 
		modDrawGrid(shtrack->cells, display->screen);
	if (display->dispover & DISPOVERLAY_SUBAPS)
		modDrawSubapts(shtrack, display->screen);
	if (display->dispover & DISPOVERLAY_VECTORS) 
		modDrawVecs(shtrack, display->screen);

	modFinishDraw(display->screen);
}
#endif


void modDrawSens(wfs_t *wfsinfo, mod_display_t *disp, SDL_Surface *screen) {	
	modBeginDraw(disp->screen);
	modDisplayImg(wfsinfo, disp);
	modFinishDraw(disp->screen);
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
	
    // SDL_Flip works for double buffered displays, but has a fallback 
    // is double buffering is not used, so it's safe to use in any case
	SDL_Flip(screen);
}
