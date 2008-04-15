/*! 
	@file foam_modules-itifg.c
	@author @authortim
	@date 2008-03-03 16:49

	@brief This file contains routines to read out a PCDIG framegrabber using the ITIFG driver.

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
 
	\li drvInitBoard() - Initialize a framegrabber board
	\li drvInitBufs() - Initialize buffers and memory for use with a framegrabber board
	\li drvInitGrab() - Start grabbing frames
	\li drvGetImg() - Grab the next available image
	\li drvStopGrab() - Stop grabbing frames
	\li drvStopBufs() - Release the memory used by the buffers
	\li drvStopBoard() - Stop the board and cleanup
 
	\section Dependencies
 
	This module depends on the itifg module version 8.4.0-0 or higher. This open source driver is used
	to access a variety of framegrabbers, including the PC-DIG board used here.

	\section History
 
	\li 2008-04-14 api improved, now works with variables instead of defines
	This file is partially based on itifg.cc, part of filter_control by Guus Sliepen <guus@sliepen.eu.org>
	which was released under the GPL version 2.
 
	\section Todo
 
	\li Automatically read the module number during initialization using iti_parse_info
*/

//#define FOAM_MODITIFG_ALONE 1
//#define FOAM_MODITIFG_DEBUG 1

#ifdef FOAM_MODITIFG_ALONE
#define FOAM_MODITIFG_DEBUG 1
#endif

#ifdef FOAM_MODITIFG_DEBUG
#define FOAM_MODITIFG_ERR printf
#else
#define FOAM_MODITIFG_ERR logDebug
#endif

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
	char device_name[512];	//!< (user) something like '/dev/ic0dma'
	char config_file[512];	//!< (user) something like '../conffiles/dalsa-cad6.cam'
	char camera_name[512];	//!< (mod) camera name, as stored in the configuration file
	char exo_name[512];		//!< (mod) exo filename, as stored in the configuration file
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
 
 @params [in] *cam A struct with at least device_name, config_file and module.
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

int drvInitBoard(mod_itifg_cam_t *cam) {
	// TvW: | O_SYNC | O_APPEND also used in test_itifg.c
	int flags = O_RDWR;
	int zero = 0;
	int one = 1;
	int result;	
		
	cam->fd = open(cam->device_name, flags);
	if (cam->fd == -1) {
		FOAM_MODITIFG_ERR("Error opening device %s: %s\n", cam->device_name, strerror(errno));
		return EXIT_FAILURE;
	}

// ???:tim:20080408 might not be necessary?
//	if (ioctl(cam->fd, GIOC_SET_LUT_LIN) < 0) {
//		close(cam->fd);
//		FOAM_MODITIFG_ERR("%s: error linearising LUTs: %s\n", cam->device_name, strerror(errno));
//		return EXIT_FAILURE;
//	}
	
	if (ioctl(cam->fd, GIOC_SET_DEFCNF, NULL) < 0) {
		close(cam->fd);
		FOAM_MODITIFG_ERR("%s: error setting camera configuration: %s\n", cam->device_name, strerror(errno));
		return EXIT_FAILURE;
	}	
	
	if (ioctl(cam->fd, GIOC_SET_CAMERA, &zero) < 0) {
		close(cam->fd);
		FOAM_MODITIFG_ERR("%s: error setting camera: %s\n", cam->device_name, strerror(errno));
		return EXIT_FAILURE;
	}
	//union iti_cam_t tmpcam;
	
	if (ioctl(cam->fd, GIOC_GET_CAMCNF, &(cam->itcam)) < 0) {
		close(cam->fd);
		FOAM_MODITIFG_ERR("%s: error getting camera configuration: %s\n", cam->device_name, strerror(errno));
		return EXIT_FAILURE;
	}

	
//	if ((result = iti_read_config(cam->config_file, &(cam->itcam), 0, cam->module, 0, cam->camera_name, cam->exo_name)) < 0) {
	if ((result = iti_read_config(cam->config_file, &(cam->itcam), 0, cam->module, 0, cam->camera_name, cam->exo_name)) < 0) {
		close(cam->fd);
		FOAM_MODITIFG_ERR("%s: error reading camera configuration from file %s: %s\n", cam->device_name, cam->config_file, strerror(errno));
		return EXIT_FAILURE;		
	}
#ifdef FOAM_MODITIFG_DEBUG
	FOAM_MODITIFG_ERR("Read config '%s'. Camera: '%s', exo: '%s'\n", cam->config_file, cam->camera_name, cam->exo_name);
#endif
	
	if (ioctl(cam->fd, GIOC_SET_CAMCNF, &(cam->itcam)) < 0) {
		close(cam->fd);
		FOAM_MODITIFG_ERR("%s: error setting camera configuration: %s\n", cam->device_name, strerror(errno));
		return EXIT_FAILURE;
	}
	
//	if (ioctl(cam->fd, GIOC_SET_TIMEOUT, &(struct timeval){0, 0}) < 0) {
//		close(cam->fd);
//		FOAM_MODITIFG_ERR("%s: error setting timeout: %s\n", cam->device_name, strerror(errno));
//		return EXIT_FAILURE;
//	}
//#ifdef FOAM_MODITIFG_DEBUG
//	FOAM_MODITIFG_ERR("Timout set to {0,0}\n");
//#endif
	
	if(ioctl(cam->fd, GIOC_SET_HDEC, &one) < 0) {
		close(cam->fd);
		FOAM_MODITIFG_ERR("%s: error setting horizontal decimation: %s\n", cam->device_name, strerror(errno));
		return EXIT_FAILURE;
	}
	
	if(ioctl(cam->fd, GIOC_SET_VDEC, &one) < 0) {
		close(cam->fd);
		FOAM_MODITIFG_ERR("%s: error setting vertical decimation: %s\n", cam->device_name, strerror(errno));
		return EXIT_FAILURE;
	}
#ifdef FOAM_MODITIFG_DEBUG
	FOAM_MODITIFG_ERR("decimation set to {1,1} (i.e. we want full frames)\n");
#endif
	
	if(ioctl(cam->fd, GIOC_GET_WIDTH, &(cam->width)) < 0) {
		close(cam->fd);
		FOAM_MODITIFG_ERR("%s: error getting width: %s\n", cam->device_name, strerror(errno));
		return EXIT_FAILURE;
	}
	
	if(ioctl(cam->fd, GIOC_GET_HEIGHT, &(cam->height)) < 0) {
		close(cam->fd);
		FOAM_MODITIFG_ERR("%s: error getting height: %s\n", cam->device_name, strerror(errno));
		return EXIT_FAILURE;
	}
	
	if(ioctl(cam->fd, GIOC_GET_DEPTH, &(cam->depth)) < 0) {
		close(cam->fd);
		FOAM_MODITIFG_ERR("%s: error getting depth: %s\n", cam->device_name, strerror(errno));
		return EXIT_FAILURE;
	}
#ifdef FOAM_MODITIFG_DEBUG
	FOAM_MODITIFG_ERR("width x height x depth: %dx%dx%d\n", cam->width, cam->height, cam->depth);
#endif
	
	if(ioctl(cam->fd, GIOC_GET_RAWSIZE, &(cam->rawsize)) < 0) {
		close(cam->fd);
		FOAM_MODITIFG_ERR("%s: error getting raw size: %s\n", cam->device_name, strerror(errno));
		return EXIT_FAILURE;
	}
	
	if(ioctl(cam->fd, GIOC_GET_PAGEDSIZE, &(cam->pagedsize)) < 0) {
		close(cam->fd);
		FOAM_MODITIFG_ERR("%s: error getting paged size: %s\n", cam->device_name, strerror(errno));
		return EXIT_FAILURE;
	}
#ifdef FOAM_MODITIFG_DEBUG
	FOAM_MODITIFG_ERR("raw size: %d, paged size: %d\n", (int) cam->rawsize, (int) cam->pagedsize);
#endif
	
	if (fcntl(cam->fd, F_SETFL, fcntl(cam->fd, F_GETFL, NULL) & ~O_NONBLOCK) < 0) {
		close(cam->fd);
		FOAM_MODITIFG_ERR("%s: error setting blocking: %s\n", cam->device_name, strerror(errno));
		return EXIT_FAILURE;
	}

#ifdef FOAM_MODITIFG_DEBUG
	FOAM_MODITIFG_ERR("Camera configuration done.\n");
#endif
	
	return EXIT_SUCCESS;
}

int drvInitBufs(mod_itifg_buf_t *buf, mod_itifg_cam_t *cam) {
	
	// start mmap
	buf->map = mmap(NULL, cam->pagedsize * buf->frames, PROT_READ | PROT_WRITE, MAP_SHARED, cam->fd, 0);
	
	if (buf->map == (void *)-1) {
		FOAM_MODITIFG_ERR("Could not mmap(): %s\n", strerror(errno));
		return EXIT_FAILURE;
	}
	
	buf->data = buf->map;
	buf->info = (iti_info_t *)((char *)buf->data + cam->rawsize);

#ifdef FOAM_MODITIFG_DEBUG
	FOAM_MODITIFG_ERR("mmap() successful.\n");
#endif
	
	return EXIT_SUCCESS;
}

int drvInitGrab(mod_itifg_cam_t *cam) {
	// reset stats if possible
//	ioctl(cam->fd, GIOC_SET_STATS, NULL);
#ifdef FOAM_MODITIFG_DEBUG
	FOAM_MODITIFG_ERR("Starting grab, lseeking to %ld.\n", +LONG_MAX);
#endif
	
	// start the framegrabber by seeking a lot???
	if ( lseek(cam->fd, +LONG_MAX, SEEK_END) == -1) {
		FOAM_MODITIFG_ERR("Error starting grab: %s\n", strerror(errno));
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

int drvStopGrab(mod_itifg_cam_t *cam) {
#ifdef FOAM_MODITIFG_DEBUG
	FOAM_MODITIFG_ERR("Stopping grab, lseeking to %ld.\n", -LONG_MAX);
#endif
	// stop the framegrabber by seeking a lot???
	if ( lseek(cam->fd, -LONG_MAX, SEEK_END) == -1) {
		FOAM_MODITIFG_ERR("Error stopping grab: %s\n", strerror(errno));
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

int drvGetImg(mod_itifg_cam_t *cam, mod_itifg_buf_t *buf, struct timeval *timeout) {
	off_t seek;
	int result;
//	struct iti_acc_t acc;
	//struct pollfd pfd = {cam->fd, POLLIN, 0};
#ifdef FOAM_MODITIFG_DEBUG
	FOAM_MODITIFG_ERR("Grabbing image...\n");	
#endif
	fd_set in_fdset, ex_fdset;
	FD_ZERO (&in_fdset);
	FD_ZERO (&ex_fdset);
	FD_SET (cam->fd, &in_fdset);
	FD_SET (cam->fd, &ex_fdset);

	//result = poll(&pfd, 1, timeout);
	result = select(1024, &in_fdset, NULL, &ex_fdset, timeout);
	
	if (result == -1)  {
		FOAM_MODITIFG_ERR("Select() returned no active FD's, error:%s\n", strerror(errno));
		return EXIT_FAILURE;
	}
	if (result == 0) {
		// timeout occured, return immediately
#ifdef FOAM_MODITIFG_DEBUG
		FOAM_MODITIFG_ERR("Timeout in drvGetImg().\n");	
#endif
		return EXIT_SUCCESS;
	}

#ifdef FOAM_MODITIFG_DEBUG
	FOAM_MODITIFG_ERR("lseek 0 SEEK_END...\n");	
#endif
	seek = lseek(cam->fd, 0, SEEK_END);
	if (seek == -1) {
		FOAM_MODITIFG_ERR("SEEK_END failed: %s\n", strerror(errno));
		return EXIT_FAILURE;
	}

#ifdef FOAM_MODITIFG_DEBUG
	FOAM_MODITIFG_ERR("Select returned: %d, SEEK_END: %d\n", result, (int) seek);	
#endif
//	if (ioctl(cam->fd, GIOC_GET_STATS, &acc) < 0) {
//		FOAM_MODITIFG_ERR("Could not read framegrabber statistics");
//		return EXIT_FAILURE;
//	}
	
//	buf->data = (void *)((char *)buf->map + ((int) seek % (buf->frames * cam->pagedsize)));
//	buf->data = (void *)((char *)buf->map + ((1 - 1) % buf->frames) * cam->pagedsize);
	buf->data = (void *)((char *)buf->map);
	buf->info = (iti_info_t *)((char *)buf->data + cam->rawsize);

	struct timeval timestamp;
	memcpy (&timestamp, &(buf->info->acq.timestamp), sizeof(struct timeval));

#ifdef FOAM_MODITIFG_DEBUG
	FOAM_MODITIFG_ERR("acq.captured %d acq.lost %d, acq.timestamp.tv_sec: %d\n", buf->info->acq.captured, buf->info->acq.lost, (int) buf->info->acq.timestamp.tv_sec);
#endif

#ifdef FOAM_MODITIFG_DEBUG
	FOAM_MODITIFG_ERR("lseek paged_size SEEK_CUR...\n");	
#endif
	seek = lseek(cam->fd, cam->pagedsize, SEEK_CUR);
	if (seek == -1) {
		FOAM_MODITIFG_ERR("SEEK_CUR failed: %s\n", strerror(errno));
		return EXIT_FAILURE;
	}	
#ifdef FOAM_MODITIFG_DEBUG
	FOAM_MODITIFG_ERR("Select returned: %d, SEEK_CUR: %d\n", result, (int) seek);	
#endif
//	iti_info_t *imageinfo;
//	imageinfo = (iti_info_t *)((char *)buf->data + cam->rawsize);
//	FOAM_MODITIFG_ERR("Captured %d frames\n", imageinfo->acq.captured);

	// TvW: hoes does this work, exactly?
//	if (acc.transfered != info->framenums.transfered) {
//		FOAM_MODITIFG_ERR("Frame %lu not in right place in mmap area (%lu is in its spot)", acc.transfered, info->framenums.transfered);
//		return EXIT_FAILURE;
//	}
	
	return EXIT_SUCCESS;
}

int drvStopBufs(mod_itifg_buf_t *buf, mod_itifg_cam_t *cam) {
	if (munmap(buf->map, cam->pagedsize * buf->frames) == -1) {
		FOAM_MODITIFG_ERR("Error unmapping camera memory: %s\n", strerror(errno));
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

int drvStopBoard(mod_itifg_cam_t *cam) {
	if (close(cam->fd) == -1) {
		FOAM_MODITIFG_ERR("Unable to close framegrabber board fd: %s\n", strerror(errno));
		return EXIT_FAILURE;
	}
	
	return EXIT_SUCCESS;
}

#ifdef FOAM_MODITIFG_ALONE
int main(int argc, char *argv[]) {
	// init vars
	int i, j;
	mod_itifg_cam_t camera;
	mod_itifg_buf_t buffer;
	char *file;
	
	camera.module = 48; // some number taken from test_itifg
	strncpy(camera.device_name, "/dev/ic0dma", 512-1);
	strncpy(camera.config_file, "../config/dalsa-cad6-pcd.cam", 512-1);

	buffer.frames = 4; // ringbuffer will be 4 frames big
	
	printf("This is the debug version for ITIFG8\n");
	printf("Trying to do something with '%s' using '%s'\n", camera.device_name, camera.config_file);
	
	// init cam
	if (drvInitBoard(&camera) != EXIT_SUCCESS)
		return EXIT_FAILURE;
	
	// init bufs
	if (drvInitBufs(&buffer, &camera) != EXIT_SUCCESS)
		return EXIT_FAILURE;

	// init grab
	drvInitGrab(&camera);

	coord_t res;
	res.x = (int) camera.width;
	res.y = (int) camera.height;
	
	// test image
	for (i=0; i<10; i++) {
		if (drvGetImg(&camera, &buffer, NULL) != EXIT_SUCCESS)
			return EXIT_FAILURE;
		
		printf("Frames grabbed: %d\n", buffer.info->acq.captured);
		//printf("Pixels 1 through 100:\n");
		for (j=0; j<10; j++)
			printf("%d,", *( ((unsigned char *) (buffer.data)) + j) );
		
		printf("\n");
		asprintf(&file, "itifg-debug-cap-%d.png",  i);
		//printf("Writing frame to disk (%s)\n", file);

		//modWritePNGArr(file, ((void *) (buffer.data)), res, 1);
	}
	
	printf("cleaning up now\n");
	
	// cleanup
	drvStopGrab(&camera);
	drvStopBufs(&buffer, &camera);
	
	// end
	printf("exit\n");
	
	return 0;
}
#endif


