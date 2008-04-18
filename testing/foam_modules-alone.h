// Some definities necessary to run modules on their own

#define FILENAMELEN 64
#define COMMANDLEN 1024

#define MAX_CLIENTS 8
#define MAX_THREADS 4
#define MAX_FILTERS 8

typedef enum { // calmode_t
	CAL_PINHOLE,	
	CAL_INFL,		
	CAL_LINTEST	
} calmode_t;


typedef enum { // axes_t
	WFC_TT=0,		
	WFC_DM=1		
} wfctype_t;

typedef enum { //fwheel_t
	FILT_PINHOLE,	
	FILT_OPEN,		
	FILT_CLOSED		
} fwheel_t;

#include "foam_cs_library.h"		

void logErr(const char *msg, ...) {
	va_list ap;
	va_start(ap, msg);
	vfprintf(stderr, msg, ap);
	fprintf(stderr, "\n");
}
void logWarn(const char *msg, ...) {
	va_list ap;
	va_start(ap, msg);
	vfprintf(stderr, msg, ap);
	fprintf(stderr, "\n");
}
void logInfo(int flg, const char *msg, ...) {
	va_list ap;
	va_start(ap, msg);
	vfprintf(stderr, msg, ap);
	fprintf(stderr, "\n");
}
void logDebug(int flg, const char *msg, ...) {
	va_list ap;
	va_start(ap, msg);
	vfprintf(stderr, msg, ap);
	fprintf(stderr, "\n");
}
