/*! 
 @file foam_modules-serial.c
 @author @authortim
 @date 2008-03-31 16:51
 
 @brief This file contains routines to drive the serial port (i.e. for filterwheels)
 
 \section Info
 
 For info, look at the source, it isn't that complicated (really)
 
 This module can compile on its own\n
 <tt>gcc foam_modules-serial.c -Wall -std=c99 -DFOAM_MODSERIAL_ALONE=1</tt>
 
 \section Functions
 
 \li drvSetSerial() - Sets a value on a serial port
 
 \section Configuration
 
 This module only supports these configurations:
 \li \b FOAM_MODSERIAL_ALONE (*undef*), ifdef, this module compiles on its own
 \li \b FOAM_MODSERIAL_DEBUG (*undef*), ifdef, gives lowlevel debug information
 
 \section History
 
 \li 2008-04-02: init

 */

#include <stdio.h>		// for stuff
#include <unistd.h>		// for close
#include <stdlib.h>		// has EXIT_SUCCESS / _FAILURE (0, 1)
#include <sys/types.h>	// for linux (open)
#include <sys/stat.h>	// for linux (open)
#include <fcntl.h>		// for fcntl
#include <errno.h>
#include <string.h>
//#include <string.h>
//#include <math.h>

#ifdef FOAM_MODSERIAL_ALONE
#define FOAM_MODSERIAL_DEBUG 1		//!< set to 1 for debugging, in that case this module compiles on its OwnerGrabButtonMask
#endif

/*!
 @brief This function a string on a port and returns the bytes written
 
 @param [in] *port The port to write to
 @param [in] *cmd The string to write to the port
 @return Number of bytes written on success, EXIT_FAILURE otherwise.
 */
int drvSetSerial(const char *port, const char *cmd) {
	int fd;
	// cmd is something like "3WX\r" with X a number (from tt3.h)
	// port is something like "/dev/ttyS0"
	
	if (port == NULL || cmd == NULL)
		return EXIT_FAILURE;
	
	fd = open(port, O_RDWR | O_NOCTTY | O_NDELAY);

	
    if (fd == -1) {
#ifdef FOAM_MODSERIAL_DEBUG
		printf("Unable to access serial port %s: %s\n", port, strerror(errno));
#else
		logErr("Unable to access serial port %s: %s", port, strerror(errno));
#endif
		return EXIT_FAILURE;
	}
	else 
		fcntl(fd, F_SETFL, 0);

#ifdef FOAM_MODSERIAL_DEBUG
	printf("Successfully opened port '%s'\n", port);
#endif
	
    int n = write(fd, cmd, strlen(cmd));
    if (n == -1)
#ifdef FOAM_MODSERIAL_DEBUG
		printf("Unable to write to serial port, asked to write %s (%d bytes) to %s, which failed: %s\n", \
			   cmd, (int) strlen(cmd), port, strerror(errno));
#else
		logErr("Unable to write to serial port, asked to write %s (%d bytes) to %s, which failed: %s", \
		   cmd, (int) strlen(cmd), port, strerror(errno));

#endif
	
    close(fd);
	
#ifdef FOAM_MODSERIAL_DEBUG
	printf("Successfully wrote '%s' to port\n", cmd);
#endif
	
	return n;
}

#ifdef FOAM_MODSERIAL_ALONE
int main (int argc, char *argv[]) {
	int i;
	char cmd[4] = "XR0\r";
	//int fd, ret;
	char buf[4];
	buf[3] = '\0';
	
	if (argc < 4) {
		printf("Please run me as <script> <port> <begin> <end> and I "\
		"will write 'XR0\\r' to serial port <port>, with 0 ranging "\
		"from <begin> to <end>\n", cmd);
		printf("In ao3 (tt3.h:170), values 1 thru 4 were used\n");
		return -1;
	}
	int beg = (int) strtol(argv[2], NULL, 10);
	int end = (int) strtol(argv[3], NULL, 10);
	
	printf("Printing 'XR0\\r' to serial port %s with 0 ranging from %d to "\
		"%d\n", cmd, argv[1], beg, end);
	

	for (i=beg; i<end+1; i++) {
		cmd[2] = i+0x30; // convert int to ASCII
		printf("Trying to write 'XR%d\\r' to %s...", i, argv[1]);

		if (drvSetSerial(argv[1], cmd) == -1) {
			printf("failed.\n");
		}
		else {
			printf("success!\n");
			/*
			printf("Reading back...");
			fd = open(argv[1], O_RDWR);
			printf("opened...");
			if (fd == -1)
				printf("failed\n");
			else {
				ret = read(fd, buf, (size_t) 3);
				if (ret == -1)
					printf("reading failed: %s\n", strerror(errno));
				else
					printf("success: %s\n", buf);

				close(fd);

			}
			 */
		}
		
		// sleep for 5 seconds between each call if we are debugging
		usleep(5000000);
	}
	
	cmd[2] = 6+0x30; // convert int to ASCII
	printf("Trying to write 'XR%d\\r' to %s...", i, argv[1]);

	if (drvSetSerial(argv[1], cmd) == -1)
		printf("failed.\n");
	else
		printf("success!\n");

		
	usleep(5000000);
	return 0;
}
#endif
