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

using namespace std;

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
		SQUARE=0,
		CIRCULAR,
	} mlashape_t;
	
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
		sh_subimg(): pos(0,0), llpos(0,0), size(0,0), track(0,0) { ; }
	} sh_simg_t;												//!< MLA subimage definition on CCD
	
	typedef struct sh_mla {
		int nsi;													//!< Number of microlenses (subapertures)
		float f;													//!< Microlens focal length
		Shwfs::sh_simg_t *ml;							//!< Array of microlens positions
		sh_mla(): nsi(0), f(-1.0), ml(NULL) { ; }
	} sh_mla_t;													//!< Microlens array struct

private:
	sh_mla_t mla;												//!< Subimages coordinates & sizes
	
	int simaxr;													//!< Maximum radius to use, or edge erosion subimages
	int simini;													//!< Minimum intensity in
	
	mode_t mode;												//!< Data processing mode (Center of Gravity, Correlation, etc)
	
	//coord_t *shifts;										//!< subap shifts @todo fix this properly
	
	//template <class T> uint32_t _cog(T *img, int xpos, int ypos, int w, int h, int stride, uint32_t simini, fcoord_t& cog);
	//template <class T> int _cogframe(T *img);
	
	int mla_subapsel();	
	
public:
	Shwfs(Io &io, foamctrl *ptc, string name, string port, Path &conffile, Camera &wfscam);
	~Shwfs();	
	
	/*! @brief Generate subaperture/subimage (sa/si) positions for a given configuration.
	 
	 @param [in] res Resolution of the sa pattern (before scaling) [pixels]
	 @param [in] size size of the sa's [pixels]
	 @param [in] pitch pitch of the sa's [pixels]
	 @param [in] xoff the horizontal position offset of odd rows [fraction of pitch]
	 @param [in] disp global displacement of the sa positions [pixels]
	 @param [in] shape shape of the pattern, circular or square (see mlashape_t)
	 @param [in] overlap how much overlap with aperture needed for inclusion (0--1)
	 @param [out] *pattern the calculated subaperture pattern
	 @return number of subapertures found
	 */
	sh_simg_t *gen_mla_grid(coord_t res, coord_t size, coord_t pitch, int xoff, coord_t disp, mlashape_t shape, float overlap, int &nsubap);
	
	bool store_mla_grid(sh_mla_t mla, Path &f, bool overwrite=false);	
	bool store_mla_grid(Path &f, bool overwrite=false);
//	virtual int verify(int);
//	virtual int calibrate(int);
//	virtual int measure(int);
};

#endif // HAVE_SHWFS_H
