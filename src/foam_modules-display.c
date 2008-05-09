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

// GLOBAL VARIABLES //
/********************/

// these are not used atm, TvW 2008-05-06 17:07
#define FOAM_MODDISPLAY_PRIO 1
//static pthread_t moddisplay_thread;
//static pthread_attr_t moddisplay_attr;
//static int moddisplay_drawing = 1;

// this will be used to temporarily store an image if displaying a GSL matrix
// since this needs conversion from the GSL datatype to a simple row-major
// matrix, we need a place to store it, and that's right here. Note that the
// data will only be allocated when necessary, and will only be as big the 
// wavefront sensor.
static uint8_t *tmpimg_b=NULL;

// ROUTINES //
/************/


int modInitDraw(mod_display_t *disp) {
    if (SDL_Init(SDL_INIT_VIDEO) == -1) {
		logWarn("Could not initialize SDL: %s", SDL_GetError());
		return EXIT_FAILURE;
	}
	
	atexit(SDL_Quit);
	
	SDL_WM_SetCaption(disp->caption, 0);
	
	disp->screen = SDL_SetVideoMode(disp->res.x, disp->res.y, 0, disp->flags);
	//SDL_HWSURFACE | SDL_DOUBLEBUF | SDL_RESIZABLE
	if (disp->screen == NULL) {
		logWarn("Unable to set video: %s", SDL_GetError());
		return EXIT_FAILURE;
	}
	
	// we use this routine to startup a helper thread which will do the actual SDL
	// such that the drawing calls are non-blocking.
    //	int rc;
    //
    //	pthread_attr_init(&moddisplay_attr);
    //	pthread_attr_setdetachstate(&moddisplay_attr, PTHREAD_CREATE_JOINABLE);
    //	
    //	rc = pthread_create(&moddisplay_thread, &moddisplay_attr, drawLoop, NULL);
    //	if (rc) {
    //		logWarn("Failed to initalize drawing module, drawing will not work.");
    //		return EXIT_FAILURE;
    //	}
    //	
	return EXIT_SUCCESS;
}

int modStopDraw() {
	// we're done, let the thread finish
    //	moddisplay_drawing = 0;
	
	// join with the display thread
    //	pthread_join(moddisplay_thread, NULL);
	
    //	pthread_attr_destroy(&moddisplay_attr);
	return EXIT_SUCCESS;
}

void drawLoop() {
    //	while (moddisplay_drawing == 1) {
    //		
    //	}
	
	// ok, we're done with drawing, stop here and join with the calling thread
    //	pthread_exit(0);
}

void drawRect(coord_t coord, coord_t size, SDL_Surface *screen) {
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
		if ((i / 10) % 2 == 1) // we're drawing a dash, so don't always draw a pixel
			continue;
        
		x1 = x0+i*dx; // since x1 is an integer, we can't just increment this, steps of 0.7 pixels wouldn't work...
		y1 = y0+i*dy;
		drawPixel(screen, x1, y1, 255, 255, 255); // draw directly to the screen in white
	}
}

void drawDeltaLine(int x0, int y0, int dx, int dy, SDL_Surface *screen) {
	drawLine(x0, y0, x0+dx, y0+dy, screen);
}

// Todo: fix img dereferencing
/*
 int modDisplayRaw(void *img, coord_t res, int datatype, SDL_Surface *screen) {
 // datatype == 0: float
 // datatype == 1: char (uint8_t)
 int x, y, i;
 float max;
 float min;
 
 // we need this loop to check the maximum and minimum intensity. 
 for (x=0; x < res.x*res.y; x++) {
 if (img[x] > max)
 max = img[x];
 if (img[x] < min)
 min = img[x];
 }
 
 logDebug(0, "Displaying image, min: %5.3f, max: %5.3f.", min, max);
 
 Uint32 color;
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
 default: {
 logWarn("Warning, unsupported bitdepth encounterd (only 8 & 32 bpp implemented)");
 return EXIT_FAILURE;
 }
 }
 
 return EXIT_SUCCESS;
 }
 */

int modDisplayImgFloat(float *img, mod_display_t *disp) {
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
        scale = 255/(max-min);
    }
    else {
        shift = disp->brightness;
        scale = disp->contrast;
    }
	
	logDebug(0, "Displaying image, min: %5.3f, max: %5.3f.", min, max);
    
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

int modDisplayImgByte(uint8_t *img, mod_display_t *disp) {
	int x, y;
    Uint8 i;                // SDL type
	uint8_t max=img[0];     // stdint type
	uint8_t min=img[0];
	uint8_t shift, scale;   // use shift and scale to adjust the pixel intensities
	
    if (disp->autocontrast == 1) {
        // we need this loop to check the maximum and minimum intensity. 
        for (x=0; x < disp->res.x * disp->res.y; x++) {
            if (img[x] > max)
                max = img[x];
            else if (img[x] < min)
                min = img[x];
        }
        shift = -min;
        scale = 255/(max-min);
    }
    else {
        // use static brightness and contrast
        shift = disp->brightness;
        scale = disp->contrast;
    }
    
	
	logDebug(0, "Displaying image, min: %d, max: %d.", min, max);
    
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

int modDisplayGSLImg(gsl_matrix_float *gslimg, mod_display_t *disp) {
	int i, j;
	// we want to display a GSL image, copy it over the normal image 
	if (tmpimgb == NULL)
		tmpimgb = malloc(disp->res.x disp->res.y);
	
	for (j=0; j < disp->res.y; j++) {
		for (i=0; i < disp->res.x; i++) {
			tmpimgb[j*disp->res.x + i] = (uint8_t) gsl_matrix_float_get(gslimg, i, j);
		}
	}
	modDisplayImgByte(tmpimgb, disp);
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
		modDisplayGSLImg(wfsinfo->darkim, disp);
	}
	else if (disp->dispsrc == DISPSRC_FLAT) {
		modDisplayGSLImg(wfsinfo->flatim, disp);
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
	
	for (sn=1; sn< shtrack->nsubap; sn++) {
		// subapt with lower coordinates (subc[sn][0],subc[sn][1])
		// first subapt has size (shsize[0],shsize[1]),
		// the rest are (subsize[0],subsize[1])
		drawRect(shtrack->subc[sn], shtrack->track, screen);
	}
	
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
	modDisplayImg(wfsinfo, display);
	modDrawGrid(shtrack->cells, display->screen);
	modDrawSubapts(shtrack, display->screen);
	modDrawVecs(shtrack, display->screen);
	modFinishDraw(display->screen);
}
#endif


void modDrawSens(wfs_t *wfsinfo, mod_display_t *disp, SDL_Surface *screen) {	
	modBeginDraw(disp->screen);
	modDisplayImg(wfsinfo->image, disp, wfsinfo->bpp);
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
