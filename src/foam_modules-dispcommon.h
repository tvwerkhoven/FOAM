/*
 Copyright (C) 2008 Tim van Werkhoven
 
 This file is part of FOAM.
 
 FOAM is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 FOAM is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with FOAM.  If not, see <http://www.gnu.org/licenses/>.
 */
/*! 
 @file foam_modules-display.h
 @author @authortim
 @date 2008-03-12
 
 @brief This is the headerfile for graphics routines	
 */

#ifndef FOAM_MODULES_DISPLAY
#define FOAM_MODULES_DISPLAY

// LIBRARIES //
/*************/

#include "foam_cs_library.h"
#include "SDL.h" 	// most portable way according to 
//http://www.libsdl.org/cgi/docwiki.cgi/FAQ_20Including_20SDL_20Headers

#ifdef FOAM_MODULES_DISLAY_SHSUPPORT
	// The user wants to have SH specific drawing routines, include sh header for datatypes
	#include "foam_modules-sh.h"
#endif //#ifdef FOAM_MODULES_DISLAY_SHSUPPORT

#ifdef FOAM_MODULES_DISPLAY_OPENGL
// The user wants  to use OpenGL with SDL, include appropriate libraries
#include "SDL_opengl.h"
#ifdef __APPLE__
    #warning "Apple is not fully supported, SDL is not well-implemented there"
    #include <OpenGL/glu.h>
    #include <OpenGL/glext.h>
    #include <GLUT/glut.h>
#else
    #include <GL/glu.h>
    #include <GL/glext.h>
    #include <GL/glx.h>
    #include <GL/glxext.h>
    #include <GL/glut.h>
    //#define glXGetProcAddress(x) (*glXGetProcAddressARB)((const GLubyte*)x)
#endif //__APPLE__

#endif //FOAM_MODULES_DISPLAY_OPENGL


// DATATYPES //
/*************/

/*!
 @brief This enum lists the available display sources used by displayDraw to decide what to display.
 */
typedef enum {
    DISPSRC_RAW,       //!< Display the raw uncorrect image from the camera
    DISPSRC_FULLCALIB,     //!< Display the full dark/flat field corrected image
    DISPSRC_FASTCALIB,     //!< Display the fast (partially) dark/flat field corrected image
    DISPSRC_DARK,      //!< Display the darkfield used (used during calibration)
    DISPSRC_FLAT       //!< Display the flatfield used (used during calibration)
} dispsrc_t;

/*!
 @brief Used to store RGB values of colors
 */
typedef struct {
	uint8_t r;	//!< Red component [0,255]
	uint8_t g;	//!< Green component [0,255]
	uint8_t b;	//!< Blue component [0,255]
} rgbcolor_t;

// We define the possible display overlays here
#ifdef FOAM_MODULES_DISLAY_SHSUPPORT
#define DISPOVERLAY_SUBAPS 0x1		//!< Display the subaperture tracker windows
#define DISPOVERLAY_GRID 0x2		//!< Display a grid overlay
#define DISPOVERLAY_VECTORS 0x4		//!< Display the displacement vectors
#define DISPOVERLAY_SUBAPLABELS 0x8	//!< Display labels with the subapts
#endif // FOAM_MODULES_DISLAY_SHSUPPORT

/*!
 @brief This struct stores some properties on how to handle the displaying.
 
 Mostly a wrapper for things like resolution, SDL_Surface pointer, captions etc.
 Also lets the user define whether or not to do brightness/contrast themselves.
 
 If autocontrast = 1, the drawing routines in this module analyse the next image
 to be displayed and configure the brightness and constrast in such a way it is
 displayed over the whole dynamic range (i.e. pixel intensities from 0 to 255 typically).
 The module will reset autocontrast to 0 and keep using the values found above.
 The user can also manually control this by changing contrast and brightness. 
 The pixel values will then be scaled as (<raw intensity> - brightness) * contrast.
 
 */
typedef struct {
	SDL_Surface *screen;		//!< (foam) SDL_Surface to use
	const SDL_VideoInfo* info;	//!< (foam) VideoInfo pointer to use (OpenGL, read only)	
	int bpp;					//!< (foam) The bpp of the display (not the source!)	

	char *caption;				//!< (user) Caption prefix for the SDL window
	coord_t res;				//!< (user) Resolution of the source image (i.e. ccd resolution)
	coord_t windowres;			//!< (foam) Resolution for the window (might be altered at runtime)
    dispsrc_t dispsrc;          //!< (user) The display source, see possibilities at dispsrc_t
    int dispover;				//!< (user) The overlays to display, see DISPOVERLAY_* defines
    rgbcolor_t col;				//!< (user) An RGB color triplet to use for display overlays (i.e. grid, subapts etc)
    int autocontrast;			//!< (user/runtime) 1 = foam sets contrast, 0 = user handles contrast
    float contrast;				//!< (user) Use this to scale the pixel intensities
    int brightness;				//!< (user) Use this to shift the pixel intensities
	Uint32 flags;				//!< (foam) Flags to use with SDL_SetVideoMode
} mod_display_t;

// ROUTINES //
/************/

/*!
 @brief This routine is used to initialize the display module (either SDL or SDL and OpenGL)
 
 Call this routine before calling any other routine in this module.
 
 @param [in] *disp A pointer to a prefilled mod_display_t struct
 */
int displayInit(mod_display_t *disp);

/*!
 @brief Clean up the display module (either SDL or OpenGL)
 
 Call this routine after you're done using this module.
 
 @param [in] *disp A pointer to a prefilled mod_display_t struct
 */
int displayFinish(mod_display_t *disp);

/*!
 @brief Process events in the SDL loop
 
 Call this to process events that might have occurred. All this processes at
 the moment is resize events.
 
 @param [in] *disp A pointer to a prefilled mod_display_t struct
 */
void displaySDLEvents(mod_display_t *disp);

/*!
 @brief This displays an image img stored as bytes.
 
 This displays an image stored in row-major format in *img on the screen
 *screen. The resolution of the image to be drawn must also be specified, and
 this must not be bigger than the resolution of the SDL_Surface. Do not forget
 to call displayBeginDraw()/displayFinishDraw().
 
 @param [in] *img pointer to the image to display
 @param [in] *disp pre-filled mod_display_t structure
 */
#ifdef FOAM_MODULES_DISLAY_SHSUPPORT
int displayImgByte(uint8_t *img, mod_display_t *disp, mod_sh_track_t *shtrack);
#else
int displayImgByte(uint8_t *img, mod_display_t *disp);
#endif

/*!
 @brief This displays an image stored as GSL matrix on the screen
 
 This converts a GSL image stored in  *gslimg to a byte image ready to
 be displayed on a screen. doscale can be set to 1 to scale the input floats
 over the whole [0,255] range. You usually want this.
 
 Do not forget to call displayBeginDraw()/displayFinishDraw().
 
 @param [in] *gslimg pointer to the gsl float matrix to be drawn
 @param [in] *disp pre-filled mod_display_t structure
 @param [in] doscale scale floats during conversion to bytes (1) or not (0)
 */
int displayGSLImg(gsl_matrix_float *gslimg, mod_display_t *disp, int doscale);

/*!
 @brief Display data on the screen
 
 This routine draws the things that are configured in the mod_display_t
 struct *disp. This routine does not need to be wrapped in displayBeginDraw()/
 displayFinishDraw().
 */
#ifdef FOAM_MODULES_DISLAY_SHSUPPORT
int displayDraw(wfs_t *wfsinfo, mod_display_t *disp, mod_sh_track_t *shtrack);
#else
int displayDraw(wfs_t *wfsinfo, mod_display_t *disp);
#endif

#ifdef FOAM_MODULES_DISLAY_SHSUPPORT
/*!
 @brief This draws all subapertures for a certain wfs
 
 Do not forget to call displayBeginDraw()/displayFinishDraw().
 
 @param [in] *shtrack mod_sh_track_t struct with info on the SH configuration
 @param [in] *disp initialize mod_display_t struct with display information
 */
int displaySubapts(mod_sh_track_t *shtrack, mod_display_t *disp);

/*!
 @brief This draws vectors from the center of the grid to the detected center of gravity
 
 Do not forget to call displayBeginDraw()/displayFinishDraw().
 
 @param [in] *shtrack mod_sh_track_t struct with info on the SH configuration
 @param [in] *disp initialize mod_display_t struct with display information
 */
int displayVecs(mod_sh_track_t *shtrack, mod_display_t *disp);

/*!
@brief Displays labels near the subapertures to identify them
*/
int displaySubaptLabels(mod_sh_track_t *shtrack, mod_display_t *disp); 

/*!
 @brief This draws a grid on the screen (useful for SH sensing)
 
 Do not forget to call displayBeginDraw()/displayFinishDraw().
 
 @param [in] gridres The grid resolution to draw on the screen (i.e. 8x8)
 @param [in] *screen SDL_Surface to draw on
 */
int displayGrid(coord_t gridres, mod_display_t *disp);

#endif

/*!
 @brief Init stuff before drawing, if necessary.
 
 This routine must be used before drawing a frame yourself when not using
 the displayDraw routine. If so, wrap the drawing routines you are using between
 displayBeginDraw() and displayFinishDraw().
 */
void displayBeginDraw(mod_display_t *disp);

/*!
 @brief Finish up some things after drawing, if necessary.
 
 This routine must be used after drawing a frame yourself when not using
 the displayDraw routine. If so, wrap the drawing routines you are using between
 displayBeginDraw and displayFinishDraw.
 */
void displayFinishDraw(mod_display_t *disp);

#endif /* FOAM_MODULES_DISPLAY */
