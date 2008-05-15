/*! 
 @file foam_modules-dispgl.c
 @author @authortim
 @date 2008-05-09
 
 @brief This file contains functions to display stuff using SDL & OpenGL
 
 \section Info
 This module can be used to display data on a screen which is rather useful when 
 doing AO. Typical usage would start by initializing one or more mod_display_t 
 structs, and then pass each of these to displayInit(). This will give a blank 
 screen which can be used later on.
 
 Fortunately, even northbridge-integrated video chips have hardware accelartion nowadays so 
 this constraint shouldn't be too bad. If OpenGL is not an option, use the display module instead.
 
 To actually display stuff, call displayDraw() whenever you want. This module is 
 probably thread safe, so you can draw from different threads if you want. 
 
 Cleaning up should be done by calling displayFinish() at the end. Furthermore, 
 some lower level routines are also available, but are probably not necessary 
 with typical usage. If you think you know what you're doing, you might get away 
 with using them.
 
 \section Functions
 
 The functions provided to the outside world are:
 \li displayInit() - Initialize this module (setup SDL, OpenGL etc)
 \li displayDraw() - Actually draw stuff on the display. OpenGL is nonblocking, so this can be fast
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

#include "foam_modules-dispcommon.h"

// PRIVATE ROUTINES //
/********************/

static void resizeWindow(mod_display_t *disp) {
	// Resize the opengl viewport to the new windowsize
	glViewport(0, 0, (GLsizei) (disp->windowres.x), (GLsizei) (disp->windowres.y)); 
	// Reset the coordinate system to the source image size
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	// This defines the coordinate system we're using, which is in the CCD-space so to say
	gluOrtho2D(0, disp->res.x, 0, disp->res.y);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	// Set the pixelzooming
	glPixelZoom((GLfloat) disp->windowres.x/disp->res.x, (GLfloat) disp->windowres.y/disp->res.y);
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

void displayBeginDraw(mod_display_t *disp) {
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	// this might be redundant, but since these values
	// are changed in a different thread, we cannot easily
	// change it from there.
	glColor3ub(disp->col.r, disp->col.g, disp->col.b);
}

void displayFinishDraw(mod_display_t *disp) {
	glFlush();
	SDL_GL_SwapBuffers();
}

int displayInit(mod_display_t *disp) {
	
    if (SDL_Init(SDL_INIT_VIDEO) == -1) {
		logWarn("Could not initialize SDL: %s", SDL_GetError());
		return EXIT_FAILURE;
	}
	
	disp->info = SDL_GetVideoInfo( );
	
	if (!disp->info) {
		logWarn("Could not get video info from SDL: %s", SDL_GetError());
		return EXIT_FAILURE;
	}
	
	disp->bpp = disp->info->vfmt->BitsPerPixel;
	
	// a minimal opengl setup, we don't need much so don't ask too much
	SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 5 );
    SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 5 );
    SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 5 );
    SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 8 );
    SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
	
	atexit(SDL_Quit);
	
	SDL_WM_SetCaption(disp->caption, 0);
	
	// init the windowres coordinates to the ccd resolution
	disp->windowres.x = disp->res.x;
	disp->windowres.y = disp->res.y;
	// flags should be like:
	disp->flags = SDL_OPENGL | SDL_RESIZABLE;
	disp->screen = SDL_SetVideoMode(disp->windowres.x, disp->windowres.y, disp->bpp, disp->flags);
	
	if (disp->screen == NULL) {
		logWarn("Unable to set video mode using SDL: %s", SDL_GetError());
		return EXIT_FAILURE;
	}

	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClearDepth(1.0f );
	// set the colors for the overlays (all lines etc will be drawn with this color from here on, only set once)
	
//	Unecessary, col.[r|g|b] is uint8_t
//	if (disp->col.r > 255 || disp->col.r < 0 || \
//		disp->col.g > 255 || disp->col.g < 0 || \
//		disp->col.b > 255 || disp->col.b < 0) {
//		logWarn("Warning, color range invalid, please give RGB values between 0 and 255, defaulting to white (255,255,255)");
//		disp->col.r = disp->col.g = disp->col.b = 255;
//	}

	glColor3ub(disp->col.r, disp->col.g, disp->col.b);


	glViewport( 0, 0, ( GLsizei )disp->windowres.x, ( GLsizei )disp->windowres.y );
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0, disp->res.x, 0, disp->res.y);
	glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
	
	
	return EXIT_SUCCESS;
}

int displayFinish(mod_display_t *disp) {
	// no cleaning up necessary for SDL w/ OpenGL
	return EXIT_SUCCESS;
}

#ifdef FOAM_MODULES_DISLAY_SHSUPPORT
int displayImgByte(uint8_t *img, mod_display_t *disp, mod_sh_track_t *shtrack) {
#else
int displayImgByte(uint8_t *img, mod_display_t *disp) {
#endif
	// contrasting is done like this:
	// We're using an 8 bit camera (at least that's the assumption :P)
	// which gives pixel values of [0,255]. Since ao3.c used this, we also
	// use this, and an image is diplayed over the whole intensity range
	// if the scaled values range from 0 to 255. Consider a darkfield image
	// with average intensity 5, brightness 0 and contrast 10, this will
	// scale the pixels from 0 to 50, giving a dark-gray darkfield image.
	// Because OpenGL accepts only [0,1], we divide the actual brightness
	// and contrast by 255 so that the image is scaled between 0 and 1
	// (again, avg intensity 5, b=0, c=10 gives scaled avg 5*(10/255))
	int i, pixels;
	float min, max;
	if (disp->autocontrast == 1) {
		// if autocontrast is set to one, we calculate the min and max
		// values once, and use to calculate brightness and contrast
		// which are then used to scale the intensity from 0 to 255
		min = max = (float) img[0];

		// if we're displaying a raw img, we have all pixels 
		// otherwise we just have track.x*.y * nsubap pixels,
		// since we don't correct the whole field, but just 
		// the illuminated pixels
		if (disp->dispsrc == DISPSRC_RAW) 
			pixels = disp->res.x * disp->res.y;
#ifdef FOAM_MODULES_DISLAY_SHSUPPORT
		else if (disp->dispsrc == DISPSRC_FASTCALIB) 
			pixels = shtrack->nsubap * shtrack->track.x * shtrack->track.y;
#endif

		for (i=0; i<pixels; i++) {
			if (img[i] > max) max = (float) img[i];
			else if (img[i] < min) min = (float) img[i];
		}
		disp->brightness = -min;
		disp->contrast = 255.0/(max-min);
		disp->autocontrast = 0;
		logInfo(0, "Autocontrast found min: %f, max: %f, giving brightness: %d, contrast: %f", \
			min, max, disp->brightness, disp->contrast);
	}
	// using the brightness and contrast values (which should
	// scale the image in the [0,255] range), we do not scale here 
	// because this is a byte image.
	glPixelTransferf(GL_RED_SCALE, (GLfloat) disp->contrast);
	glPixelTransferf(GL_GREEN_SCALE, (GLfloat) disp->contrast);
	glPixelTransferf(GL_BLUE_SCALE, (GLfloat) disp->contrast);
	glPixelTransferf(GL_RED_BIAS, (GLfloat) disp->brightness);
	glPixelTransferf(GL_GREEN_BIAS, (GLfloat) disp->brightness);
	glPixelTransferf(GL_BLUE_BIAS, (GLfloat) disp->brightness);
	if (disp->dispsrc == DISPSRC_RAW)  {
		glDrawPixels((GLsizei) disp->res.x, (GLsizei) disp->res.y, GL_LUMINANCE, GL_UNSIGNED_BYTE, (const GLvoid *) img);
	}
#ifdef FOAM_MODULES_DISLAY_SHSUPPORT
	else if (disp->dispsrc == DISPSRC_FASTCALIB) {
		float f[4];
		//glGetFloatv(GL_CURRENT_RASTER_POSITION, f);
		//logDebug(0, "Drawing subapts, first at (%d, %d), rast at: (%.2f, %.2f, %.2f, %.2f)", shtrack->subc[0].x, shtrack->subc[0].y, f[0], f[1], f[2], f[3]);
		for (i=0; i<shtrack->nsubap; i++) {
			// we loop over all subapts, and display them one by
			// one at coordinates subc[i].x, .y with size
			// (track.x, .y). Rasterposition (re)sets the coor-
			// dinates where pixels are drawn.
			glRasterPos2i(shtrack->subc[i].x, shtrack->subc[i].y);
			glDrawPixels((GLsizei) shtrack->track.x, (GLsizei) shtrack->track.y, GL_LUMINANCE, GL_UNSIGNED_BYTE, (const GLvoid *) img);
		}
		// reset raster position
		glRasterPos2i(0, 0);
		//glGetFloatv(GL_CURRENT_RASTER_POSITION, f);
		//logDebug(0, "Rast at: (%.2f, %.2f, %.2f, %.2f)", f[0], f[1], f[2], f[3]);
	}
#endif
		pixels = shtrack->nsubap * shtrack->track.x * shtrack->track.y;
	return EXIT_SUCCESS;
}

int displayGSLImg(gsl_matrix_float *img, mod_display_t *disp, int doscale) {
	// TvW, May 13, 2008, this code can be improved because we do not need
	// to manually copy the gsl matrix to a foat array, OpenGL can
	// directly copy floats even if the stride != width of the image.
    static float *gsltmp=NULL;
    int i, j;
	float min, max;
    if (gsltmp == NULL) {
	    gsltmp = malloc(disp->res.x * disp->res.y * sizeof(float));
    }

    for (i=0; i< disp->res.y; i++) {
	    for (j=0; j<disp->res.x; j++) {
		    gsltmp[i*disp->res.y + j] = gsl_matrix_float_get(img, i, j);
	    }
    }
	// see contrast hints above
	if (disp->autocontrast == 1) {
		min = max = gsltmp[0];
		for (i=0; i<disp->res.y; i++) {
			for (j=0; j<disp->res.x; j++) {
				if (gsltmp[i * disp->res.x + j] > max) max = gsltmp[i * disp->res.x + j];
				else if (gsltmp[i * disp->res.x + j] < min) min = gsltmp[i * disp->res.x + j];
			}
		}
		disp->brightness = -min;
		disp->contrast = 255.0/(max-min);
		disp->autocontrast = 0;
		logInfo(0, "Autocontrast found min: %f, max: %f, giving brightness: %d, contrast: %f", \
			min, max, disp->brightness, disp->contrast);
	}
	glPixelTransferf(GL_RED_SCALE, (GLfloat) disp->contrast/255.0);
	glPixelTransferf(GL_GREEN_SCALE, (GLfloat) disp->contrast/255.0);
	glPixelTransferf(GL_BLUE_SCALE, (GLfloat) disp->contrast/255.0);
	glPixelTransferf(GL_RED_BIAS, (GLfloat) disp->brightness/255.0);
	glPixelTransferf(GL_GREEN_BIAS, (GLfloat) disp->brightness/255.0);
	glPixelTransferf(GL_BLUE_BIAS, (GLfloat) disp->brightness/255.0);
    glDrawPixels((GLsizei) disp->res.x, (GLsizei) disp->res.y, GL_LUMINANCE, GL_FLOAT, (const GLvoid *) gsltmp);

    return EXIT_FAILURE;
}
void displaySDLEvents(mod_display_t *disp) {
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
				//if (event.key.keysym.sym == SDLK_ESCAPE)
					//stopFoam();
				break;
			case SDL_VIDEORESIZE:
				logDebug(0, "Resizing window to %d,%d\n", event.resize.w, event.resize.h);
				
				// update window information in the struct
				disp->windowres.x = event.resize.w;
				disp->windowres.y = event.resize.h;
				// Get new SDL video mode
				disp->screen = SDL_SetVideoMode( event.resize.w, event.resize.h, disp->bpp, disp->flags);
				// Do the actual (opengl) resizing)
				resizeWindow(disp);
				
				break;
			case SDL_QUIT:
					return;
		}
	}
}
#ifdef FOAM_MODULES_DISLAY_SHSUPPORT

int displaySubapts(mod_sh_track_t *shtrack, mod_display_t *disp) {
	if (shtrack->nsubap == 0)
		return EXIT_SUCCESS;				// if there's nothing to draw, don't draw (shouldn't happen)
    
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
	return EXIT_SUCCESS;
}

int displayVecs(mod_sh_track_t *shtrack, mod_display_t *disp) {
	if (shtrack->nsubap == 0)
		return EXIT_SUCCESS;		// if there's nothing to draw, don't draw (shouldn't happen)

	int sn=0;

	// To draw the displacement vector relative to the center of the lenslet grid,
	// we take gridc, which are lower left coordinates of the grid cells, and 
	// add half the grid size to get to the center, and then use the disp vector
	// to draw the vector itself.
	// color already set at init, unecessary if no change
	//glColor3f(1.0, 1.0, 0.0);
	//glColor3f(disp->col.r, disp->col.g, disp->col.b);
	glBegin(GL_LINES);
	sn = 0;
	logDebug(LOG_SOMETIMES, "Drawing vectors: (%d,%d)-> (%d,%d)", \
		shtrack->gridc[sn].x + (shtrack->shsize.x/2), shtrack->gridc[sn].y + (shtrack->shsize.y/2),  \
		shtrack->subc[sn].x + (shtrack->shsize.x/2) + \
		gsl_vector_float_get(shtrack->disp, sn*2+0),\
		shtrack->subc[sn].y + (shtrack->shsize.y/2) + \
		gsl_vector_float_get(shtrack->disp, sn*2+1));
	for (sn=0; sn < shtrack->nsubap; sn++) {
		glVertex2f(shtrack->gridc[sn].x + (shtrack->shsize.x/2), shtrack->gridc[sn].y + (shtrack->shsize.y/2));
		glVertex2f(shtrack->subc[sn].x + (shtrack->track.x/2) + \
		gsl_vector_float_get(shtrack->disp, sn*2+0),\
		shtrack->subc[sn].y + (shtrack->track.y/2) + \
		gsl_vector_float_get(shtrack->disp, sn*2+1));
	}
	glEnd();

	return EXIT_SUCCESS;	
}

int displayGrid(coord_t gridres, mod_display_t *disp) {
	int j;
	glBegin(GL_LINES);
	// color already set at init, unecessary if no change
	//glColor3f(disp->col.r, disp->col.g, disp->col.b);
	//glColor3f(0.0, 1.0, 0.0);
	for (j=1; j < gridres.x; j++) {
		glVertex2f(j*disp->res.x/gridres.x, 0);
		glVertex2f(j*disp->res.x/gridres.x, disp->res.y);
	}
	for (j=1; j < gridres.y; j++) {
		glVertex2f(0, j*disp->res.y/gridres.y);
		glVertex2f(disp->res.x, j*disp->res.y/gridres.y);
	}
	
	glEnd();
	
	return EXIT_SUCCESS;
}

#endif

#ifdef FOAM_MODULES_DISLAY_SHSUPPORT
int displayDraw(wfs_t *wfsinfo, mod_display_t *disp, mod_sh_track_t *shtrack) {
#else
int displayDraw(wfs_t *wfsinfo, mod_display_t *disp) {
#endif
	
	displayBeginDraw(disp);
	
    if (disp->dispsrc == DISPSRC_RAW) {
		if (wfsinfo->bpp == 8) {
			uint8_t *imgc = (uint8_t *) wfsinfo->image;
			displayImgByte(imgc, disp, shtrack);
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
	    //logDebug(LOG_NOFORMAT, "disp dark ");
		displayGSLImg(wfsinfo->darkim, disp, 1);
	}
	else if (disp->dispsrc == DISPSRC_FLAT) {
	    //logDebug(LOG_NOFORMAT, "disp flat ");
		displayGSLImg(wfsinfo->flatim, disp, 1);
	}
	else if (disp->dispsrc == DISPSRC_FULLCALIB) {
	    //logDebug(LOG_NOFORMAT, "disp calib ");
		displayGSLImg(wfsinfo->corrim, disp, 1);
	}
	else if (disp->dispsrc == DISPSRC_FASTCALIB) {
	    //logDebug(LOG_NOFORMAT, "disp calib ");
		uint8_t *imgc = (uint8_t *) wfsinfo->corr;
		displayImgByte(imgc, disp, shtrack);
	}
	
#ifdef FOAM_MODULES_DISLAY_SHSUPPORT
	// display overlays (grid, subapts, vectors)
	if (disp->dispover & DISPOVERLAY_GRID) 
		displayGrid(shtrack->cells, disp);
	if (disp->dispover & DISPOVERLAY_SUBAPS)
		displaySubapts(shtrack, disp);
	if (disp->dispover & DISPOVERLAY_VECTORS) 
		displayVecs(shtrack, disp);
#endif
	
	displayFinishDraw(disp);
	
	return EXIT_FAILURE;
}
