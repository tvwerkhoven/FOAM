/*
 Copyright (C) 2008 Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)
 
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

 $Id$
 */
/*! 
	@file foam_modules-itifg.h
	@author Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)
	@date 2008-07-15

	@brief This file contains prototypes to read out a PCDIG framegrabber using the ITIFG driver.
*/

#ifndef FOAM_MODULES_ITIFG
#define FOAM_MODULES_ITIFG

// DEFINES //
/***********/

#define FOAM_MODITIFG_MAXFD 1024	//!< Maximum FD that select() polls

// LIBRARIES //
/*************/

//#include "foam_modules-itifg.h"

#ifdef FOAM_MODITIFG_ALONE
#define FOAM_DEBUG 1
// used for writing the frames to disk
#include "foam_modules-img.h"
// used for displaying stuff
#include "foam_modules-dispcommon.h"
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

#ifndef FOAM_MODITIFG_ALONE
// itifg does not like timeval if built standalone...
#include <sys/time.h>	// for timeval
#endif

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

// DATATYPES //
/*************/

/*! @brief Struct which holds some data to initialize ITIFG cameras with
 
 To initialize a framegrabber board using itifg, some info is needed. 
 Additional info given by the driver will again be stored in the struct.
 To initialize a board, the fields prefixed with '(user)' should already be
 filled in. After initialization, the '(foam)' fields will also be filled.
 */
typedef struct {
	short width;			//!< (foam) CCD width
	short height;			//!< (foam) CCD height
	int depth;				//!< (foam) CCD depth (i.e. 8bit)
	int fd;					//!< (foam) FD to the framegrabber
    
	size_t pagedsize;		//!< (foam) size of the complete frame + some metadata
	size_t rawsize;			//!< (foam) size of the raw frame (width*height*depth)
    
	union iti_cam_t itcam;	//!< (foam) see iti_cam_t (itifg driver)
	int module;				//!< (user) module used, 48 in mcmath setup
    
	char *device_name;		//!< (user) something like '/dev/ic0dma'
	char *config_file;		//!< (user) something like '../conffiles/dalsa-cad6.cam'
	char camera_name[128];		//!< (foam) camera name, as stored in the configuration file
	char exo_name[128];			//!< (foam) exo filename, as stored in the configuration file
} mod_itifg_cam_t;

/*!
 @brief Stores data on itifg camera buffer
 
 This struct stores some variables which makes it easier to
 work with the buffer used by the itifg driver. It should be initialized
 with only 'frames' holding a value, this will be the length of the buffer.
 Again, '(user)' is to be given by the user, '(foam)' will be filled in automatically.
 */
typedef struct {
	int frames;				//!< (user) how many frames should the buffer hold?
	iti_info_t *info;		//!< (foam) information on the current frame (not available in itifg-8.4.0)
	void *data;				//!< (foam) location of the current frame
	void *map;				//!< (foam) location of the mmap()'ed memory
} mod_itifg_buf_t;

// PROTOTYPES //
/**************/

/*!
 @brief Initialize a framegrabber board supported by ITIFG.
 
 This function requires a mod_itifg_cam_t struct which holds at least 
 device_name, config_file and module. The rest will be filled in by this
 function and is used in subsequent framegrabber calls.
 
 @param [in] *cam A struct with at least device_name, config_file and module.
 @return EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
 */
int itifgInitBoard(mod_itifg_cam_t *cam);

/*!
 @brief Initialize buffers for a framegrabber board which will hold the image data.
 
 This function requires a previously initialized mod_itifg_cam_t struct filled
 by itifgInitBoard(), and a mod_itifg_buf_t struct which holds only 'frames'. The 
 buffer will hold this many frames.
 
 @param [in] *cam Struct previously filled by a itifgInitBoard() call.
 @param [in] *buf Struct with only member .frames set indicating the size of the buffer
 @return EXIT_SUCCESS on success, EXIT_FAILURE otherwise. 
 */
int itifgInitBufs(mod_itifg_buf_t *buf, mod_itifg_cam_t *cam);

/*!
 @brief Start the actual framegrabbing

 This function starts the framegrabbing for *cam.
 
 Starting and stopping the framegrabber can be done multiple times without problems. If
 no frames are necessary for example, grabbing can be (temporarily) stopped by itifgStopGrab()
 and later resumed with this function.
 
 @param [in] *cam Struct previously filled by a itifgInitBoard() call.
 @return EXIT_SUCCESS on success, EXIT_FAILURE otherwise. 
 */
int itifgInitGrab(mod_itifg_cam_t *cam);

/*!
 @brief Stop framegrabbing
  
 This function stops the framegrabbing for *cam.
 
 Also see: itifgInitGrab()
 
 @param [in] *cam Struct previously filled by a itifgInitBoard() call.
 @return EXIT_SUCCESS on success, EXIT_FAILURE otherwise. 
 */
int itifgStopGrab(mod_itifg_cam_t *cam);

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
 
 @param [in] *buf Struct previously filled by a itifgInitBufs() call.
 @param [in] *cam Struct previously filled by a itifgInitBoard() call.
 @param [in] *timeout Timeout used with the select() call. Use NULL to disable
 @param [out] **newdata Pass the address of a void pointer here and that pointer will point to the new data upon succesful completion, can ben NULL.
 @return EXIT_SUCCESS on new frame or timeout, EXIT_FAILURE otherwise. 
 */
int itifgGetImg(mod_itifg_cam_t *cam, mod_itifg_buf_t *buf, struct timeval *timeout, void **newdata);

/*!
 @brief Stops a framegrabber board
 
 This function stops the framegrabber that was started previously by itifgInitBoard().
 
 @param [in] *cam Struct previously filled by a itifgInitBoard() call.
 @return EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
 */
int itifgStopBoard(mod_itifg_cam_t *cam);

/*!
 @brief Closes and frees buffers for a framegrabber board
 
 This function closes buffers used an frees the memory associated with it. 
 @param [in] *cam Struct previously filled by a itifgInitBoard() call.
 @param [in] *buf Struct previously filled by a itifgInitBufs() call.
 @return EXIT_SUCCESS on success, EXIT_FAILURE otherwise. 
 */
int itifgStopBufs(mod_itifg_buf_t *buf, mod_itifg_cam_t *cam);

#endif //#ifdef FOAM_MODULES_ITIFG
