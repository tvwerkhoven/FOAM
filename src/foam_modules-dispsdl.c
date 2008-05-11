/*! 
 @file foam_modules-dispsdl.c
 @author @authortim
 @date 2008-02-06 
 
 @brief This file contains display functions using only SDL.
 
 \section Info
 This module can be used to display data on a screen which is rather useful when 
 doing AO. Typical usage would start by initializing one or more mod_display_t 
 structs, and then pass each of these to displayInit(). This will give a blank 
 screen which can be used later on.
 
 To actually display stuff, call displayDraw() whenever you want. This module is 
 probably thread safe, so you can draw from different threads if you want. 
 
 Cleaning up should be done by calling displayFinish() at the end. Furthermore, 
 some lower level routines are also available, but are probably not necessary 
 with typical usage. If you think you know what you're doing, you might get away 
 with using them.
 
 \section Functions
 
 The functions provided to the outside world are:
 \li displayInit() - Initialize this module (setup SDL etc)
 \li displayDraw() - Actually draw stuff on the display. Note that SDL tends to sync to the refreshrate, so this routine blocks until done.
 \li displayEvent() - Process events that might occur (not configurable yet)
 \li displayFinish() - Clean up the module
 
 Some lower level routines include:
 \li displayBeginDraw() - Used to initialize the drawing of a single frame. Necessary if one functiones below are used. 
 \li displayFinishDraw() - Used to finalize the drawing of a single frame. Necessary if one functiones below are used. 
 \li displayImgByte() - Display an image stored as unsigned byte (8 bit) 
 \li displayGSLImgFloat() - Display a GSL matrix stored as floats. This routine first converts floats to bytes and then calls the previous function. This means that this function is rather slow.
 \li displayGrid() - Display a (square) grid showing the lenslet array (SH)
 \li displaySubapts() - Displays the current location of the subaperture tracking windows (SH)
 \li displayVecs() - Draw vectors between the center of the grid and the center of the tracker window (SH)

 
 The functions with (SH) appendices are Shack-Hartmann specific routines and are 
 only available if this module is compiled with FOAM_MODULES_DISLAY_SHSUPPORT.
 
 \section Dependencies
 
 This module might depend on the Shack Hartmann module if you want define 
 FOAM_MODULES_DISLAY_SHSUPPORT
 
 \section License
 
 This code is licensed under the GPL, version 2.
 
 */

// HEADERS //
/***********/

#include "foam_modules-dispsdl.h"

// GLOBAL VARIABLES //
/********************/

// Used to store a GSL image as byte if display a GSL matrix 
static uint8_t *tmpimg_b=NULL;

// LOCAL ROUTINES //
/******************/

/*!
 @brief This draws a rectangle starting at {coord[0], coord[1]} with size {size[0], size[1]} on screen *screen
 
 @param [in] coord Lower left coordinate of the rectangle to be drawn
 @param [in] size Size of the rectangle to be drawn 
 @param [in] *screen SDL_Surface to draw on
 */
static void drawRect(coord_t coord, coord_t size, SDL_Surface *screen);

static void drawDash(int x0, int y0, int x1, int y1, SDL_Surface *screen);

/*!
 @brief This draws a line from {x0, y0} to {x1, y1} without any aliasing
 
 @param [in] x0 starting x-coordinate
 @param [in] y0 starting y-coordinate
 @param [in] x1 end x-coordinate
 @param [in] y1 end y-coordinate
 @param [in] *screen SDL_Surface to draw on
 */
static void drawLine(int x0, int y0, int x1, int y1, SDL_Surface *screen);

static void drawDeltaLine(int x0, int y0, int dx, int dy, SDL_Surface *screen);

/*!
 @brief This draws one RGB pixel at a specific coordinate
 
 @param [in] *screen SDL_Surface to draw on
 @param [in] x x-coordinate to draw on
 @param [in] y y-coordinate to draw on
 @param [in] R red component of the colour
 @param [in] G green component of the colour
 @param [in] B blue component of the colour
 */
static void drawPixel(SDL_Surface *screen, int x, int y, Uint8 R, Uint8 G, Uint8 B);


static void drawRect(coord_t coord, coord_t size, SDL_Surface *screen) {
	// lower line
	drawLine(coord.x, coord.y, coord.x + size.x + 1, coord.y, screen);
	// top line
	drawLine(coord.x, coord.y + size.y, coord.x + size.x, coord.y + size.y, screen);
	// left line
	drawLine(coord.x, coord.y, coord.x, coord.y + size.y, screen);
	// right line
	drawLine(coord.x + size.x, coord.y, coord.x + size.x, coord.y + size.y, screen);
	// done
}

static void drawLine(int x0, int y0, int x1, int y1, SDL_Surface *screen) {
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

static void drawDash(int x0, int y0, int x1, int y1, SDL_Surface *screen) {
	int step = abs(x1-x0);
	int i;
	if (abs(y1-y0) > step) step = abs(y1-y0); // this can be done faster?
    
	float dx = (x1-x0)/(float) step;
	float dy = (y1-y0)/(float) step;
    
	drawPixel(screen, x0, y0, 255, 255, 255);
	for(i=0; i<step; i++) {
		if ((i / 10) % 2 == 1) // we're drawing a dash, so don't always draw a pixel
			continue;
        
		x1 = x0+i*dx; // since x1 is an integer, we can't just increment this, steps of 0.7 pixels wouldn't work...
		y1 = y0+i*dy;
		drawPixel(screen, x1, y1, 255, 255, 255); // draw directly to the screen in white
	}
}

static void drawDeltaLine(int x0, int y0, int dx, int dy, SDL_Surface *screen) {
	drawLine(x0, y0, x0+dx, y0+dy, screen);
}

static void drawPixel(SDL_Surface *screen, int x, int y, Uint8 R, Uint8 G, Uint8 B) {
	Uint32 color = SDL_MapRGB(screen->format, R, G, B);
	
	// crop the pixel coordinates if they go out of bound (good if you don't 
	// want to segfault or something)
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

// ROUTINES //
/************/

int displayInit(mod_display_t *disp) {
    if (SDL_Init(SDL_INIT_VIDEO) == -1) {
		logWarn("Could not initialize SDL: %s", SDL_GetError());
		return EXIT_FAILURE;
	}
	
	atexit(SDL_Quit);
	
	SDL_WM_SetCaption(disp->caption, 0);
	
	disp->flags = SDL_HWSURFACE | SDL_DOUBLEBUF;
	disp->screen = SDL_SetVideoMode(disp->res.x, disp->res.y, 0, disp->flags);
	
	if (disp->screen == NULL) {
		logWarn("Unable to set video: %s", SDL_GetError());
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

int displayFinish(mod_display_t *disp) {
	// no cleaning up necessary for SDL, done with atexit() in displayInit()
	return EXIT_SUCCESS;
}

#if (0) // Disable the following piece of code
// This function works but floats are currently not supported
int displayImgFloat(float *img, mod_display_t *disp) {
	// ONLY does float images as input
	int x, y;
    Uint8 i;
	float max=img[0];
	float min=img[0];
	float shift, scale;   // use shift and scale to adjust the pixel intensities    
	
	// we need this loop to check the maximum and minimum intensity. 
    if (disp->autocontrast == 1) {
        for (x=0; x < disp->res.x*disp->res.y; x++) {
            if (img[x] > max)
                max = img[x];
            else if (img[x] < min)
                min = img[x];
        }
        shift = -min;
		if (max-min != 0)
			scale = 255/(max-min);
		else 
			scale = 1;
		
    }
    else {
        shift = disp->brightness;
        scale = disp->contrast;
    }
	
	//logDebug(0, "Displaying image, (%5.3f, %5.3f) -> (%5.3f, %5.3f).", min, max, (min+shift) * scale, (max+shift)*scale);
    
	Uint32 color;
	// trust me, i'm not proud of this code either ;) TODO
	switch (disp->screen->format->BytesPerPixel) {
		case 1: { // Assuming 8-bpp
            Uint8 *bufp;
            
            for (x=0; x<disp->res.x; x++) {
                for (y=0; y<disp->res.y; y++) {
                    i = (Uint8) ((img[y*disp->res.x + x]+shift)*scale);
                    color = SDL_MapRGB(disp->screen->format, i, i, i);
                    bufp = (Uint8 *)disp->screen->pixels + y*disp->screen->pitch + x;
                    *bufp = color;
                }
            }
        }
			break;
		case 2: {// Probably 15-bpp or 16-bpp
            Uint16 *bufp;
            
            for (x=0; x<disp->res.x; x++) {
                for (y=0; y<disp->res.y; y++) {
                    i = (Uint8) ((img[y*disp->res.x + x]+shift)*scale);
                    color = SDL_MapRGB(disp->screen->format, i, i, i);
                    bufp = (Uint16 *)disp->screen->pixels + y*disp->screen->pitch/2 + x;
                    *bufp = color;
                }
            }
        }
			break;
		case 3: { // Slow 24-bpp mode, usually not used
            Uint8 *bufp;
            
            for (x=0; x<disp->res.x; x++) {
                for (y=0; y<disp->res.y; y++) {
                    i = (Uint8) ((img[y*disp->res.x + x]+shift)*scale);
                    color = SDL_MapRGB(disp->screen->format, i, i, i);
                    bufp = (Uint8 *)disp->screen->pixels + y*disp->screen->pitch + x * 3;
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
            for (x=0; x<disp->res.x; x++) {
                for (y=0; y<disp->res.y; y++) {
                    i = (Uint8) ((img[y*disp->res.x + x]+shift)*scale);
                    color = SDL_MapRGB(disp->screen->format, i, i, i);
                    bufp = (Uint32 *)disp->screen->pixels + y*disp->screen->pitch/4 + x;
                    *bufp = color;
                }
            }
        }
            break;
	}
	
	return EXIT_SUCCESS;
}
#endif

int displayImgByte(uint8_t *img, mod_display_t *disp) {
	int x, y;
    Uint8 i;					// SDL type
	uint8_t max=img[0];			// stdint type
	uint8_t min=img[0];
	uint8_t shift=0, scale=1;   // use shift and scale to adjust the pixel intensities
	
    if (disp->autocontrast == 1) {
		// we need this loop to check the maximum and minimum intensity. 
		for (x=0; x < disp->res.x * disp->res.y; x++) {
			if (img[x] > max)
				max = img[x];
			else if (img[x] < min)
				min = img[x];
		}
		shift = -min;
		if (max-min != 0)
			scale = 255/(max-min);
		else 
			scale = 1;
    }
	else {
        // use static brightness and contrast
        shift = disp->brightness;
        scale = disp->contrast;
    }    
	
	//logDebug(0, "Displaying image, (%d, %d) -> (%5.3f, %5.3f).", min, max, (min+shift) * scale, (max+shift)*scale);
    
	Uint32 color;
	// trust me, i'm not proud of this code either ;)
	// unfortunately, there doesn't seem to be a (much) better way, save of using OpenGL
	switch (disp->screen->format->BytesPerPixel) {
		case 1: { // Assuming 8-bpp
            Uint8 *bufp;
            
            for (x=0; x<disp->res.x; x++) {
                for (y=0; y<disp->res.y; y++) {
                    i = (Uint8) ((img[y*disp->res.x + x]+shift)*scale);
                    color = SDL_MapRGB(disp->screen->format, i, i, i);
                    bufp = (Uint8 *)disp->screen->pixels + y*disp->screen->pitch + x;
                    *bufp = color;
                }
            }
        }
			break;
		case 2: {// Probably 15-bpp or 16-bpp
            Uint16 *bufp;
            
            for (x=0; x<disp->res.x; x++) {
                for (y=0; y<disp->res.y; y++) {
                    i = (Uint8) ((img[y*disp->res.x + x]+shift)*scale);
                    color = SDL_MapRGB(disp->screen->format, i, i, i);
                    bufp = (Uint16 *)disp->screen->pixels + y*disp->screen->pitch/2 + x;
                    *bufp = color;
                }
            }
        }
			break;
		case 3: { // Slow 24-bpp mode, usually not used
            Uint8 *bufp;
            
            for (x=0; x<disp->res.x; x++) {
                for (y=0; y<disp->res.y; y++) {
                    i = (Uint8) ((img[y*disp->res.x + x]+shift)*scale);
                    color = SDL_MapRGB(disp->screen->format, i, i, i);
                    bufp = (Uint8 *)disp->screen->pixels + y*disp->screen->pitch + x * 3;
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
            for (x=0; x<disp->res.x; x++) {
                for (y=0; y<disp->res.y; y++) {
                    i = (Uint8) ((img[y*disp->res.x + x]+shift)*scale);
                    color = SDL_MapRGB(disp->screen->format, i, i, i);
                    bufp = (Uint32 *)disp->screen->pixels + y*disp->screen->pitch/4 + x;
                    *bufp = color;
                }
            }
        }
			break;
	}
	return EXIT_SUCCESS;
}

int displayGSLImg(gsl_matrix_float *gslimg, mod_display_t *disp, int doscale) {
	int i, j;
	float min, max;
	// we want to display a GSL image, copy it over the normal image 
	if (tmpimg_b == NULL)
		tmpimg_b = malloc(disp->res.x * disp->res.y);
	
	if (doscale == 1) {
		// although this calls seem to be fast code (asm optimized or something)
		// this is absolutely not the case. The following is dog slow.
		gsl_matrix_float_minmax(gslimg, &min, &max);
		gsl_matrix_float_add_constant(gslimg, -min);
		gsl_matrix_float_scale(gslimg, 255/(max-min));
	}
	
	// Copy the float image to a byte array so we can display it (again, slooow)
	for (i=0; i < disp->res.x; i++) {
		for (j=0; j < disp->res.y; j++) {
			tmpimg_b[i*disp->res.x + j] = (uint8_t) gsl_matrix_float_get(gslimg, i, j);
		}
	}
	
	// and finally display the copied image using displayImgByte()
	return displayImgByte(tmpimg_b, disp);
}

#ifdef FOAM_MODULES_DISLAY_SHSUPPORT
int displayDraw(wfs_t *wfsinfo, mod_display_t *disp, mod_sh_track_t *shtrack) {
#else
int displayDraw(wfs_t *wfsinfo, mod_display_t *disp) {
#endif
	displayBeginDraw(disp);

    // !!!:tim:20080507 this is probably not very clean (float is not 16 bpp)
    // databpp == 16 
    // databpp == 8 char (uint8_t)
	if (disp->dispsrc == DISPSRC_RAW) {
		if (wfsinfo->bpp == 8) {
//			uint8_t *imgc = 
			displayImgByte((uint8_t *) wfsinfo->image, disp);
		}
		else {
			displayFinishDraw(disp);
			return EXIT_FAILURE;
		}
		// !!!:tim:20080510 floats deprecated for the moment
//		else if (wfsinfo->bpp == 16) {
//			float *imgc = (float *) wfsinfo->image;
//			modDisplayImgFloat(imgc, disp);
//		}
	}
	else if (disp->dispsrc == DISPSRC_DARK) {
		displayGSLImg(wfsinfo->darkim, disp, 1);
	}
	else if (disp->dispsrc == DISPSRC_FLAT) {
		displayGSLImg(wfsinfo->flatim, disp, 1);
	}
	else if (disp->dispsrc == DISPSRC_CALIB) {
		displayGSLImg(wfsinfo->corrim, disp, 1);
	}
	
#ifdef FOAM_MODULES_DISLAY_SHSUPPORT
	// display overlays (grid, subapts, vectors)
	if (display->dispover & DISPOVERLAY_GRID) 
		displayGrid(shtrack->cells, display);
	if (display->dispover & DISPOVERLAY_SUBAPS)
		displaySubapts(shtrack, display);
	if (display->dispover & DISPOVERLAY_VECTORS) 
		displayVecs(shtrack, display);
#endif
	
	displayFinishDraw(display);
	
    return EXIT_FAILURE;
}
	
void displaySDLEvents(mod_display_t *dispgl) {
	// Not implemented in the SDL-only display module yet.
	return;
}

#ifdef FOAM_MODULES_DISLAY_SHSUPPORT

int displaySubapts(mod_sh_track_t *shtrack, mod_display_t *disp) {
	if (shtrack->nsubap == 0)
		return EXIT_SUCCESS;				// if there's nothing to draw, don't draw (shouldn't happen)
        
	if (shtrack->subc == NULL)
		return EXIT_SUCCESS;				// if there's nothing to draw, don't draw (shouldn't happen)

	int sn=0;
    
	// we draw the reference subaperture rectangle bigger than the rest to indicate 
    // that this subapt is the reference subapt. Subapertures are shtrack->track
    // big, and have the lower left coordinate at subc[sn].x, subc[sn].y. We draw
    // the reference subapt shtrack->shsize big, so we need subc[sn] - (shsize - track)/2
	coord_t refcoord;
    refcoord.x = shtrack->subc[0].x - (shtrack->shsize.x - shtrack->track.x)/2;
    refcoord.y = shtrack->subc[0].y - (shtrack->shsize.y - shtrack->track.y)/2;
    
	drawRect(refcoord, shtrack->shsize, disp->screen);
	
	for (sn=1; sn< shtrack->nsubap; sn++) {
		// subapt with lower coordinates (subc[sn][0],subc[sn][1])
		// first subapt has size (shsize[0],shsize[1]),
		// the rest are (subsize[0],subsize[1])
		drawRect(shtrack->subc[sn], shtrack->track, disp->screen);
	}
	
	return EXIT_SUCCESS;
}

int displayVecs(mod_sh_track_t *shtrack, mod_display_t *disp) {
	if (shtrack->nsubap == 0)
		return EXIT_SUCCESS;				// if there's nothing to draw, don't draw (shouldn't happen)

	if (shtrack->disp == NULL)
		return EXIT_SUCCESS;				// if there's nothing to draw, don't draw (shouldn't happen)

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
                      disp->screen);
	
	return EXIT_SUCCESS;
}

int displayGrid(coord_t gridres, mod_display_t *disp) {
	int xc, yc;
	int gridw = disp->windowres.x/gridres.x;
	int gridh = disp->windowres.y/gridres.y;
    
	for (xc=1; xc < gridres.x; xc++)
		drawDash(xc*gridw, 0, xc*gridw, disp->windowres.y, disp->screen);
	
	for (yc=1; yc< gridres.y; yc++)
		drawDash(0, yc*gridh, disp->windowres.x, yc*gridh, disp->screen);
    
	return EXIT_SUCCESS;
}

#endif

void displayBeginDraw(mod_display_t *disp) {
	if ( SDL_MUSTLOCK(disp->screen) )	{
		if ( SDL_LockSurface(disp->screen) < 0 )
			return;
	}
}

void displayFinishDraw(mod_display_t *disp) {
	if ( SDL_MUSTLOCK(disp->screen) )
		SDL_UnlockSurface(disp->screen);
	
	// SDL_Flip works for double buffered displays, but has a fallback 
	// is double buffering is not used, so it's safe to use in any case
	SDL_Flip(disp->screen);
}

	
