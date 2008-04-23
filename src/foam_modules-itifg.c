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

int drvInitBoard(mod_itifg_cam_t *cam) {
	// TvW: | O_SYNC | O_APPEND also used in test_itifg.c
	int flags = O_RDWR | O_APPEND | O_SYNC | O_NONBLOCK;
	int zero = 0;
	int one = 1;
	int result;	
		
	cam->fd = open(cam->device_name, flags);
	if (cam->fd == -1) {
		logWarn("Error opening device %s: %s", cam->device_name, strerror(errno));
		return EXIT_FAILURE;
	}

// ???:tim:20080408 might not be necessary?
//	if (ioctl(cam->fd, GIOC_SET_LUT_LIN) < 0) {
//		close(cam->fd);
//		logWarn("%s: error linearising LUTs: %s", cam->device_name, strerror(errno));
//		return EXIT_FAILURE;
//	}
	
	if (ioctl(cam->fd, GIOC_SET_DEFCNF, NULL) < 0) {
		close(cam->fd);
		logWarn("%s: error setting camera configuration: %s", cam->device_name, strerror(errno));
		return EXIT_FAILURE;
	}	
	
	if (ioctl(cam->fd, GIOC_SET_CAMERA, &zero) < 0) {
		close(cam->fd);
		logWarn("%s: error setting camera: %s", cam->device_name, strerror(errno));
		return EXIT_FAILURE;
	}
	//union iti_cam_t tmpcam;
	
	if (ioctl(cam->fd, GIOC_GET_CAMCNF, &(cam->itcam)) < 0) {
		close(cam->fd);
		logWarn("%s: error getting camera configuration: %s", cam->device_name, strerror(errno));
		return EXIT_FAILURE;
	}

	
//	if ((result = iti_read_config(cam->config_file, &(cam->itcam), 0, cam->module, 0, cam->camera_name, cam->exo_name)) < 0) {
	if ((result = iti_read_config(cam->config_file, &(cam->itcam), 0, cam->module, 0, cam->camera_name, cam->exo_name)) < 0) {
		close(cam->fd);
		logWarn("%s: error reading camera configuration from file %s: %s", cam->device_name, cam->config_file, strerror(errno));
		return EXIT_FAILURE;		
	}
#ifdef FOAM_MODITIFG_DEBUG
	logDebug(0, "Read config '%s'. Camera: '%s', exo: '%s'", cam->config_file, cam->camera_name, cam->exo_name);
#endif
	
	if (ioctl(cam->fd, GIOC_SET_CAMCNF, &(cam->itcam)) < 0) {
		close(cam->fd);
		logWarn("%s: error setting camera configuration: %s", cam->device_name, strerror(errno));
		return EXIT_FAILURE;
	}
	
//	if (ioctl(cam->fd, GIOC_SET_TIMEOUT, &(struct timeval){0, 0}) < 0) {
//		close(cam->fd);
//		logWarn("%s: error setting timeout: %s", cam->device_name, strerror(errno));
//		return EXIT_FAILURE;
//	}
//#ifdef FOAM_MODITIFG_DEBUG
//	logDebug(0, "Timout set to {0,0}");
//#endif
	
	if(ioctl(cam->fd, GIOC_SET_HDEC, &one) < 0) {
		close(cam->fd);
		logWarn("%s: error setting horizontal decimation: %s", cam->device_name, strerror(errno));
		return EXIT_FAILURE;
	}
	
	if(ioctl(cam->fd, GIOC_SET_VDEC, &one) < 0) {
		close(cam->fd);
		logWarn("%s: error setting vertical decimation: %s", cam->device_name, strerror(errno));
		return EXIT_FAILURE;
	}
#ifdef FOAM_MODITIFG_DEBUG
	logDebug(0, "decimation set to {1,1} (i.e. we want full frames)");
#endif
	
	if(ioctl(cam->fd, GIOC_GET_WIDTH, &(cam->width)) < 0) {
		close(cam->fd);
		logWarn("%s: error getting width: %s", cam->device_name, strerror(errno));
		return EXIT_FAILURE;
	}
	
	if(ioctl(cam->fd, GIOC_GET_HEIGHT, &(cam->height)) < 0) {
		close(cam->fd);
		logWarn("%s: error getting height: %s", cam->device_name, strerror(errno));
		return EXIT_FAILURE;
	}
	
	if(ioctl(cam->fd, GIOC_GET_DEPTH, &(cam->depth)) < 0) {
		close(cam->fd);
		logWarn("%s: error getting depth: %s", cam->device_name, strerror(errno));
		return EXIT_FAILURE;
	}
#ifdef FOAM_MODITIFG_DEBUG
	logDebug(0, "width x height x depth: %dx%dx%d", cam->width, cam->height, cam->depth);
#endif
	
	if(ioctl(cam->fd, GIOC_GET_RAWSIZE, &(cam->rawsize)) < 0) {
		close(cam->fd);
		logWarn("%s: error getting raw size: %s", cam->device_name, strerror(errno));
		return EXIT_FAILURE;
	}
	
	if(ioctl(cam->fd, GIOC_GET_PAGEDSIZE, &(cam->pagedsize)) < 0) {
		close(cam->fd);
		logWarn("%s: error getting paged size: %s", cam->device_name, strerror(errno));
		return EXIT_FAILURE;
	}
#ifdef FOAM_MODITIFG_DEBUG
	logDebug(0, "raw size: %d, paged size: %d", (int) cam->rawsize, (int) cam->pagedsize);
#endif
	
	if (fcntl(cam->fd, F_SETFL, fcntl(cam->fd, F_GETFL, NULL) & ~O_NONBLOCK) < 0) {
		close(cam->fd);
		logWarn("%s: error setting blocking: %s", cam->device_name, strerror(errno));
		return EXIT_FAILURE;
	}

#ifdef FOAM_MODITIFG_DEBUG
	logDebug(0, "Camera configuration done.");
#endif
	
	return EXIT_SUCCESS;
}

int drvInitBufs(mod_itifg_buf_t *buf, mod_itifg_cam_t *cam) {
	
	// start mmap
	buf->map = mmap(NULL, cam->pagedsize * buf->frames, PROT_READ | PROT_WRITE, MAP_SHARED, cam->fd, 0);
	
	if (buf->map == (void *)-1) {
		logWarn("Could not mmap(): %s", strerror(errno));
		return EXIT_FAILURE;
	}
	
	buf->data = buf->map;
	buf->info = (iti_info_t *)((char *)buf->data + cam->rawsize);

#ifdef FOAM_MODITIFG_DEBUG
	logDebug(0, "mmap() successful.");
#endif
	
	return EXIT_SUCCESS;
}

int drvInitGrab(mod_itifg_cam_t *cam) {
	// reset stats if possible
//	ioctl(cam->fd, GIOC_SET_STATS, NULL);
#ifdef FOAM_MODITIFG_DEBUG
	logDebug(0, "Starting grab, lseeking to %ld.", +LONG_MAX);
#endif
	
	// start the framegrabber by seeking a lot???
	if ( lseek(cam->fd, +LONG_MAX, SEEK_END) == -1) {
		logWarn("Error starting grab: %s", strerror(errno));
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

int drvStopGrab(mod_itifg_cam_t *cam) {
#ifdef FOAM_MODITIFG_DEBUG
	logDebug(0, "Stopping grab, lseeking to %ld.", -LONG_MAX);
#endif
	// stop the framegrabber by seeking a lot???
	if ( lseek(cam->fd, -LONG_MAX, SEEK_END) == -1) {
		logWarn("Error stopping grab: %s", strerror(errno));
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
	logDebug(0, "Grabbing image...");	
#endif
	fd_set in_fdset, ex_fdset;
	FD_ZERO (&in_fdset);
	FD_ZERO (&ex_fdset);
	FD_SET (cam->fd, &in_fdset);
	FD_SET (cam->fd, &ex_fdset);

	//result = poll(&pfd, 1, timeout);
	result = select(FOAM_MODITIFG_MAXFD, &in_fdset, NULL, &ex_fdset, timeout);
	
	if (result == -1)  {
		logWarn("Select() returned no active FD's, error:%s", strerror(errno));
		return EXIT_FAILURE;
	}
	if (result == 0) {
		// timeout occured, return immediately
#ifdef FOAM_MODITIFG_DEBUG
		logWarn("Timeout in drvGetImg().");	
#endif
		return EXIT_SUCCESS;
	}

#ifdef FOAM_MODITIFG_DEBUG
	logDebug(0, "lseek 0 SEEK_END...");	
#endif
	seek = lseek(cam->fd, 0, SEEK_END);
	if (seek == -1) {
		logWarn("SEEK_END failed: %s", strerror(errno));
		return EXIT_FAILURE;
	}

#ifdef FOAM_MODITIFG_DEBUG
	logDebug(0, "Select returned: %d, SEEK_END: %d", result, (int) seek);	
#endif

//	buf->data = (void *)((char *)buf->map + ((int) seek % (buf->frames * cam->pagedsize)));
//	buf->data = (void *)((char *)buf->map + ((1 - 1) % buf->frames) * cam->pagedsize);
	buf->data = (void *)((char *)buf->map);
	buf->info = (iti_info_t *)((char *)buf->data + cam->rawsize);

//	struct timeval timestamp;
//	memcpy (&timestamp, &(buf->info->acq.timestamp), sizeof(struct timeval));

#ifdef FOAM_MODITIFG_DEBUG
	//logDebug(0, "acq.captured %d acq.lost %d, acq.timestamp.tv_sec: %d .tv_usec: %d", buf->info->acq.captured, buf->info->acq.lost, (int) buf->info->acq.timestamp.tv_sec, (int) buf->info->acq.timestamp.tv_usec);
	//logDebug(0, "dma.transfered %d ", buf->info->dma.transfered);
#endif

#ifdef FOAM_MODITIFG_DEBUG
	logDebug(0, "lseek %d SEEK_CUR...", cam->pagedsize);	
#endif
	seek = lseek(cam->fd, cam->pagedsize, SEEK_CUR);
	//seek = lseek(cam->fd, seek, SEEK_CUR);
	if (seek == -1) {
		logWarn("SEEK_CUR failed: %s", strerror(errno));
		return EXIT_FAILURE;
	}	
#ifdef FOAM_MODITIFG_DEBUG
	logDebug(0, "Select returned: %d, SEEK_CUR: %d", result, (int) seek);	
#endif
//	iti_info_t *imageinfo;
//	imageinfo = (iti_info_t *)((char *)buf->data + cam->rawsize);
//	logDebug(0, "Captured %d frames", imageinfo->acq.captured);

	// TvW: hoes does this work, exactly?
//	if (acc.transfered != info->framenums.transfered) {
//		logWarn("Frame %lu not in right place in mmap area (%lu is in its spot)", acc.transfered, info->framenums.transfered);
//		return EXIT_FAILURE;
//	}
	
	return EXIT_SUCCESS;
}

int drvStopBufs(mod_itifg_buf_t *buf, mod_itifg_cam_t *cam) {
	if (munmap(buf->map, cam->pagedsize * buf->frames) == -1) {
		logWarn("Error unmapping camera memory: %s", strerror(errno));
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

int drvStopBoard(mod_itifg_cam_t *cam) {
	if (close(cam->fd) == -1) {
		logWarn("Unable to close framegrabber board fd: %s", strerror(errno));
		return EXIT_FAILURE;
	}
	
	return EXIT_SUCCESS;
}

#ifdef FOAM_MODITIFG_ALONE
int main(int argc, char *argv[]) {

	if (argc < 2)  {
		printf("Need config file! call <prog> <conffile.cam>\n");
		exit(-1);
	}
	else
		printf("Using conffile '%s'\n", argv[1]);

	// init vars
	int i, j, f;
	mod_itifg_cam_t camera;
	mod_itifg_buf_t buffer;
	char *file;
	
	camera.module = 48; // some number taken from test_itifg
	strncpy(camera.device_name, "/dev/ic0dma", 512-1);
	strncpy(camera.config_file, argv[1], 512-1);

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
//	drvInitGrab(&camera);
	// start grabbing with a 4 frame loop
	//lseek(camera.fd, buffer.frames * camera.pagedsize, SEEK_END);
	//lseek(camera.fd, buffer.frames * camera.pagedsize, SEEK_END);
	lseek(camera.fd, +LONG_MAX, SEEK_END);

	coord_t res;
	res.x = (int) camera.width;
	res.y = (int) camera.height;

	off_t seekc, seeke;
	//struct timeval timeout;
	struct timeval *timeout = NULL;
	int result;
	mod_itifg_buf_t *buf = &buffer;
	mod_itifg_cam_t *cam = &camera;
	fd_set in_fdset, ex_fdset;
	int pix, pixs;
	
	
	// test images
	for (i=0; i<10; i++) {
		FD_ZERO (&in_fdset);
		FD_ZERO (&ex_fdset);
		FD_SET (camera.fd, &in_fdset);
		FD_SET (camera.fd, &ex_fdset);
		
		//result = poll(&pfd, 1, timeout);
		result = select(1024, &in_fdset, NULL, &ex_fdset, timeout);
		
		if (result == -1)
			printf("Select() returned no active FD's, error:%s\n", strerror(errno));
		else if (result == 0)
			printf("Timeout in drvGetImg().\n");	
		
		printf("select: %d | ", result);
		seeke = lseek(cam->fd, 0, SEEK_END);
		if (seeke == -1)
			printf("SEEK_END failed: %s\n", strerror(errno));
		
		printf("lseek fd 0 seek_end: %d | ", (int) seeke);

		seekc = lseek(cam->fd, 0, SEEK_CUR);
		if (seekc == -1)
			printf("SEEK_CUR failed: %s\n", strerror(errno));
		
		printf("lseek fd 0 seek_cur: %d | frame from %d to %d\n", (int) seekc, (int) seekc, (int) seeke);
		
		buf->data = (void *)((char *)buf->map);
		buf->info = (iti_info_t *)((char *)buf->data + cam->rawsize);
		
		printf("images: \n");
		for (f=0; f<buffer.frames; f++) {
			pixs = 0;
			for (j=0; j<25; j++) { 
				pix = *( ((unsigned char *) (buffer.data)) + j + f*camera.pagedsize); 
				pixs += pow(2,j) * pix;
				printf("%d,", pix);
			}
			printf("H: %d\n", (int) sqrt(pixs));
		}
		
		seekc = lseek(cam->fd, cam->pagedsize, SEEK_CUR);
		printf("lseek fd %d seek_cur: %d | END\n", cam->pagedsize, (int) seekc);
		if (seekc == -1)
			printf("SEEK_CUR failed: %s\n", strerror(errno));

		// reset frame capture
		/*
		if (seekc >= buffer.frames * camera.pagedsize)  {
			seeke = lseek(camera.fd, buffer.frames * camera.pagedsize, SEEK_END);
			printf("RESET: lseek fd %d SEEK_END: %d\n", buffer.frames * camera.pagedsize,(int) seeke);
		}
		*/
//		if (seekc >= buffer.frames * camera.pagedsize) 
//			lseek(camera.fd, buffer.frames * camera.pagedsize, SEEK_END);

		// reset frame capture
		/*
		if (seekc >= buffer.frames * camera.pagedsize) 
			lseek(camera.fd, buffer.frames * camera.pagedsize, SEEK_END);
		*/
		
	}
	lseek(camera.fd, -LONG_MAX, SEEK_END);
	exit(0);
	
	
	printf("Testing various lseek combinations...\n");
	printf("***************************************\n");
		
	printf("\nseek_end 0 / seek_cur pagedsize / buf->data = buf->map...\n");
	printf("****************************************\n");
	for (i=0; i<5; i++) {

		FD_ZERO (&in_fdset);
		FD_ZERO (&ex_fdset);
		FD_SET (cam->fd, &in_fdset);
		FD_SET (cam->fd, &ex_fdset);

		//result = poll(&pfd, 1, timeout);
		result = select(1024, &in_fdset, NULL, &ex_fdset, timeout);

		if (result == -1)
			printf("Select() returned no active FD's, error:%s\n", strerror(errno));
		else if (result == 0)
			printf("Timeout in drvGetImg().\n");	

		printf("select: %d | ", result);
		seeke = lseek(cam->fd, 0, SEEK_END);
		if (seeke == -1)
			printf("SEEK_END failed: %s\n", strerror(errno));

		printf("0 seek_end: %d | ", (int) seeke);
		buf->data = (void *)((char *)buf->map);
		buf->info = (iti_info_t *)((char *)buf->data + cam->rawsize);
			
		seekc = lseek(cam->fd, cam->pagedsize, SEEK_CUR);
		printf("%d seek_cur: %d | ", cam->pagedsize, (int) seekc);
		if (seekc == -1)
			printf("SEEK_CUR failed: %s\n", strerror(errno));

		printf("image: \n");
		for (f=0; f<buffer.frames; f++) {
			pixs = 0;
			for (j=0; j<25; j++) { 
				pix = *( ((unsigned char *) (buffer.data)) + j + f*camera.pagedsize); 
				pixs += pow(2,j) * pix;
				printf("%d,", pix);
			}
			printf("H: %d", (int) sqrt(pixs));
			
			printf("\n");
		}
		//sleep(1);
	}
		
	printf("\n");
	drvStopGrab(&camera);
	drvInitGrab(&camera);
	
	printf("\nseek_end 0 / buf->data = buf->map / seek_cur pagedsize...\n");
	printf("doing SEEK_END, read image, wait 1 sec (buffer fills?), read image, \
		   do SEEK_CUR, read image, wait 1 sec (buffer fills?), read image, loop\n");
	printf("****************************************\n");
	for (i=0; i<3; i++) {
		
		FD_ZERO (&in_fdset);
		FD_ZERO (&ex_fdset);
		FD_SET (cam->fd, &in_fdset);
		FD_SET (cam->fd, &ex_fdset);
		
		//result = poll(&pfd, 1, timeout);
		result = select(1024, &in_fdset, NULL, &ex_fdset, timeout);
		
		if (result == -1)
			printf("Select() returned no active FD's, error:%s\n", strerror(errno));
		else if (result == 0)
			printf("Timeout in drvGetImg().\n");	
		
//		printf("select: %d | ", result);
		seeke = lseek(cam->fd, 0, SEEK_END);
		if (seeke == -1)
			printf("SEEK_END failed: %s\n", strerror(errno));

		printf("0 seek_end: %d |\n", (int) seeke);
		
		for (i=0; i<2; i++) {
			pixs = 0;
			for (j=0; j<25; j++) { 
				pix = *( ((unsigned char *) (buffer.data)) + j); 
				pixs += pow(2,j) * pix;
				printf("%d,", pix);
			}
			printf("H: %d\n", (int) sqrt(pixs));
		}
		
		sleep(1);
		
		for (i=0; i<2; i++) {
			pixs = 0;
			for (j=0; j<25; j++) { 
				pix = *( ((unsigned char *) (buffer.data)) + j); 
				pixs += pow(2,j) * pix;
				printf("%d,", pix);
			}
			printf("H: %d\n", (int) sqrt(pixs));
		}

		buf->data = (void *)((char *)buf->map);
		buf->info = (iti_info_t *)((char *)buf->data + cam->rawsize);
		
		seekc = lseek(cam->fd, cam->pagedsize, SEEK_CUR);
		printf("%d seek_cur: %d |\n", cam->pagedsize, (int) seekc);
		if (seekc == -1)
			printf("SEEK_CUR failed: %s\n", strerror(errno));
		
		for (i=0; i<2; i++) {
			pixs = 0;
			for (j=0; j<25; j++) { 
				pix = *( ((unsigned char *) (buffer.data)) + j); 
				pixs += pow(2,j) * pix;
				printf("%d,", pix);
			}
			printf("H: %d\n", (int) sqrt(pixs));
		}
		
		sleep(1);
		
		for (i=0; i<2; i++) {
			pixs = 0;
			for (j=0; j<25; j++) { 
				pix = *( ((unsigned char *) (buffer.data)) + j); 
				pixs += pow(2,j) * pix;
				printf("%d,", pix);
			}
			printf("H: %d\n", (int) sqrt(pixs));
		}
		
//		printf("image: \n");
//		for (f=0; f<buffer.frames; f++) {
//			pixs = 0;
//			for (j=0; j<25; j++) { 
//				pix = *( ((unsigned char *) (buffer.data)) + j + f*camera.pagedsize); 
//				pixs += pow(2,j) * pix;
//				printf("%d,", pix);
//			}
//			printf("H: %d", (int) sqrt(pixs));
//			
//			printf("\n");
//		}
	}
	
	printf("\n");
	//	drvStopGrab(&camera);
	//	drvInitGrab(&camera);
	
//	printf("seek_end 0 / seek_cur <seek_end out> / buf->data = buf->map...\n");
//	printf("****************************************\n");
//	for (i=0; i<5; i++) {
//		FD_ZERO (&in_fdset);
//		FD_ZERO (&ex_fdset);
//		FD_SET (cam->fd, &in_fdset);
//		FD_SET (cam->fd, &ex_fdset);
//		
//		//result = poll(&pfd, 1, timeout);
//		result = select(1024, &in_fdset, NULL, &ex_fdset, timeout);
//		
//		if (result == -1)
//			printf("Select() returned no active FD's, error:%s\n", strerror(errno));
//		else if (result == 0)
//			printf("Timeout in drvGetImg().\n");	
//		
//		printf("select: %d | ", result);
//		seeke = lseek(cam->fd, 0, SEEK_END);
//		if (seeke == -1)
//			printf("SEEK_END failed: %s\n", strerror(errno));
//		
//		printf("0 seek_end: %d | ", (int)  seeke);
//		buf->data = (void *)((char *)buf->map);
//		buf->info = (iti_info_t *)((char *)buf->data + cam->rawsize);
//		
//		seekc = lseek(cam->fd, seeke, SEEK_CUR);
//		printf("%d seek_cur: %d | ", (int) seeke, (int) seekc);
//		if (seekc == -1)
//			printf("SEEK_CUR failed: %s\n", strerror(errno));
//		
//		printf("image: \n");
//		for (f=0; f<buffer.frames; f++) {
//			pixs = 0;
//			for (j=0; j<25; j++) { 
//				pix = *( ((unsigned char *) (buffer.data)) + j + f*camera.pagedsize); 
//				pixs += pow(2,j) * pix;
//				printf("%d,", pix);
//			}
//			printf("H: %d", (int) sqrt(pixs));
//			
//			printf("\n");
//		}
//	}
//	
//	
//	printf("\n");
//	drvStopGrab(&camera);
//	drvInitGrab(&camera);
//	printf("seek_end <seek_end out> / seek_cur pagedsize / buf->data = buf->map...\n");
//	printf("****************************************\n");
//	// old seek_out value, init to 0
//	off_t seekeo = (off_t) 0;
//	for (i=0; i<5; i++) {
//		FD_ZERO (&in_fdset);
//		FD_ZERO (&ex_fdset);
//		FD_SET (cam->fd, &in_fdset);
//		FD_SET (cam->fd, &ex_fdset);
//		
//		//result = poll(&pfd, 1, timeout);
//		result = select(1024, &in_fdset, NULL, &ex_fdset, timeout);
//		
//		if (result == -1)
//			printf("Select() returned no active FD's, error:%s\n", strerror(errno));
//		else if (result == 0)
//			printf("Timeout in drvGetImg().\n");	
//		
//		printf("select: %d | ", result);
//		seeke = lseek(cam->fd, seekeo, SEEK_END);
//		if (seeke == -1)
//			printf("SEEK_END failed: %s\n", strerror(errno));
//		
//		printf("%d seek_end: %d | ", (int) seekeo, (int) seeke);
//		
//		// store seek_out output for use in next loop
//		seekeo = seeke;
//		buf->data = (void *)((char *)buf->map);
//		buf->info = (iti_info_t *)((char *)buf->data + cam->rawsize);
//		
//		seekc = lseek(cam->fd, cam->pagedsize, SEEK_CUR);
//		printf("%d seek_cur: %d | ", cam->pagedsize, (int) seekc);
//		if (seekc == -1)
//			printf("SEEK_CUR failed: %s\n", strerror(errno));
//		
//		printf("image: \n");
//		for (f=0; f<buffer.frames; f++) {
//			pixs = 0;
//			for (j=0; j<25; j++) { 
//				pix = *( ((unsigned char *) (buffer.data)) + j + f*camera.pagedsize); 
//				pixs += pow(2,j) * pix;
//				printf("%d,", pix);
//			}
//			printf("H: %d", (int) sqrt(pixs));
//			
//			printf("\n");
//		}
//	}
//
//	drvStopGrab(&camera);
//	drvInitGrab(&camera);
//	printf("\nseek_end 0 / seek_cur <disabled> / buf->data = buf->map...\n");
//	printf("****************************************\n");
//	for (i=0; i<5; i++) {
//
//		FD_ZERO (&in_fdset);
//		FD_ZERO (&ex_fdset);
//		FD_SET (cam->fd, &in_fdset);
//		FD_SET (cam->fd, &ex_fdset);
//
//		//result = poll(&pfd, 1, timeout);
//		result = select(1024, &in_fdset, NULL, &ex_fdset, timeout);
//
//		if (result == -1)
//			printf("Select() returned no active FD's, error:%s\n", strerror(errno));
//		else if (result == 0)
//			printf("Timeout in drvGetImg().\n");	
//
//		printf("select: %d | ", result);
//		seeke = lseek(cam->fd, 0, SEEK_END);
//		if (seeke == -1)
//			printf("SEEK_END failed: %s\n", strerror(errno));
//
//		printf("0 seek_end: %d | ", (int) seeke);
//		buf->data = (void *)((char *)buf->map);
//		buf->info = (iti_info_t *)((char *)buf->data + cam->rawsize);
//			
//		//seekc = lseek(cam->fd, cam->pagedsize, SEEK_CUR);
//		printf("%d seek_cur: <disabled> | ", cam->pagedsize);
//		//if (seekc == -1)
//			//printf("SEEK_CUR failed: %s\n", strerror(errno));
//
//		printf("image: \n");
//		for (f=0; f<buffer.frames; f++) {
//			pixs = 0;
//			for (j=0; j<25; j++) { 
//				pix = *( ((unsigned char *) (buffer.data)) + j + f*camera.pagedsize); 
//				pixs += pow(2,j) * pix;
//				printf("%d,", pix);
//			}
//			printf("H: %d", (int) sqrt(pixs));
//			
//			printf("\n");
//		}
//	}
//		
//	printf("\n");
//	printf("cleaning up now\n");
	
	//drvStopGrab(&camera);
	//drvInitGrab(&camera);
	longrun:
	printf("\nseek_end 0 / seek_cur pagedsize / buf->data = buf->map... long run!\n");
	printf("****************************************\n");
	// no buffering 
	setvbuf(stdout, NULL, _IONBF, 0);
	for (i=0; i<31000; i++) {
		
		FD_ZERO (&in_fdset);
		FD_ZERO (&ex_fdset);
		FD_SET (cam->fd, &in_fdset);
		FD_SET (cam->fd, &ex_fdset);
		
		//result = poll(&pfd, 1, timeout);
		result = select(1024, &in_fdset, NULL, &ex_fdset, timeout);
		
		if (result == -1)
			printf("Select() returned no active FD's, error:%s\n", strerror(errno));
		else if (result == 0)
			printf("Timeout in drvGetImg().\n");	
		
		seeke = lseek(cam->fd, 0, SEEK_END);
		if (seeke == -1)
			printf("SEEK_END failed: %s\n", strerror(errno));
		
		buf->data = (void *)((char *)buf->map);
		buf->info = (iti_info_t *)((char *)buf->data + cam->rawsize);
		
		seekc = lseek(cam->fd, cam->pagedsize, SEEK_CUR);
		if (seekc == -1)
			printf("SEEK_CUR failed: %s\n", strerror(errno));

		if (i % 100 == 0)
			printf(".");
		if (i > 30000) 
			printf("i: %d, end: %d, cur: %d\n", i, (int) seeke, (int) seekc);
	}
	printf("after 31k images: seek_end: %d, seek_cur: %d \n", (int) seeke, (int) seekc);
	printf("buffer looks like (pagedsize boundaries):\n");
	
	for (f=0; f<buffer.frames; f++) {
		for (j=0; j<25; j++) { 
			pix = *( ((unsigned char *) (buffer.data)) + j + f*camera.pagedsize); 
			printf("%d,", pix);
		}
		printf("\n");
	}

	printf("buffer looks like (iti_info_t appendix (first 25 bytes)):\n");
	for (f=0; f<buffer.frames; f++) {
		for (j=0; j<25; j++) { 
			pix = *( ((unsigned char *) (buffer.data)) + camera.rawsize + j + f*camera.pagedsize); 
			printf("%d,", pix);
		}
		printf("\n");
	}

	printf("buffer looks like (iti_info_t appendix (first 25 bytes)):\n");
	for (f=0; f<buffer.frames; f++) {
		for (j=0; j<25; j++) { 
			pix = *( ((unsigned char *) (buffer.data)) + camera.rawsize + j + f*camera.pagedsize); 
			printf("%d,", pix);
		}
		printf("\n");
	}
	
	
	printf("buffer looks like (rawsize boundaries):\n");
	for (f=0; f<buffer.frames; f++) {
		for (j=0; j<25; j++) { 
			pix = *( ((unsigned char *) (buffer.data)) + j + f*camera.rawsize); 
			printf("%d,", pix);
		}
		printf("\n");
	}
	
	printf("\n");
	printf("cleaning up now\n");
	
	// cleanup
	drvStopGrab(&camera);
	drvStopBufs(&buffer, &camera);
	
	// end
	printf("exit\n");
	
	return 0;
}
#endif


