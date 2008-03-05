/*! 
	@file foam_modules-itifg.c
	@author @authortim
	@date 2008-03-03 16:49

	@brief This file contains routines to read out a PCDIG itifg camera

	\section Info
	
	\section Functions
	
	\li modSimDM
	
	\section History
	This file is based on itifg.cc, part of filter_control by Guus Sliepen <guus@sliepen.eu.org>
	which was released under the GPL version 2.
*/

int drvReadSensor() {
	char device_name[128];
	int flags = O_RDWR;
	FILE *fd;
	// TvW: | O_SYNC | O_APPEND also used in test_itifg.c
	
	fd = open(device_name, flags);
	if (fd == NULL) logErr("Error opening device %s: %s", device_name, strerror(errno));
	
	closed(fd);
}