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

#ifdef FOAM_MODULES_DISLAY_SHSUPPORT
	// The user wants to have SH specific drawing routines, include sh header for datatypes
	#include "foam_modules-sh.h"
#endif //#ifdef FOAM_MODULES_DISLAY_SHSUPPORT

#ifdef FOAM_MODULES_DISPLAY_OPENGL
// The user wants  to use OpenGL with SDL, include appropriate libraries
#include "SDL_opengl.h"
#ifdef __APPLE__
    #include <OpenGL/glu.h>
    #include <OpenGL/glext.h>
#else
    #include <GL/glu.h>
    #include <GL/glext.h>
    #include <GL/glx.h>
    #include <GL/glxext.h>
    //#define glXGetProcAddress(x) (*glXGetProcAddressARB)((const GLubyte*)x)
#endif

#endif


// DATATYPES //
/*************/

/*!
 @brief This enum lists the available display sources
 */
typedef enum {
    DISPSRC_RAW,       //!< Display the raw uncorrect image from the camera
    DISPSRC_CALIB,     //!< Display the dark/flat field corrected image
    DISPSRC_DARK,      //!< Display the darkfield used (probably rarely used)
    DISPSRC_FLAT       //!< Display the flatfield used (probably rarely used)
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
#endif // FOAM_MODULES_DISLAY_SHSUPPORT

/*!
 @brief This struct stores some properties on how to handle the displaying.
 
 Basically a wrapper for thins like resolution, SDL_Surface pointer, captions etc.
 Also lets the user define whether or not to do brightness/contrast themselves.
 If autocontrast = 1, the drawing routines in this module make sure that the whole
 displayscale is used (i.e. pixel intensities from 0 to 255 typically). If set to
 0, the user can control this by changing contrast and brightness. The pixel values
 will then be scaled as (<raw intensity> - brightness) * contrast.
 */
typedef struct {
	SDL_Surface *screen;		//!< (mod) SDL_Surface to use
	const SDL_VideoInfo* info;	//!< (mod) VideoInfo pointer to use (OpenGL, read only)	
	int bpp;					//!< (mod) The bpp of the display (not the source!)	

	char *caption;				//!< (user) Caption for the SDL window
	coord_t res;				//!< (user) Resolution of the source image (i.e. ccd resolution)
	coord_t windowres;			//!< (mod) Resolution for the window (might be altered at runtime)
    dispsrc_t dispsrc;          //!< (user) The display source, see possibilities at dispsrc_t
    int dispover;				//!< (user) The overlays to display, see DISPOVERLAY_* defines
    rgbcolor_t col;				//!< (user) An RGB color triplet to use for display overlays (i.e. grid, subapts etc)
    int autocontrast;			//!< (user/runtime) 1 = foam handles contrast, 0 = user handles contrast
    float contrast;				//!< (user) if autocontrast=0, use this to scale the pixel intensities
    int brightness;				//!< (user) if autocontrast=0, use this to shift the pixel intensities
	Uint32 flags;				//!< (mod) Flags to use with SDL_SetVideoMode
} mod_display_t;

// ROUTINES //
/************/

/*!
 @brief This routine is used to initialize the display module (either SDL or OpenGL)
 
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
 
 Call this to process events that might have occurred.
 
 @param [in] *disp A pointer to a prefilled mod_display_t struct
 */
void displaySDLEvents(mod_display_t *disp);

/*!
 @brief This displays an image img stored as bytes.
 
 This displays an image stored in row-major format in *img on the screen
 *screen. The resolution of the image to be drawn must also be specified, and
 this must not be bigger than the resolution of the SDL_Surface. Do not forget
 to call displayBeginDraw()/displayFinishDraw().
 
 @param [in] *wfsinfo pointer to the wfs to show the image for
 @param [in] *disp pre-filled mod_display_t structure
 */
int displayImgByte(uint8_t *img, mod_display_t *disp);

/*!
 @brief This displays a GSL matrix on the screen
 
 This converts a GSL image stored in  *gslimg to a byte image ready to
 be displayed on a screen. doscale can be used to apply scaling when
 converting floats to bytes or to directly cast it.
 Do not forget to call displayBeginDraw()/displayFinishDraw().
 
 @param [in] *gslimg pointer to the gsl float matrix to be drawn
 @param [in] *disp pre-filled mod_display_t structure
 @param [in] doscale scale floats during byte conversion (1) or not (0)
 */
int displayGSLImg(gsl_matrix_float *gslimg, mod_display_t *disp, int doscale);

/*!
 @brief Display data on the screen
 
 This routine draws the things that are configured in the mod_display_t
 struct *disp. This routine does not to be wrapped in displayBeginDraw()/
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
 @brief This draws a grid on the screen (useful for SH sensing)
 
 Do not forget to call displayBeginDraw()/displayFinishDraw().
 
 @param [in] gridres The grid resolution to draw on the screen (i.e. 8x8)
 @param [in] *screen SDL_Surface to draw on
 */
int displayGrid(coord_t gridres, mod_display_t *disp);

#endif

/*!
 @brief Finish drawing, if necessary
 
 This routine must be used before drawing a frame yourself when not using
 the displayDraw routine. If so, wrap the drawing routines you are using between
 displayBeginDraw and displayFinishDraw.
 */
void displayBeginDraw(mod_display_t *disp);

/*!
 @brief Init stuff before drawing, if necessary
 
 This routine must be used after drawing a frame yourself when not using
 the displayDraw routine. If so, wrap the drawing routines you are using between
 displayBeginDraw and displayFinishDraw.
 */
void displayFinishDraw(mod_display_t *disp);

#endif /* FOAM_MODULES_DISPLAY */
