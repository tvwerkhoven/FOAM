/*! 
 @file foam_modules-dispgl.c
 @author @authortim
 @date 2008-05-09
 
 @brief This file contains functions to display stuff using SDL & OpenGL
 
 \section Info
 This module can be used to display images, lines, rectangles etc. using hardware accelartion.
 If available, usage of opengl is recommended. This has the advantage of being much
 easier to use (less error-prone), much faster if hardware is available, and provides non-blocking 
 flush commands to quickly deliver the drawing commands. 
 
 Fortunately, even northbridge-integrated video chips have hardware accelartion nowadays so 
 this constraint shouldn't be too bad. If OpenGL is not an option, use the display module instead.
 
 \section Functions
 
 The functions provided to the outside world are:
 
 \section Dependencies
 
 This module does not depend on other modules.
 
 \section License
 
 This code is licensed under the GPL, version 2.
 */

// HEADERS //
/***********/

#include "foam_modules-dispgl.h"

// PRIVATE ROUTINES //
/********************/

static void resizeWindow(mod_dispgl_t *dispgl) {
	// Resize the opengl viewport to the new windowsize
	glViewport(0, 0, (GLsizei) (dispgl->windowres.x), (GLsizei) (dispgl->windowres.y)); 
	// Reset the coordinate system to the source image size
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	// This defines the coordinate system we're using, which is in the CCD-space so to say
	gluOrtho2D(0, sourcesize.x, sourcesize.y, 0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	// Set the pixelzooming
	glPixelZoom((GLfloat) windowsize.w/sourcesize.x, (GLfloat) windowsize.h/sourcesize.y);
	glFlush();
	SDL_GL_SwapBuffers();
}

static void drawRect(coord_t origin, coord_t size) {
	glBegin(GL_LINE_LOOP);
	glVertex2f(origin.x			, origin.y			);
	glVertex2f(origin.x + size.x, origin.y			);
	glVertex2f(origin.x + size.x, origin.y + size.y	);
	glVertex2f(origin.x			, origin.y + size.y	);
	glEnd();
}


// PUBLIC ROUTINES //
/*******************/

void dispglBeginDraw() {
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
}

void dispglFinishDraw() {
	glFlush();
	SDL_GL_SwapBuffers();
}

int dispglInit(mod_dispgl_t *dispgl) {
	
    if (SDL_Init(SDL_INIT_VIDEO) == -1) {
		logWarn("Could not initialize SDL: %s", SDL_GetError());
		return EXIT_FAILURE;
	}
	
	dispgl->info = SDL_GetVideoInfo( );
	
	if (!dispgl->info) {
		logWarn("Could not get video info from SDL: %s", SDL_GetError());
		return EXIT_FAILURE;
	}
	
	dispgl->bpp = dispgl->info->vfmt->BitsPerPixel;
	
	// a minimal opengl setup, we don't need much so don't ask too much
	SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 5 );
    SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 5 );
    SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 5 );
    SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 8 );
    SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
	
	atexit(SDL_Quit);
	
	SDL_WM_SetCaption(dispgl->caption, 0);
	
	// flags should be like:
	// SDL_HWSURFACE | SDL_OPENGL | SDL_RESIZABLE
	dispgl->screen = SDL_SetVideoMode(dispgl->windowres.x, dispgl->windowres.y, dispglgl->bpp, dispgl->flags);
	
	if (dispgl->screen == NULL) {
		logWarn("Unable to set video mode using SDL: %s", SDL_GetError());
		return EXIT_FAILURE;
	}

	glClearColor(0.0, 0.0, 0.0, 0.0);
    glClearDepth(1.0f );
	
	
    glViewport( 0, 0, ( GLsizei )dispgl->windowres.x, ( GLsizei )dispgl->windowres.y );
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, dispgl->res.x, dispgl->res.y, 0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
	
	
	return EXIT_SUCCESS;
}

int dispglShowImg(void *img, mod_dispgl_t *dispgl, int databpp) {
    // !!!:tim:20080507 this is probably not very clean (float is not 16 bpp)
    // databpp == 16 
    // databpp == 8 char (uint8_t)
    if (databpp == 8) {
		glDrawPixels((GLsizei) dispgl->res.x, (GLsizei) dispgl->res.y, GL_LUMINANCE, GL_UNSIGNED_BYTE, (const GLvoid *) img);
    }
    else if (databpp == 16) {
		glDrawPixels((GLsizei) dispgl->res.x, (GLsizei) dispgl->res.y, GL_LUMINANCE, GL_FLOAT, (const GLvoid *) img);
    }
    
    return EXIT_FAILURE;
}

void dispglSDLEvents(mod_dispgl_t *dispgl) {
	// Poll for events, and handle the ones we care about.
	SDL_Event event;
	while (SDL_PollEvent(&event)) 
	{
		switch (event.type) 
		{
			case SDL_MOUSEBUTTONDOWN:
				//fprintf(stderr, "mouse at: (%d, %d)\n", event.button.x, event.button.y);
				break;
			case SDL_KEYUP:
				// If escape is pressed, return (and thus, quit)
				if (event.key.keysym.sym == SDLK_ESCAPE)
					stopFoam();
				break;
			case SDL_VIDEORESIZE:
				logDebug(0, "Resizing window to %d,%d\n", event.resize.w, event.resize.h);
				
				// update window information in the struct
				dispgl->windowres.x = event.resize.w
				dispgl->windowres.y = event.resize.h
				// Get new SDL video mode
				dispgl->screen = SDL_SetVideoMode( event.resize.w,event.resize.h, bpp, flags);
				// Do the actual (opengl) resizing)
				dispglResize(dispgl);
				
				break;
			case SDL_QUIT:
				stopFoam();
		}
	}
}

#ifdef FOAM_MODULES_DISLAY_SHSUPPORT

// !!!:tim:20080414 shortcut for SH display routines
void dispglDrawSubapts(mod_sh_track_t *shtrack) {
	if (shtrack->nsubap == 0)
		return;				// if there's nothing to draw, don't draw (shouldn't happen)
    
	int sn=0;
    
	// we draw the reference subaperture rectangle bigger than the rest to indicate 
    // that this subapt is the reference subapt. Subapertures are shtrack->track
    // big, and have the lower left coordinate at subc[sn].x, subc[sn].y. We draw
    // the reference subapt shtrack->shsize big, so we need subc[sn] - (shsize - track)/2
	coord_t refcoord;
    refcoord.x = shtrack->subc[0].x - (shtrack->shsize.x - shtrack->track.x)/2;
    refcoord.y = shtrack->subc[0].y - (shtrack->shsize.y - shtrack->track.y)/2;
    
	drawRect(refcoord, shtrack->shsize);
	
	for (sn=1; sn< shtrack->nsubap; sn++) {
		// subapt with lower coordinates (subc[sn].x, subc[sn].y)
		// first subapt has size (shsize.x, shsize.y),
		// the rest are (track.x, track.y)
		drawRect(shtrack->subc[sn], shtrack->track);
	}
}

// !!!:tim:20080414 shortcut for SH display routines
void dispglDrawVecs(mod_sh_track_t *shtrack) {
	if (shtrack->nsubap == 0)
		return;		// if there's nothing to draw, don't draw (shouldn't happen)
	
	int sn=0;
	
    // To draw the displacement vector relative to the center of the lenslet grid,
    // we take gridc, which are lower left coordinates of the grid cells, and 
    // add half the grid size to get to the center, and then use the disp vector
    // to draw the vector itself.
	glColor3f(1.0, 1.0, 0.0);
	glBegin(GL_LINES);
	for (sn=0; sn < shtrack->nsubap; sn++) {
		glVertex2f(shtrack->gridc[sn].x + (shtrack->shsize.x/2), shtrack->gridc[sn].y + (shtrack->shsize.y/2));
		glVertex2f(gsl_vector_float_get(shtrack->disp, sn*2+0), gsl_vector_float_get(shtrack->disp, sn*2+1));
	}
	glEnd();
	
}

void dispglGrid(coord_t gridres, coord_t imgres) {
	int j;
	glBegin(GL_LINES);
	glColor3f(0.0, 1.0, 0.0);
	for (j=1; j < gridres.x; j++) {
		glVertex2f(j*imgres.x/gridres.x, 0);
		glVertex2f(j*imgres.x/gridres.x, imgres.y);
	}
	for (j=1; j < shtrack->cells.y; j++) {
		glVertex2f(j*imgres.y/gridres.y, 0);
		glVertex2f(j*imgres.y/gridres.y, imgres.x);
	}
	
	glEnd();
	
	return EXIT_SUCCESS;
}

#endif

#ifdef FOAM_MODULES_DISLAY_SHSUPPORT
void dispglDrawStuff(wfs_t *wfsinfo, mod_dispgl_t *dispgl, mod_sh_track_t *shtrack) {
#else
void dispglDrawStuff(wfs_t *wfsinfo, mod_dispgl_t *dispgl) {
#endif
	dispglBeginDraw();
	dispglShowImg(wfsinfo->image, dispgl, wfsinfo->bpp);
	
#ifdef FOAM_MODULES_DISLAY_SHSUPPORT	
	dispglGrid(shtrack->cells, wfsinfo->res);
	dispglDrawSubapts(shtrack) {
	dispglDrawVecs(shtrack)
#endif
		
	dispglFinishDraw();
}