/*! 
	@file foam_modules-itifg.c
	@author @authortim
	@date 2008-03-03 16:49

	@brief This file contains routines to read out a PCDIG framegrabber using the ITIFG driver.

	\section Info
 
	More documentation at the end of the source file (Hints for getting itifg running by Matthias Stein)
 
	This module compiles on its own\n
	<tt>gcc foam_modules-itifg.c -DFOAM_MODITIFG_ALONE=1 -Wall -lc -I../../../drivers/itifg-8.4.0-0/include/ -L../../../drivers/itifg-8.4.0-0/lib/ -litifg_g -lm -lc</tt>
	
	\section Functions
	
	\li drvReadSensor() - Reads out a camera into the global ptc struct
	
	\section History
	This file is based on itifg.cc, part of filter_control by Guus Sliepen <guus@sliepen.eu.org>
	which was released under the GPL version 2.
*/

#define FOAM_MODITIFG_ALONE 1
#define FOAM_MODITIFG_DEBUG 1
#define FOAM_MODITIFG_DEV "/dev/ic0dma"
#define FOAM_MODITIFG_CONFFILE "../config/dalsa-cad6-pcd.cam"
#define FOAM_MODITIFG_MODULE 48

#ifdef FOAM_MODITIFG_ALONE
#define FOAM_MODITIFG_DEBUG 1
#endif

#ifdef FOAM_MODITIFG_DEBUG
#define FOAM_MODITIFG_ERR printf
#endif

//#include <foam_modules-itifg.h>

#include <stdio.h>	// for stuff
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

//#include <signal.h> // ?
//#include <setjmp.h> // ?

//#include <termios.h> // ?
//#include <sys/param.h> // ?
//#include <sys/stat.h> // ?
//#include <sys/ipc.h> // ?
//#include <sys/shm.h> // ?
//#include <sys/mman.h> // ?
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
 
 To initialize a camera, some information is needed. This is stored in this
 struct that will be passed along to camera related functions. These 
 functions will then fill in the blanks as much as possible, given some
 data that is already present. See functions for details.
 */
typedef struct {
	short width;			//!< will hold CCD width
	short height;			//!< will hold CCD height
	int depth;				//!< will hold CCD depth (i.e. 8bit)
	int fd;					//!< will hold FD to the framegrabber
	size_t pagedsize;		//!< size of the complete frame + some metadata
	size_t rawsize;			//!< size of the raw frame (width*height*depth)
	union iti_cam_t cam;	//!< see iti_cam_t
	int module;				//!< 48 in mcmath setup
	char device_name[512];	//!< something like '/dev/ic0dma'
	char config_file[512];	//!< something like '../conffiles/dalsa-cad6.cam'
	char camera_name[512];	//!< will be provided by itifg
	char exo_name[512];		//!< will be provided by itifg
} mod_itifg_cam;

typedef struct {
	int frames;				//!< how many frames should the buffer hold?
	iti_info_t *info;		//!< this will point to information on the current frame
	void *data;				//!< this will point to the current frame
	void *map;				//!< this will point to the mmap()'ed memory
} mod_itifg_buf;

int drvInitSensor(mod_itifg_cam *cam);
int drvInitBufs(mod_itifg_buf *buf, mod_itifg_cam *cam);

int drvInitSensor(mod_itifg_cam *cam) {
	// TvW: | O_SYNC | O_APPEND also used in test_itifg.c
	int flags = O_RDWR;
	int zero = 0;
	int one = 1;
	int result;	
		
	cam->fd = open(cam->device_name, flags);
	if (cam->fd == -1) {
		FOAM_MODITIFG_ERR("Error opening device %s: %s\n", cam->camera_name, strerror(errno));
		return EXIT_FAILURE;
	}
	
	if (ioctl(cam->fd, GIOC_SET_LUT_LIN) < 0) {
		close(cam->fd);
		FOAM_MODITIFG_ERR("%s: error linearising LUTs: %s\n", cam->camera_name, strerror(errno));
		return EXIT_FAILURE;
	}
	
	if (ioctl(cam->fd, GIOC_SET_DEFCNF, NULL) < 0) {
		close(cam->fd);
		FOAM_MODITIFG_ERR("%s: error setting camera configuration: %s\n", cam->camera_name, strerror(errno));
		return EXIT_FAILURE;
	}	
	
	if (ioctl(cam->fd, GIOC_SET_CAMERA, &zero) < 0) {
		close(cam->fd);
		FOAM_MODITIFG_ERR("%s: error setting camera: %s\n", cam->camera_name, strerror(errno));
		return EXIT_FAILURE;
	}
	
	if (ioctl(cam->fd, GIOC_GET_CAMCNF, &cam) < 0) {
		close(cam->fd);
		FOAM_MODITIFG_ERR("%s: error getting camera configuration: %s\n", cam->camera_name, strerror(errno));
		return EXIT_FAILURE;
	}
	
	if ((result = iti_read_config(cam->config_file, &(cam->cam), 0, cam->module, 0, cam->camera_name, cam->exo_name)) < 0) {
		close(cam->fd);
		FOAM_MODITIFG_ERR("%s: error reading camera configuration: %s\n", cam->camera_name, strerror(errno));
		return EXIT_FAILURE;		
	}
	
	if (ioctl(cam->fd, GIOC_SET_CAMCNF, &(cam->cam)) < 0) {
		close(cam->fd);
		FOAM_MODITIFG_ERR("%s: error setting camera configuration: %s\n", cam->camera_name, strerror(errno));
		return EXIT_FAILURE;
	}
	
	if (ioctl(cam->fd, GIOC_SET_TIMEOUT, &(struct timeval){0, 0}) < 0) {
		close(cam->fd);
		FOAM_MODITIFG_ERR("%s: error setting timeout: %s\n", cam->camera_name, strerror(errno));
		return EXIT_FAILURE;
	}
#ifdef FOAM_MODITIFG_DEBUG
	FOAM_MODITIFG_ERR("Timout set to {0,0}\n");
#endif
	
	if(ioctl(cam->fd, GIOC_SET_HDEC, &one) < 0) {
		close(cam->fd);
		FOAM_MODITIFG_ERR("%s: error setting horizontal decimation: %s\n", cam->camera_name, strerror(errno));
		return EXIT_FAILURE;
	}
	
	if(ioctl(cam->fd, GIOC_SET_VDEC, &one) < 0) {
		close(cam->fd);
		FOAM_MODITIFG_ERR("%s: error setting vertical decimation: %s\n", cam->camera_name, strerror(errno));
		return EXIT_FAILURE;
	}
#ifdef FOAM_MODITIFG_DEBUG
	FOAM_MODITIFG_ERR("decimation set to {0,0}\n");
#endif
	
	if(ioctl(cam->fd, GIOC_GET_WIDTH, &(cam->width)) < 0) {
		close(cam->fd);
		FOAM_MODITIFG_ERR("%s: error getting width: %s\n", cam->camera_name, strerror(errno));
		return EXIT_FAILURE;
	}
	
	if(ioctl(cam->fd, GIOC_GET_HEIGHT, &(cam->height)) < 0) {
		close(cam->fd);
		FOAM_MODITIFG_ERR("%s: error getting height: %s\n", cam->camera_name, strerror(errno));
		return EXIT_FAILURE;
	}
	
	if(ioctl(cam->fd, GIOC_GET_DEPTH, &(cam->depth)) < 0) {
		close(cam->fd);
		FOAM_MODITIFG_ERR("%s: error getting depth: %s\n", cam->camera_name, strerror(errno));
		return EXIT_FAILURE;
	}
#ifdef FOAM_MODITIFG_DEBUG
	FOAM_MODITIFG_ERR("width x height x depth: %dx%dx%d\n", cam->width, cam->height, cam->depth);
#endif
	
	if(ioctl(cam->fd, GIOC_GET_RAWSIZE, &(cam->rawsize)) < 0) {
		close(cam->fd);
		FOAM_MODITIFG_ERR("%s: error getting raw size: %s\n", cam->camera_name, strerror(errno));
		return EXIT_FAILURE;
	}
	
	if(ioctl(cam->fd, GIOC_GET_PAGEDSIZE, &(cam->pagedsize)) < 0) {
		close(cam->fd);
		FOAM_MODITIFG_ERR("%s: error getting paged size: %s\n", cam->camera_name, strerror(errno));
		return EXIT_FAILURE;
	}
#ifdef FOAM_MODITIFG_DEBUG
	FOAM_MODITIFG_ERR("raw size: %d, paged size: %d\n", cam->rawsize, cam->pagedsize);
#endif
	
	if (fcntl(cam->fd, F_SETFL, fcntl(cam->fd, F_GETFL, NULL) & ~O_NONBLOCK) < 0) {
		close(cam->fd);
		FOAM_MODITIFG_ERR("%s: error setting blocking: %s\n", cam->camera_name, strerror(errno));
		return EXIT_FAILURE;
	}

#ifdef FOAM_MODITIFG_DEBUG
	FOAM_MODITIFG_ERR("Camera configuration done.");
#endif
	
	return EXIT_SUCCESS;
}

int drvInitBufs(mod_itifg_buf *buf, mod_itifg_cam *cam) {
	
	// start mmap
	buf->map = mmap(NULL, cam->pagedsize * buf->frames, PROT_READ | PROT_WRITE, MAP_SHARED, cam->fd, 0);
	
	if (buf->map == (void *)-1) {
		FOAM_MODITIFG_ERR("Could not mmap(): %s\n", strerror(errno));
		return EXIT_FAILURE;
	}
	
	buf->data = buf->map;
	buf->info = (iti_info_t *)((char *)buf->data + cam->rawsize);
	//lseek +LONG_MAX SEEK_END

#ifdef FOAM_MODITIFG_DEBUG
	FOAM_MODITIFG_ERR("mmap() success");
#endif
	
	return EXIT_SUCCESS;
}

void drvInitGrab(mod_itifg_cam *cam) {
	// reset stats if possible
//	ioctl(cam->fd, GIOC_SET_STATS, NULL);
	
	// start the framegrabber by seeking a lot???
	lseek(cam->fd, +LONG_MAX, SEEK_END);
}

void drvStopGrab(mod_itifg_cam *cam) {
	// start the framegrabber by seeking a lot???
	lseek(cam->fd, -LONG_MAX, SEEK_END);
}

int drvGetImg(mod_itifg_cam *cam, mod_itifg_buf *buf, int timeout) {
	int result;
//	struct iti_acc_t acc;
	struct pollfd pfd = {cam->fd, POLLIN, 0};
	
	result = poll(&pfd, 1, timeout);
	if (result <= 0)
		return EXIT_FAILURE;
	
//	if (ioctl(cam->fd, GIOC_GET_STATS, &acc) < 0) {
//		FOAM_MODITIFG_ERR("Could not read framegrabber statistics");
//		return EXIT_FAILURE;
//	}
	
//	buf->data = (void *)((char *)buf->map + ((acc.transfered - 1) % buf->frames) * cam->pagedsize);
	buf->data = (void *)((char *)buf->map + ((1 - 1) % buf->frames) * cam->pagedsize);
	buf->info = (iti_info_t *)((char *)buf->data + cam->rawsize);
	
	// TvW: hoes does this work, exactly?
//	if (acc.transfered != info->framenums.transfered) {
//		FOAM_MODITIFG_ERR("Frame %lu not in right place in mmap area (%lu is in its spot)", acc.transfered, info->framenums.transfered);
//		return EXIT_FAILURE;
//	}
	
	return EXIT_SUCCESS;
}

int drvStopBufs(mod_itifg_buf *buf, mod_itifg_cam *cam) {
	munmap(buf->map, cam->pagedsize * buf->frames);
	return EXIT_SUCCESS;
}


#ifdef FOAM_MODITIFG_ALONE
int main() {
	// init vars
	int i, j;
	mod_itifg_cam camera;
	mod_itifg_buf buffer;
	
	camera.module = 48; // some number taken from test_itifg
	strncpy(camera.device_name, "/dev/ic0dma", 512-1);
	strncpy(camera.config_file, "../conffiles/dalsa-cad6.cam", 512-1);

	buffer.frames = 4; // ringbuffer will be 4 frames big
	
	printf("This is the debug version for ITIFG8\n");
	
	// init cam
	drvInitSensor(&camera);
	
	// init bufs
	drvInitBufs(&buffer, &camera);
	
	// test image
	for (i=0; i<10; i++) {
		drvGetImg(&camera, &buffer, 1000);
		printf("Frames grabbed: %d\n", buffer.info->acq.captured);
		printf("Pixels 1 through 100:\n");
		for (j=0; j<100; j++)
			printf("%d,", *( ((char *) (buffer.data)) + j) );
		
		printf("\n");
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

/*
There are two basic operation modes:

1. The 'grab' mode
You only want to have one image at a time and You want to have the
most recent image. You don't care, if there are images lost, when
You process one.
Often used for live video and quick processing.

call sequence:
open
mmap ringbuffer size (min 2 images)
ioctl GET_CAMCNF
modify via iti_read_config
ioctl SET_CAMCNF
ioctl GET_RAW|PAGED_SIZE
lseek +LONG_MAX SEEK_END

loop:
wait for image (poll/select/signal)
lseek 0 SEEK_CUR to get the offset of the current image into Your
memory
Image Processing

lseek -LONG_MAX SEEK_END
munmap
close

2. The 'snap' mode
You are interested in EVERY image you can get. No image should be
lost. The number of images is limited by the main memory You have.

same call sequence except:
no mmap/munmap
lseek number_of_bytes_to_read SEEK_END
into the loop:
only read - it is blockin till a new image is there

When working in grab mode - mmap makes sense, in snap mode - read is
the better solution.

- What is the difference between the /dev/ic0??? descriptors? when
are they used?

There are cfg, acq, dma, exs, lut, iop.

cfg - FPGA download
acq - camera to board acquisition
dma - on-board memory to host memory transfer
exs - trigger/shutter/exposure handling
lut - on-board lookup table handling
iop - on-board i/o ports

For common image acqustion usage, You never use acq directly! You open
dma instead and if acq isn't open at this moment, dma couples acq
internally.

- How do I start and stop the framegrabber? The changelog mentions
lseek() ('ioctls STRT/STOP replaced by one lseek interface'), where
and how can I use that with itifg?

to start an image acquisition, You have to do

grab mode: lseek +LONG_MAX SEEK_END

snap mode: lseek number_of_images * number_of_bytes_per_image SEEK_END

to stop acqusition, you alwas do

lseek -LONG_MAX SEEK_END

For the grab mode you need some additional calls lseek SEEK_CUR/SEEK_END
to get the offset of the most recent image/to confirm this image

- What do the various parameters in the .cam files mean?

You setup the board for a specific camera. The parameters are different
for the different boards and different cameras. There are some more
exapmles into the conffiles dir and some of the parameters are self
explaining. If You are unsure ask me for the behavior of a a specific
switch.

- Are all camera specific settings in the .cam files or do I need to
do some things myself? I.e.: do certain cameras require a different
mode of operation, or is this all the same for all cameras?

There can be various operation modes for one camera. One of the most
important choise it weather the camera makes images from itself (free-
																 running mode) or it is controlled by the board (trigger and/or exposure
																												 time). Then the exs device gets involved.

And more specifically:
- How do I find out the module that I need to use with
iti_read_config() (fourth argument)? I currently use 48, but I don't
know why (this is what itifg_test uses).

There is a function iti_parse_info, that reads /proc/itifg8 and fills
You a 'setup' structure, You can the it from that. Other approach:
You tell me Your board an I tell You the number.

If I overlooked certain documentation, please tell me. Also, if these
questions are too general for the itifg driver, let me know. However I
could not find the answer to these questions (easily). An alternative
would be to take your test code and cut out the parts I need, but
that's a rather hacky approach in which case I wouldn't fully
understand what I would be doing.

Thanks in advance,

Tim van Werkhoven


Sorry to be short at some points, the time...

Happy Easter! (I'm back next Tuesday)

matthias
 
*/

