// Some definities necessary to run modules on their own

#ifndef FOAM_MODULES_ALONE
#define FOAM_MODULES_ALONE

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
} filter_t;

#include "foam_cs_library.h"		

#endif //#ifdef FOAM_MODULES_ALONE
