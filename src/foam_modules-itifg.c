/*! 
	@file foam_modules-itifg.c
	@author @authortim
	@date 2008-03-03 16:49

	@brief This file contains routines to read out a PCDIG itifg camera.

	\section Info
	
	\section Functions
	
	\li drvReadSensor() - Reads out a camera into the global ptc struct
	
	\section History
	This file is based on itifg.cc, part of filter_control by Guus Sliepen <guus@sliepen.eu.org>
	which was released under the GPL version 2.
*/

#include <foam_modules-itifg.h>

int drvReadSensor() {
	char device_name[] = "/dev/ic0dma";
	int flags = O_RDWR;
	int zero = 0;
	int one = 1;
	FILE *fd;
	union iti_cam_t cam;
	// TvW: | O_SYNC | O_APPEND also used in test_itifg.c
	
	fd = open(device_name, flags);
	if (!fd) 
		printf("Error opening device %s: %s", device_name, strerror(errno));
		
	if (ioctl(fd, GIOC_SET_LUT_LIN) < 0) {
		close(fd);
		printf(device + ": error linearising LUTs: " + strerror(errno));
	}

	if (ioctl(fd, GIOC_SET_DEFCNF, NULL) < 0) {
		close(fd);
		printf(device + ": error getting camera configuration: " + strerror(errno));
	}	
	
	if (ioctl(fd, GIOC_SET_CAMERA, &zero) < 0) {
		close(fd);
		printf(device + ": error setting camera: " + strerror(errno));
	}

	if (ioctl(fd, GIOC_GET_CAMCNF, &cam) < 0) {
		close(fd);
		printf(device + ": error getting camera configuration: " + strerror(errno));
	}

	int result;
	
	*camera_name = *exo_name = 0;
	
	
	closed(fd);
}