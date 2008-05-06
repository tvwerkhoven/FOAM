/*! 
	@file foam_modules-itifg.h
	@author @authortim
	@date May 05, 2008

	@brief This file contains prototypes to read out a PCDIG framegrabber using the ITIFG driver.

	\section Info
 
	More documentation at the end of the source file (Hints for getting itifg running by Matthias Stein)
 
	This module compiles on its own, but needs some dependencies\n
	<tt>
	gcc -pipe -Wall -Wextra -g -DDEBUG_ITIFG=255    \
	-Iitifg/include  \
	-Litifg/lib  \
	-Ifoam/trunk/code/src/ \
	-o itifg-test foam/trunk/code/src/foam_cs_library.c foam/trunk/code/src/foam_modules-img.c foam/trunk/code/src/foam_modules-itifg.c \
	-litifg -lm -lc -g \
	-lgd -lpng -lSDL_image `sdl-config --libs --cflags`
	</tt>
	
	\section Functions
	
	Functions provided by this module are listed below. Typical usage would be to use the functions from top
	to bottom consecutively.
 
	\li drvInitBoard() - (1) Initialize a framegrabber board
	\li drvInitBufs() - (2) Initialize buffers and memory for use with a framegrabber board
	\li drvInitGrab() - (i>2) Start grabbing frames
	\li drvGetImg() - (s>g>i) Grab the next available image
	\li drvStopGrab() - (n-1>s>g) Stop grabbing frames
	\li drvStopBufs() - (n-1) Release the memory used by the buffers
	\li drvStopBoard() - (n) Stop the board and cleanup
 
	\section Dependencies
 
	This module depends on the itifg module version 8.4.0-0 or higher. This open source driver is used
	to access a variety of framegrabbers, including the PC-DIG board used here.

	\section History
 
    \li 2008-04-25 Code improved after discussing some things with the author of itifg, Matthias Stein 
 (see http://sourceforge.net/mailarchive/forum.php?forum_name=itifg-tech for details)
 
	\li 2008-04-14 api improved, now works with variables instead of defines
	This file is partially based on itifg.cc, part of filter_control by Guus Sliepen <guus@sliepen.eu.org>
	which was released under the GPL version 2.
 
	\section Todo
 
	\li Automatically read the module number during initialization using iti_parse_info
*/

#ifdef FOAM_MODITIFG_ALONE
#define FOAM_MODITIFG_DEBUG 1
#endif

#define FOAM_MODITIFG_MAXFD 1024	//!< Maximum FD that select() polls

//#include "foam_modules-itifg.h"

#ifdef FOAM_MODITIFG_ALONE
// used for writing the frames to disk
#include "foam_modules-img.h"
// necessary for coord_t
#include "foam_cs_library.h"
#endif


#include <stdio.h>	// for stuff (asprintf)
#include <stdlib.h> // more stuff
#include <unistd.h> // for close()
#include <string.h>	// for strerror (itifgExt.h)
#include <errno.h>	// for errno
#include <poll.h>	// for poll()
#include <fcntl.h>	// for O_RDWR, open()
#include <time.h>	// for itifgExt.h
#include <limits.h>	// for LONG_MAX
#include <math.h>	// ?

#include <sys/ioctl.h>	// for ioctl()
#include <sys/mman.h>	// for mmap()

#include <sys/time.h>	// for timeval

// Old stuff from test_itifg begins here:
//#include <signal.h> // ?
//#include <setjmp.h> // ?

//#include <termios.h> // ?
//#include <sys/param.h> // ?
//#include <sys/stat.h> // ?
//#include <sys/ipc.h> // ?
//#include <sys/shm.h> // ?
//#include <sys/poll.h> // ?
//#include <sys/wait.h> // ?

// #include <X11/Xos.h>
// #include <X11/Xlib.h>
// #include <X11/Xutil.h>
// #include <X11/Xatom.h>
// 
// #include <X11/extensions/XShm.h>

// these are for the itifg calls:
#include "itifgExt.h"
//#include "amvsReg.h"
//#include "amdigReg.h"
//#include "ampcvReg.h"
//#include "amcmpReg.h"
#include "pcdigReg.h"
#include "libitifg.h"

/*! @brief Struct which holds some data to initialize ITIFG cameras with
 
 To initialize a framegrabber board using itifg, some info is needed. 
 Additional info given by the driver will again be stored in the struct.
 To initialize a board, the fields prefixed with '(user)' should already be
 filled in. After initialization, the '(mod)' fields will also be filled.
 */
typedef struct {
	short width;			//!< (mod) CCD width
	short height;			//!< (mod) CCD height
	int depth;				//!< (mod) CCD depth (i.e. 8bit)
	int fd;					//!< (mod) FD to the framegrabber
	size_t pagedsize;		//!< (mod) size of the complete frame + some metadata
	size_t rawsize;			//!< (mod) size of the raw frame (width*height*depth)
	union iti_cam_t itcam;	//!< (mod) see iti_cam_t (itifg driver)
	int module;				//!< (user) module used, 48 in mcmath setup
	char *device_name;		//!< (user) something like '/dev/ic0dma'
	char *config_file;		//!< (user) something like '../conffiles/dalsa-cad6.cam'
	char *camera_name;		//!< (mod) camera name, as stored in the configuration file
	char *exo_name;			//!< (mod) exo filename, as stored in the configuration file
} mod_itifg_cam_t;

/*!
 @brief Stores data on itifg camera buffer
 
 This struct stores some variables which makes it easier to
 work with the buffer used by the itifg driver. It should be initialized
 with only 'frames' holding a value, this will be the length of the buffer.
 Again, '(user)' is to be given by the user, '(mod)' will be filled in automatically.
 */
typedef struct {
	int frames;				//!< (user) how many frames should the buffer hold?
	iti_info_t *info;		//!< (mod) information on the current frame
	void *data;				//!< (mod) location of the current frame
	void *map;				//!< (mod) location of the mmap()'ed memory
} mod_itifg_buf_t;

/*!
 @brief Initialize a framegrabber board
 
 This function requires a mod_itifg_cam_t struct which holds at least 
 device_name, config_file and module. The rest will be filled in by this
 function and is used in subsequent framegrabber calls.
 
 @param [in] *cam A struct with at least device_name, config_file and module.
 @return EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
 */
int drvInitBoard(mod_itifg_cam_t *cam);

/*!
 @brief Initialize buffers for a framegrabber board
 
 This function requires a previously initialized mod_itifg_cam_t struct filled
 by drvInitBoard(), and a mod_itifg_buf_t struct which holds only 'frames'. The 
 buffer will hold this many frames.
 
 @param [in] *cam Struct previously filled by a drvInitBoard() call.
 @param [in] *buf Struct with only member .frames set indicating the size of the buffer
 @return EXIT_SUCCESS on success, EXIT_FAILURE otherwise. 
 */
int drvInitBufs(mod_itifg_buf_t *buf, mod_itifg_cam_t *cam);

/*!
 @brief Start the actual framegrabbing

 This function starts the framegrabbing for *cam.
 
 Starting and stopping the framegrabber can be done multiple times without problems. If
 no frames are necessary for example, grabbing can be (temporarily) stopped by drvStopGrab()
 and later resumed with this function.
 
 @param [in] *cam Struct previously filled by a drvInitBoard() call.
 @return EXIT_SUCCESS on success, EXIT_FAILURE otherwise. 
 */
int drvInitGrab(mod_itifg_cam_t *cam);

/*!
 @brief Stop framegrabbing
  
 This function stops the framegrabbing for *cam.
 
 See also drvInitGrab()
 
 @param [in] *cam Struct previously filled by a drvInitBoard() call.
 @return EXIT_SUCCESS on success, EXIT_FAILURE otherwise. 
 */
int drvStopGrab(mod_itifg_cam_t *cam);

/*!
 @brief Get the next available image from the camera
 
 This function waits until the next full frame is avalaible using select() (i.e.
 waiting does not take up CPU time). After this function returns without errors,
 buf->data points to the newest frame and buf->info points to the metadata on this frame 
 (buf->info currently does not work, 2008-04-14). buf->data needs to be casted to
 a suitable datatype before usage. This depends on the specific hardware configuration
 and can usually be deduced from buf->depth which gives the bits per pixel.
 
 Additionally, a timeout can be given which is used in the select() call. If a 
 timeout occurs, this function returns with EXIT_SUCCESS.
 
 @param [in] *buf Struct previously filled by a drvInitBufs() call.
 @param [in] *cam Struct previously filled by a drvInitBoard() call.
 @param [in] *timeout Timeout used with the select() call. Use NULL to disable
 @return EXIT_SUCCESS on new frame or timeout, EXIT_FAILURE otherwise. 
 */
int drvGetImg(mod_itifg_cam_t *cam, mod_itifg_buf_t *buf, struct timeval *timeout);

/*!
 @brief Stops a framegrabber board
 
 This function stops the framegrabber that was started previously by drvInitBoard().
 
 @param [in] *cam Struct previously filled by a drvInitBoard() call.
 @return EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
 */
int drvStopBoard(mod_itifg_cam_t *cam);

/*!
 @brief Closes and frees buffers for a framegrabber board
 
 This function closes buffers used an frees the memory associated with it. 
 @param [in] *cam Struct previously filled by a drvInitBoard() call.
 @param [in] *buf Struct previously filled by a drvInitBufs() call.
 @return EXIT_SUCCESS on success, EXIT_FAILURE otherwise. 
 */
int drvStopBufs(mod_itifg_buf_t *buf, mod_itifg_cam_t *cam);

