/*! 
 @file foam_modules-dispgl.h
 @author @authortim
 @date 2008-05-09
 
 @brief This is the headerfile for the opengl graphics routines	
 */

#ifndef FOAM_MODULES_DISPGL
#define FOAM_MODULES_DISPGL

#include "foam_cs_library.h"

#ifdef FOAM_MODULES_DISLAY_SHSUPPORT
// The user wants to have SH specific drawing routines, include sh header for datatypes
#include "foam_modules-sh.h"
#endif //#ifdef FOAM_MODULES_DISLAY_SHSUPPORT

#include "SDL.h" 	// most portable way according to 
					//http://www.libsdl.org/cgi/docwiki.cgi/FAQ_20Including_20SDL_20Headers
#include "SDL_opengl.h"

// DATATYPES //
/*************/

/*!
 @brief This enum lists the available display source
 */
typedef enum {
    DISPSRC_RAW,       //!< Display the raw uncorrect image from the camera
    DISPSRC_CALIB,     //!< Display the dark/flat field corrected image
    DISPSRC_DARK,      //!< Display the darkfield used (probably rarely used)
    DISPSRC_FLAT       //!< Display the flatfield used (probably rarely used)
} dispsrc_t;

// We define the possible display overlays here
#ifdef FOAM_MODULES_DISLAY_SHSUPPORT
#define DISPOVERLAY_SUBAPS 0x1
#define DISPOVERLAY_GRID 0x2
#define DISPOVERLAY_VECTORS 0x4
#endif // FOAM_MODULES_DISLAY_SHSUPPORT
    

/*!
 @brief This struct stores some properties on how to handle the displaying with opengl.
 
 Baiscally a wrapper for thins like resolution, SDL_Surface pointer, captions etc.
 Also lets the user define whether or not to do brightness/contrast themselves.
 If autocontrast = 1, the drawing routines in this module make sure that the whole
 displayscale is used (i.e. pixel intensities from 0 to 255 typically). If set to
 0, the user can control this by changing contrast and brightness. The pixel values
 will then be scaled as (<raw intensity> - brightness) * contrast.
 */
typedef struct {
	SDL_Surface *screen;		//!< (mod) SDL_Surface to use
	const SDL_VideoInfo* info	//!< (mod) VideoInfo pointer to use (read only)
	char *caption;				//!< (user) Caption for the SDL window
	coord_t res;				//!< (user) Resolution of the CCD used
	coord_t windowres;			//!< (user) Initial resolution for the window
	int bpp;					//!< (mod) The bpp of the display (not the source!)
	Uint32 flags;				//!< (user) Flags to use with SDL_SetVideoMode
    dispsrc_t dispsrc;          //!< (user) The display source, can be DISP_RAW, DISP_CALIB, DISP_DARK, DISP_FLAT
    int dispover;          //!< (user) The overlays to display, see DISPOVERLAY_* defines
    int autocontrast;			//!< (user/runtime) 1 = foam handles contrast, 0 = user handles contrast
    float contrast;               //!< (user) if autocontrast=0, use this to scale the pixel intensities
    int brightness;             //!< (user) if autocontrast=0, use this to shift the pixel intensities
} mod_display_t;

// ROUTINES //
/************/

/*!
 @brief This routine is used to initialize SDL with OpenGL
 
 Call this routine before calling any other routine in this module.
 
 @param [in] *disp A pointer to a prefilled mod_dispgl_t struct
 */
int dispglInit(mod_dispgl_t *dispgl);

/*!
 @brief This displays an image located at *img on the display.
 
 This displays an image stored in row-major format in *img in a window. 
 The resolution of the image to be drawn must be specified in *dispgl.
 this must not be bigger than the resolution of the SDL_Surface. Do not forget
 to call dispglBeginDraw()/dispglFinishDraw().
 
 @param [in] *img void pointer to the image
 @param [in] *disp pre-filled mod_dispgl_t structure
 @param [in] databpp The bitdepth of the image (8 for byte, 16 for float atm)
 */
int dispglShowImg(void *img, mod_dispgl_t *disp, int databpp);

#ifdef FOAM_MODULES_DISLAY_SHSUPPORT
/*!
 @brief This draws all subapertures for a certain wfs
 
 Do not forget to call dispglBeginDraw()/dispglFinishDraw().
 
 @param [in] *wfsinfo wfs_t struct with info on the current wfs
 */
int dispglDrawSubapts(mod_sh_track_t *shtrack);

/*!
 @brief This draws vectors from the center of the grid to the detected center of gravity
 
 Do not forget to call dispglBeginDraw()/dispglFinishDraw().
 
 @param [in] *wfsinfo wfs_t struct with info on the current wfs
 */
int dispglDrawVecs(mod_sh_track_t *shtrack); 

/*!
 @brief This draws a grid on the screen
 
 Note that there are two coordinate systems, the window resolution and the camera
 chip resolution. Since all AO'ing is done in the camera chip coordinate system,
 the display is set up in such a way that drawing is also done in that coordinate
 system, *even if the window is resized!*.
 
 If you have a 256x256 ccd chip and want to draw a 8x8 grid, call:
 <tt>dispglGrid(coord_t gridres = {.x=8, .y=8}, coord_t imgres = {.x=256, .y=256});</tt>
 
 Do not forget to call dispglBeginDraw()/dispglFinishDraw().
 
 @param [in] gridres The grid resolution to draw on the screen (i.e. 8x8)
 @param [in] imgres The resolution of the source image (*not* the window resolution)
 */
int dispglGrid(coord_t gridres, coord_t imgres);

/*!
 @brief Draw useful stuff on the screen
 
 This (higher level) routine draws the things on the screen that are configured in
 dispgl.dispsrc and dispgl.dispover. See mod_dispgl_t datatype for more information.
 You do *not* have to call dispglBeginDraw()/dispglFinishDraw() here, it is done for
 you.
 */
void dispglDrawStuff(wfs_t *wfsinfo, mod_dispgl_t *dispgl, mod_sh_track_t *shtrack);
#endif

/*
 @brief Handle SDL events that might occur
 
 This routine checks if any events have occured and tries to act accordingly.
 This routine takes care of resizing the window, among other things.
 
 @param [in] *dispgl Prefilled mod_dispgl_t struct that went through dispglInit().
 */
void dispglSDLEvents(mod_dispgl_t *dispgl);

/*!
 @brief Finish drawing, call this after actual drawing commands.
 */
void dispglFinishDraw();

/*!
 @brief Begin drawing, call this before actual drawing commands.
 */
void dispglBeginDraw();

#endif /* FOAM_MODULES_DISPLAY */
