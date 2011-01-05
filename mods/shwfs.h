/*
 shwfs.h -- Shack-Hartmann utilities class header file
 Copyright (C) 2009--2011 Tim van Werkhoven <t.i.m.vanwerkhoven@xs4all.nl>
 
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
#include "shift.h"

using namespace std;

const string shwfs_type = "shwfs";

// CLASS DEFINITION //
/********************/

/*!
 @brief Shack-Hartmann wavefront sensor class
 
 Note the difference between subapertures (i.e. the physical microlenses 
 usually used in SHWFS) and subimages (i.e. the images formed by the 
 microlenses on the CCD). It is the subimages we are interested in when
 processing the CCD data.
 
 \section cam_netio Camera net IO
 
 Valid commends include:
 - mla generate
 - mla find
 - mla store
 - mla del
 - mla add
 
 \section cam_cfg Configuration parameters
 

 */
class Shwfs: public Wfs {
	friend class SimulCam;
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
	
	typedef struct sh_subimg {
		coord_t llpos;										//!< Lower-left position of this subimg (pos - size/2)
		coord_t size;											//!< Subaperture size (pixels)
		size_t id;
		sh_subimg(): llpos(0,0), size(0,0), id(0) { ; }
	} sh_simg_t;												//!< MLA subimage definition on CCD
	
	typedef struct sh_mla {
		int nsi;													//!< Number of microlenses (subapertures)
		float f;													//!< Microlens focal length
		sh_simg_t *ml;										//!< Array of microlens positions
		sh_mla(): nsi(0), f(-1.0), ml(NULL) { ; }
	} sh_mla_t;													//!< Microlens array struct

private:
	Shift shifts;												//!< Shift computation class. Does the heavy lifting.
	gsl_vector_float *shift_vec;				//!< SHWFS shift vector
	
	Shift::method_t method;							//!< Data processing method (Center of Gravity, Correlation, etc)
	
	sh_mla_t mlacfg;										//!< Subimages configuration (coordinates & sizes)
	
	// Parameters for dynamic MLA grods:
	int simaxr;													//!< Maximum radius to use, or edge erosion subimages
	int simini;													//!< Minimum intensity in
	
	// Parameters for static MLA grids:
	coord_t sisize;											//!< Subimage size
	coord_t sipitch;										//!< Pitch between subimages
	coord_t disp;												//!< Displacement of complete pattern
	float overlap;											//!< Overlap required 
	int xoff;														//!< Odd row offset between lenses
	mlashape_t shape;										//!< MLA Shape (SQUARE or CIRCULAR)
	
	/*! @brief Find maximum intensity & index of img

	 @param [in] *img Image to scan
	 @param [in] nel Number of pixels in image
	 @param [out] idx Index of maximum intensity pixel
	 @return Maximum intensity
	 */
	template <class T> int _find_max(const T *const img, const size_t nel, size_t *idx);
	
	string get_mla_str(const sh_mla_t mla) const; //!< Represent a MLA configuration as one string
	string get_mla_str() const { return get_mla_str(mlacfg); }
	int set_mla_str(string mla_str); //!< Set MLA configuration from string, return number of subaps
		
	int mla_subapsel();	
	
public:
	Shwfs(Io &io, foamctrl *const ptc, const string name, const string port, Path const &conffile, Camera &wfscam, const bool online=true);
	~Shwfs();	
	
	/*! @brief Generate subaperture/subimage (sa/si) positions for a given configuration.

	 @param [in] *mla The calculated subaperture pattern
	 @param [in] res Resolution of the sa pattern (before scaling) [pixels]
	 @param [in] size Size of the sa's [pixels]
	 @param [in] pitch Pitch of the sa's [pixels]
	 @param [in] xoff The horizontal position offset of odd rows [fraction of pitch]
	 @param [in] disp Global displacement of the sa positions [pixels]
	 @param [in] shape Shape of the pattern, circular or square (see mlashape_t)
	 @param [in] overlap How much overlap with aperture needed for inclusion (0--1)
	 @return Number of subapertures found
	 */
	int gen_mla_grid(sh_mla_t *const mla, const coord_t res, const coord_t size, const coord_t pitch, const int xoff, const coord_t disp, const mlashape_t shape, const float overlap);

	/*! @brief Find subaperture/subimage (sa/si) positions in a given frame.
	 
	 This function takes a frame from the camera and finds the brightest spots to use as MLA grid
	 
	 @param [in] *mla The calculated subaperture pattern
	 @param [in] size Size of the sa's [pixels]
	 @param [in] mini Minimimum intensity in a SA pattern
	 @param [in] nmax Maximum number of SA's to search
	 @param [in] iter Number of iterations to do
	 @return Number of subapertures found
	 */
	int find_mla_grid(sh_mla_t *const mla, const coord_t size, const int mini=0, const int nmax=-1, const int iter=1);
	 
	bool store_mla_grid(const sh_mla_t mla, const Path &f, const bool overwrite=false) const;	//!< Store external MLA grid to disk, as CSV
	bool store_mla_grid(const Path &f, const bool overwrite=false) const;	//!< Store this MLA grid to disk, as CSV

	//!< Convert shifts to basis functions
	int shift_to_basis(const gsl_vector_float *const invec, const wfbasis basis, gsl_vector_float *outvec);
	
	// From Wfs::
	virtual int measure();
	virtual int calibrate();
	
	// From Devices::
	virtual void on_message(Connection *const, string);
};

#endif // HAVE_SHWFS_H

/*!
 \page dev_wfs_shwfs Shack-Hartmann Wavefront sensor devices
 
 The Shwfs class provides control for SH-wavefront sensors.
 
 */ 