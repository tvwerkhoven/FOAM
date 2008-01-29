/*! 
@file foam_cs_library 
@brief Library file for the Control Software
@author @authortim
@date November 13 2007

(was cs_library.c)

This file contains things necessary to run the Control Software that are not related to adaptive optics itself. 

*/

#include "foam_cs_library.h"

control_t ptc = { //!< Global struct to hold system characteristics and other data. Initialize with complete but minimal configuration
	.mode = AO_MODE_OPEN,
	.wfs_count = 0,
	.wfc_count = 0,
	.frames = 0
};

config_t cs_config = { 
	.listenip = "0.0.0.0", // listen on any IP by default, can be overridden by config file
	.listenport = 10000,
	.infofd = NULL,
	.errfd = NULL,
	.debugfd = NULL,
	.use_syslog = false,
	.syslog_prepend = "foam",
	.use_stderr = true,
	.loglevel = LOGINFO
};

conntrack_t clientlist;

SDL_Surface *screen;	// Global surface to draw on


static int formatLog(char *output, const char *prepend, const char *msg) {
	char timestr[9];
	time_t curtime;
	struct tm *loctime;

	curtime = time (NULL);
	loctime = localtime (&curtime);
	strftime (timestr, 9, "%H:%M:%S", loctime);

	output[0] = '\0'; // reset string
	
	strcat(output,timestr);
	strcat(output,prepend);
	strcat(output,msg);
	strcat(output,"\n");
	
	return EXIT_SUCCESS;
}

/* logging functions */
void logInfo(const char *msg, ...) {
	if (cs_config.loglevel < LOGINFO) 		// Do we need this loglevel?
		return;
	
	va_list ap, aq, ar; // We need three of these because we cannot re-use a va_list variable
	
	va_start(ap, msg);
	va_copy(aq, ap);
	va_copy(ar, ap);
	
	formatLog(logmessage, " <info>: ", msg);
	
	if (cs_config.infofd != NULL) // Do we want to log this to a file?
		vfprintf(cs_config.infofd, logmessage , ap);
	
	if (cs_config.use_stderr == true) // Do we want to log this to stderr
		vfprintf(stderr, logmessage, aq);
		
	if (cs_config.use_syslog == true) 	// Do we want to log this to syslog?
		syslog(LOG_INFO, msg, ar);
	
	va_end(ap);
	va_end(aq);
	va_end(ar);
}

void logDirect(const char *msg, ...) {
	// this log command is always logged and without any additional formatting on the loginfo level
		
	va_list ap, aq, ar; // We need three of these because we cannot re-use a va_list variable
	
	va_start(ap, msg);
	va_copy(aq, ap);
	va_copy(ar, ap);
	
	if (cs_config.infofd != NULL) // Do we want to log this to a file?
		vfprintf(cs_config.infofd, msg , ap);
	
	if (cs_config.use_stderr == true) // Do we want to log this to stderr
		vfprintf(stderr, msg, aq);
		
	if (cs_config.use_syslog == true) 	// Do we want to log this to syslog?
		syslog(LOG_INFO, msg, ar);
	
	va_end(ap);
	va_end(aq);
	va_end(ar);
}

void logErr(const char *msg, ...) {
	if (cs_config.loglevel < LOGERR) 	// Do we need this loglevel?
		return;

	va_list ap, aq, ar;
	
	va_start(ap, msg);
	va_copy(aq, ap);
	va_copy(ar, ap);
	
	formatLog(logmessage, " <error>: ", msg);

	
	if (cs_config.errfd != NULL)	// Do we want to log this to a file?
		vfprintf(cs_config.errfd, logmessage, ap);

	if (cs_config.use_stderr == true) // Do we want to log this to stderr?
		vfprintf(stderr, logmessage, aq);
	
	if (cs_config.use_syslog == true) // Do we want to log this to syslog?
		syslog(LOG_ERR, msg, ar);

	va_end(ap);
	va_end(aq);
	va_end(ar);
}

void logDebug(const char *msg, ...) {
	if (cs_config.loglevel < LOGDEBUG) 	// Do we need this loglevel?
		return;
		
	va_list ap, aq, ar;
	
	va_start(ap, msg);
	va_copy(aq, ap);
	va_copy(ar, ap);

	formatLog(logmessage, " <debug>: ", msg);
	
	if (cs_config.debugfd != NULL) 	// Do we want to log this to a file?
		vfprintf(cs_config.debugfd, logmessage, ap);
	
	if (cs_config.use_stderr == true)	// Do we want to log this to stderr?
		vfprintf(stderr, logmessage, aq);

	
	if (cs_config.use_syslog == true) 	// Do we want to log this to syslog?
		syslog(LOG_DEBUG, msg, ar);

	va_end(ap);
	va_end(aq);
	va_end(ar);
}

void drawRect(int coord[2], int size[2], SDL_Surface *screen) {

	// lower line
	drawLine(coord[0], coord[1], coord[0] + size[0], coord[1], screen);
	// top line
	drawLine(coord[0], coord[1] + size[1], coord[0] + size[0], coord[1] + size[1], screen);
	// left line
	drawLine(coord[0], coord[1], coord[0], coord[1] + size[1], screen);
	// right line
	drawLine(coord[0] + size[0], coord[1], coord[0] + size[0], coord[1] + size[1], screen);
	// done


}

void drawLine(int x0, int y0, int x1, int y1, SDL_Surface *screen) {
	int step = abs(x1-x0);
	int i;
	if (abs(y1-y0) > step) step = abs(y1-y0); // this can be done faster?
	 
	float dx = (x1-x0)/(float) step;
	float dy = (y1-y0)/(float) step;

	DrawPixel(screen, x0, y0, 255, 255, 255);
	for(i=0; i<step; i++) {
		x0 = round(x0+dx); // round because integer casting would floor, resulting in an ugly line (?)
		y0 = round(y0+dy);
		DrawPixel(screen, x0, y0, 255, 255, 255); // draw directly to the screen in white
	}
}

int displayImg(float *img, int res[2], SDL_Surface *screen) {
	// ONLY does float images
	int x, y, i;
	float max=img[0];
	float min=img[0];
	
	//logDebug("Displaying image precalc, min: %f, max: %f (%f,%f).", min, max, img[0], img[100]);
	
	// we need this loop to check the maximum and minimum intensity. Do we need that? can't SDL do that?	
	for (x=0; x < res[0]*res[1]; x++) {
		if (img[x] > max)
			max = img[x];
		if (img[x] < min)
			min = img[x];
	}
	
	logDebug("Displaying image, min: %f, max: %f.", min, max);
	Slock(screen);
	for (x=0; x<res[0]; x++) {
		for (y=0; y<res[1]; y++) {
			i = (int) ((img[y*res[0] + x]-min)/(max-min)*255);
			DrawPixel(screen, x, y, \
				i, \
				i, \
				i);
		}
	}
	Sulock(screen);
	SDL_Flip(screen);
	
	return EXIT_SUCCESS;
}

void Slock(SDL_Surface *screen) {
	if ( SDL_MUSTLOCK(screen) )	{
		if ( SDL_LockSurface(screen) < 0 )
			return;
	}
}

void Sulock(SDL_Surface *screen) {
	if ( SDL_MUSTLOCK(screen) )
		SDL_UnlockSurface(screen);
}

void DrawPixel(SDL_Surface *screen, int x, int y, Uint8 R, Uint8 G, Uint8 B) {
	Uint32 color = SDL_MapRGB(screen->format, R, G, B);
	switch (screen->format->BytesPerPixel) {
		case 1: // Assuming 8-bpp
			{
				Uint8 *bufp;
				bufp = (Uint8 *)screen->pixels + y*screen->pitch + x;
				*bufp = color;
			}
			break;
		case 2: // Probably 15-bpp or 16-bpp
			{
				Uint16 *bufp;
				bufp = (Uint16 *)screen->pixels + y*screen->pitch/2 + x;
				*bufp = color;
			}
			break;
		case 3: // Slow 24-bpp mode, usually not used
			{
				Uint8 *bufp;
				bufp = (Uint8 *)screen->pixels + y*screen->pitch + x * 3;
				if(SDL_BYTEORDER == SDL_LIL_ENDIAN)
				{
					bufp[0] = color;
					bufp[1] = color >> 8;
					bufp[2] = color >> 16;
				} else {
					bufp[2] = color;
					bufp[1] = color >> 8;
					bufp[0] = color >> 16;
				}
			}
			break;
		case 4: { // Probably 32-bpp
				Uint32 *bufp;
				bufp = (Uint32 *)screen->pixels + y*screen->pitch/4 + x;
				*bufp = color;
			}
			break;
	}
}
