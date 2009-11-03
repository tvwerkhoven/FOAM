/*
 Copyright (C) 2008 Tim van Werkhoven (tvwerkhoven@xs4all.nl)
 
 This file is part of FOAM.
 
 FOAM is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 FOAM is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with FOAM.  If not, see <http://www.gnu.org/licenses/>.
 */
/*! 
 @file foam_modules-alone.h
 @author Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)
 @date 2008-07-15 19:12
 
 @brief This header file includes some definitions necessary to run modules on their own.
 
 This header file declares some simple things that are necessary to compile
 modules on their own. Usually modules are used from a prime module, and in that
 case the prime module header file provides these definitions. Since we 
 sometimes want to run modules on their own, this is a sort of 'dummy' header 
 file providing very basic definitions.
 
 Compiling modules alone is done in case of hardware debugging, see the modules
 themselves for details.
 */

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
