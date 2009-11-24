/*
 Copyright (C) 2008 Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)
 
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

 $Id$
 */
/*! 
 @file foam_modules-sh.h
 @author Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)
 @date 2008-07-15
 
 @brief This header file prototypes functions used in SH wavefront sensing.
  */

#ifndef __SHTRACK_H__
#define __SHTRACK_H__

// LIBRABRIES //
/**************/

#include "foam.h"
#include "foamctrl.h"
#include "types.h"
#include "imgio.h"

typedef struct {
	int ns;											// Number of subapertures

	coord_t size;								// Pixel resolution of the WFS
	coord_t cells;							// Grid resolution (number of cells)
	coord_t shsize;							// Pixels per cell
	coord_t track;							// Pixels used for tracking
	int samaxr;									// Max radius / edge erosion
	int samini;									// Minimum intensity per 

	coord_t *subc;							// Tracking window positions
	coord_t *cellc;							// Cell position
	gsl_vector_float *refc;			// Reference position
	gsl_vector_float *disp;			// Displacement measured

	fcoord_t offs;							// Global offset coordinate to use
} shwfs_t;


class Shtrack {
	string phfile;
	string inffile;
public:
	Shtrack(int, int);
	Shtrack(coord_t);
	~Shtrack(void);

	shwfs_t *sh;
	
	void setShsize(int w, int h) { sh->shsize.x = w; sh->shsize.y = h; }
	void setShsize(coord_t size) { sh->shsize = size; }
	void setSize(coord_t size) { sh->size = size; }
	void setTrack(float f) { setTrack(f,f); }
	void setTrack(float fx, float fy) { 
		sh->track.x = sh->shsize.x * fx;
		sh->track.y = sh->shsize.y * fy;
	}
	void setSamaxr(int maxr) { sh->samaxr = maxr; }
	void setSamini(int i) { sh->samini = i; }
	void setPHFile(string f) { phfile = f; }
	void setInfFile(string f) { inffile = f; }
	
//	int selSubaps(wfs_t *wfs);
//	int cogFind(wfs_t *wfs);
};

#endif /* __SHTRACK_H__ */
