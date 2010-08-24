/*
 shwfs.h -- Shack-Hartmann utilities class header file
 Copyright (C) 2009--2010 Tim van Werkhoven <t.i.m.vanwerkhoven@xs4all.nl>
 
 This file is part of FOAM.
 
 FOAM is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 2 of the License, or
 (at your option) any later version.
 
 FOAM is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with FOAM.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef HAVE_SHWFS_H
#define HAVE_SHWFS_H

#include "cam.h"
#include "io.h"
#include "wfs.h"

const string shwfs_type = "shwfs";

// CLASS DEFINITION //
/********************/

/*!
 @brief TODO Shack-Hartmann wavefront sensor class
 */
class Shwfs: public Wfs {
private:
	coord_t subap;											//!< Number of subapertures in X and Y
	coord_t sasize;											//!< Subaperture size (pixels)
	coord_t track;											//!< Subaperture tracking window size (pixels)
	int samaxr;													//!< Maximum radius to use, or edge erosion subapertures
	int samini;													//!< Minimum amount of subapertures to use
	
	int mode;													//!< Data processing mode (Center of Gravity, Correlation, etc)
	
	fcoord_t *trackpos;
	coord_t *sapos;
	coord_t *shifts;
	
	int ns;
	
	template <class T> uint32_t _cog(T *img, int xpos, int ypos, int w, int h, int stride, uint32_t samini, fcoord_t& cog);
	template <class T> int _cogframe(T *img);
	
	int subapSel();	
	void printGrid(int *map);
	
public:
	// Public datatypes
	typedef enum {
		CAL_SUBAPSEL=0,
		CAL_PINHOLE
	} wfs_cal_t;												//!< Different calibration methods
	
	typedef enum {
		COG=0,
		CORR
	} mode_t;														//!< Different SHWFS operation modes
	
	Shwfs(Io &io, string conffile);
	~Shwfs();	
	
	virtual int verify(int);
	virtual int calibrate(int);
	virtual int measure(int);
	virtual int configure(wfs_prop_t *);
};

#endif // HAVE_SHWFS_H
