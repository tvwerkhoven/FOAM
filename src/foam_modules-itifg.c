/*! 
	@file foam_modules-itifg.c
	@author @authortim
	@date 2008-03-03 16:49

	@brief This file contains routines to read out a PCDIG itifg camera.

	\section Info
 
	More documentation at the end of the source file (Hints for getting itifg running by Matthias Stein)
	
	\section Functions
	
	\li drvReadSensor() - Reads out a camera into the global ptc struct
	
	\section History
	This file is based on itifg.cc, part of filter_control by Guus Sliepen <guus@sliepen.eu.org>
	which was released under the GPL version 2.
*/

#define FOAM_MODITIFG_ALONE 1
#define FOAM_MODITIFG_DEBUG 1
#define FOAM_MODITIFG_DEV "/dev/ic0dma"
#define FOAM_MODITIFG_CONFFILE ""
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
#include <errno.h>
//#include <signal.h> // ?
//#include <setjmp.h> // ?
#include <fcntl.h>	// for O_RDWR, open()
#include <time.h> // for itifgExt.h
//#include <termios.h> // ?
#include <math.h> // ?

#include <sys/ioctl.h>	// for ioctl()
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

int drvInitSensor();


int drvInitSensor() {
	char device_name[] = FOAM_MODITIFG_DEV;
	char config_file[] = FOAM_MODITIFG_CONFFILE;
	char camera_name[512];
	char exo_name[512];
	
	// TvW: | O_SYNC | O_APPEND also used in test_itifg.c
	int flags = O_RDWR;
	int zero = 0;
	int one = 1;
	int fd;
	union iti_cam_t cam;
	
	
	fd = open(device_name, flags);
	if (fd == -1) {
		FOAM_MODITIFG_ERR("Error opening device %s: %s\n", device_name, strerror(errno));
		return EXIT_FAILURE;
	}
	
	if (ioctl(fd, GIOC_SET_LUT_LIN) < 0) {
		close(fd);
		FOAM_MODITIFG_ERR("%s: error linearising LUTs: %s\n", device_name, strerror(errno));
		return EXIT_FAILURE;
	}
	
	if (ioctl(fd, GIOC_SET_DEFCNF, NULL) < 0) {
		close(fd);
		FOAM_MODITIFG_ERR("%s: error setting camera configuration: %s\n", device_name, strerror(errno));
		return EXIT_FAILURE;
	}	
	
	if (ioctl(fd, GIOC_SET_CAMERA, &zero) < 0) {
		close(fd);
		FOAM_MODITIFG_ERR("%s: error setting camera: %s\n", device_name, strerror(errno));
		return EXIT_FAILURE;
	}
	
	if (ioctl(fd, GIOC_GET_CAMCNF, &cam) < 0) {
		close(fd);
		FOAM_MODITIFG_ERR("%s: error getting camera configuration: %s\n", device_name, strerror(errno));
		return EXIT_FAILURE;
	}
	
	int result;	
	*camera_name = *exo_name = 0;
	
	if ((result = iti_read_config(config_file, &cam, 0, FOAM_MODITIFG_MODULE, 0, camera_name, exo_name)) < 0) {
		close(fd);
		FOAM_MODITIFG_ERR("%s: error reading camera configuration: %s\n", device_name, strerror(errno));
		return EXIT_FAILURE;		
	}
	
	if (ioctl(fd, GIOC_SET_CAMCNF, &cam) < 0) {
		close(fd);
		FOAM_MODITIFG_ERR("%s: error setting camera configuration: %s\n", device_name, strerror(errno));
		return EXIT_FAILURE;
	}
	
	
	return 0;
}

#ifdef FOAM_MODITIFG_ALONE
int main() {
	// init vars
	
	printf("This is the debug version for ITIFG8\n");
	
	// init cam
	drvInitSensor();
	
	// test image
	
	// cleanup
	
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

