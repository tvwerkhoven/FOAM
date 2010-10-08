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

#include "camera.h"
#include "io.h"
#include "wfs.h"

const string shwfs_type = "shwfs";
const int shwfs_maxlenses = 128;

// CLASS DEFINITION //
/********************/

/*!
 @brief Shack-Hartmann wavefront sensor class
 
 Note the difference between subapertures (i.e. the physical microlenses 
 usually used in SHWFS) and subimages (i.e. the images formed by the 
 microlenses on the CCD). It is the subimages we are interested in when
 processing the CCD data.
 
 @todo Document this
 */
class Shwfs: public Wfs {
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
	
	typedef struct sh_subimg {
		coord_t pos;											//!< Centroid position of this subimg
		coord_t llpos;										//!< Lower-left position of this subimg (pos - size/2)
		coord_t size;											//!< Subaperture size (pixels)
		coord_t track;										//!< Subaperture tracking window size
		sh_subimg(): pos(0,0), size(0,0), track(0,0) { ; }
	} sh_simg_t; //!< Subimage definition on CCD

private:
	sh_simg_t mla[shwfs_maxlenses];			//!< Subimages coordinates & sizes
	
	int simaxr;													//!< Maximum radius to use, or edge erosion subimages
	int simini;													//!< Minimum intensity in
	
	mode_t mode;												//!< Data processing mode (Center of Gravity, Correlation, etc)
	
	fcoord_t *trackpos;
	coord_t *sapos;
	coord_t *shifts;
	
	int ns;
	
	template <class T> uint32_t _cog(T *img, int xpos, int ypos, int w, int h, int stride, uint32_t samini, fcoord_t& cog);
	template <class T> int _cogframe(T *img);
	
	int subapSel();	
	void printGrid(int *map);
	
public:
	
	Shwfs(Io &io, string name, string port, string conffile);
	~Shwfs();	
	
	virtual int verify(int);
	virtual int calibrate(int);
	virtual int measure(int);
};

#endif // HAVE_SHWFS_H
