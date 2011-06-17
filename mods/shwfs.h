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

#include <vector>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_vector.h>
#include "types.h"

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
 
 \section shwfs_netio Camera net IO
 
 Valid commends include:
 - mla generate
 - mla find
 - mla store
 - mla del [idx]
 - mla add
 - mla get [idx]
 
 - get shifts
 
 - calibrate
 - measure
 
 \section shwfs_cfg Configuration parameters
 

 \section shwfs_todo 

 - @todo make shift_vec ringbuffer
 
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
	
	std::vector<vector_t> mlacfg;				//!< Microlens array configuration. Each element is a vector with the lower-left corner and upper-right corner of the subimage. Same order as shift_vec.
	
private:
	Shift shifts;												//!< Shift computation class. Does the heavy lifting.
	gsl_vector_float *shift_vec;				//!< SHWFS shift vector. Shift for subimage N are elements N*2+0 and N*2+1. Same order as mlacfg @todo Make this a ring buffer
	gsl_vector_float *ref_vec;					//!< SHWFS reference shift vector. Use this as 'zero' value
	
	
//	typedef std::map<float, gsl_vector_float *> act_map_t;
//	typedef std::map<int, act_map_t > inf_data_t; //!< Raw influence data, for each actuator (int actid), store a series of positions (float p) and measurements (gsl_vector_float *meas)
//	inf_data_t inf_data;
	
	// Alternative:
	//	typedef std::map<int, vector <fcoord_t>> act_map_t;
	//	typedef std::map<int, act_map_t > inf_data_t; //!< Raw influence data, for each actuator (int actid), store each measurement (int measurement_id) and a vector of all actuator signals and corresponding measurements
	//	inf_data_t inf_data;								//!< Raw influence data: inf_data[act_id][measurement_id] is a vector of (actuator signal, subap measurement) vectors. Note that each subap has two measurement_id (x and y).
	
	// Alternative2:
	typedef struct infdata {
		infdata(): init(false), nact(0), nmeas(0) {  }
		bool init;
		size_t nact;
		size_t nmeas;
		
		struct _meas {
			std::vector<float> actpos;			//!< Actuator positions (voltages) applied for each measmat
			std::vector<gsl_matrix_float *> measmat; //!< Matrices with raw measurements for infmat (should be (nmeas, nact), nmeas > nact)
			gsl_matrix *infmat;							//!< Influence matrix, represents the influence of a WFC on this Wfs (should be (nmeas, nact))
		} meas;														//!< Influence measurements
		
		struct _actmat {
			gsl_matrix_float *mat;					//!< Actuation matrix = V . Sigma^-1 . U^T (size (nact, nmeas))
			gsl_matrix *U;									//!< SVD matrix U of infmat (size (nmeas, nact))
			gsl_vector *s;									//!< SVD vector s of infmat (size (nact, 1))
			gsl_matrix *Sigma;							//!< SVD matrix Sigma of infmat (size (nact, nact))
			gsl_matrix *V;									//!< SVD matrix V of infmat (size (nact, nact))
		} actmat;													//!< Actuation matrix & related entitites
		
	} infdata_t;
	
	std::map<std::string, infdata_t> calib;	//!< Calibration data for a specific WFC.
	// calib.meas.actpos
	// calib.meas.measmat[N]
	// calib.meas.infmat
	// calib.actmat.mat
	// calib.actmat.U	
	// Reminder about (GSL) matrices: 
	// Matrix (10,5) X vector (5,1) gives a (10,1) vector
	// Matrix (10,5) X matrix (5,4) gives a (10,4) matrix
	// See <http://www.gnu.org/software/gsl/manual/html_node/Singular-Value-Decomposition.html>	
	
	Shift::method_t method;							//!< Data processing method (Center of Gravity, Correlation, etc)
	
	// Parameters for dynamic MLA grids:
	int simaxr;													//!< Maximum radius to use, or edge erosion subimages
	float simini_f;											//!< Minimum intensity for a subimage as fraction of the max intensity in a frame
	
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
	
	/*! @brief Represent the MLA configuration as one string
	 
	 @return <N> [idx x0 y0 x1 y1 [idx x0 y0 x1 y1 [...]]]
	 */
	string get_mla_str() const;
	
	/*! @brief Set MLA configuration from string, return number of subaps, reverse of get_mla_str(). Output stored in mlacfg.
	 
	 @param [in] <N> [idx x0 y0 x1 y1 [idx x0 y0 x1 y1 [...]]]
	 @return Number of subimages successfully added (might be != N)
	 */
	int set_mla_str(string mla_str);
	
	int mla_subapsel();
	
	//!< Calculate influence for each Zernike mode
//	int calc_zern_infl(int nmodes);
	//!< Calculate slopes (helper for calc_zern_infl())
//	int calc_slope(gsl_matrix *tmp, std::vector<vector_t> &mlacfg, double *slope); 
	
	//!< Represent SHWFS shifts as a string [<N> [idx Sx0 Sy0 Sx1 Sy1 [idx Sx0 Sy0 Sx1 Sy1 [...]]]
	string get_shifts_str() const;
	
public:
	Shwfs(Io &io, foamctrl *const ptc, const string name, const string port, Path const &conffile, Camera &wfscam, const bool online=true);
	~Shwfs();
	
	/*! @brief Generate subaperture/subimage (sa/si) positions for a given configuration.

	 @param [in] mlacfg The calculated subaperture pattern will be stored here
	 @param [in] res Resolution of the sa pattern (before scaling) [pixels]
	 @param [in] size Size of the sa's [pixels]
	 @param [in] pitch Pitch of the sa's [pixels]
	 @param [in] xoff The horizontal position offset of odd rows [fraction of pitch]
	 @param [in] disp Global displacement of the sa positions [pixels]
	 @param [in] shape Shape of the pattern, circular or square (see mlashape_t)
	 @param [in] overlap How much overlap with aperture needed for inclusion (0--1)
	 @return Number of subapertures found
	 */
	int gen_mla_grid(std::vector<vector_t> &mlacfg, const coord_t res, const coord_t size, const coord_t pitch, const int xoff, const coord_t disp, const mlashape_t shape, const float overlap);

	/*! @brief Find subaperture/subimage (sa/si) positions in a given frame.
	 
	 This function takes a frame from the camera and finds the brightest spots to use as MLA grid
	 
	 @param [in] mlacfg The calculated subaperture pattern will be stored here
	 @param [in] size Size of the sa's [pixels]
	 @param [in] mini_f Minimimum intensity a pixel has to have to consider it, as fraction of the maximum intensity.
	 @param [in] nmax Maximum number of SA's to search
	 @param [in] iter Number of iterations to do
	 @return Number of subapertures found
	 */
	int find_mla_grid(std::vector<vector_t> &mlacfg, const coord_t size, const float mini_f=0.6, const int nmax=-1, const int iter=1);
	
	//!< Store MLA grid to disk, as CSV
	bool store_mla_grid(const bool overwrite=false) const;
	
	int mla_update_si(const int nx0, const int ny0, const int nx1, const int ny1, const int idx=-1);
	int mla_del_si(const int idx);

	/*! @brief Convert shifts to basis functions
	 
	 Based on 'invec', which is a vector of image shifts, calculated 'outvec', a
	 vector of measurements in a specific basis. Basis can be things like 
	 Zernike, Karhunen-Loéve, or simply 'sensor' modes.
	 
	 @param [in] *invec Inputvector of shift measurements
	 @param [in] basis Basis to convert *invec to
	 @param [out] *outvec Vector of measurements in specific basis
	 */
	int shift_to_basis(const gsl_vector_float *const invec, const wfbasis basis, gsl_vector_float *outvec);

	/*! @brief Compute control vector 
	 
	 Calculate control vector for a specific wavefront corrector based on 
	 previously determined influence function (from build_infmat() and
	 calc_actmat()).
	 
	 @param [in] wfcname Name of the wavefront corrector to be used.
	 @param [in] *shift Vector of measured shifts
	 @param [out] *act Generalized actuator commands for wfcname (pre-allocated)
	 @return Computed control vector
	 */
	gsl_vector_float *comp_ctrlcmd(string wfcname, gsl_vector_float *shift, gsl_vector_float *act);
	
	/*! @brief Initialize influence matrix, allocate memory
	 
	 Before we can measure the influence function of a WFC-WFS pair, we need to
	 allocate some memory. This is done here. 
	 
	 First, we need a specific wfcname to calibrate against. Then we need the 
	 number of actuators in that WFC and finally we need the positions that will
	 be actuated.
	 
	 @param [in] wfcname Name of the WFC this WFS is calibrated with
	 @param [in] nact Number of actuators in the WFC
	 @param [in] &actpos List of positions that will be actuated
	 */
	void init_infmat(string wfcname, size_t nact, vector <float> &actpos);
	
	/*! @brief Build influece matrix
	 
	 Given a specific wfc actuation (wfcact), and the data that produces 
	 (frame), compute part of the influence matrix.
	 
	 The influence matrix is a matrix that represents the influence of a Wfc
	 on the measurements of a Wfs, i.e. given an arbitrary Wfc actuation vector,
	 this matrix calculates the data vector that will be measured. The inverse
	 is used to drive a Wfc given Wfs measurements and is called the actuation
	 matrix.

	 @param [in] wfcname Name of the WFC this WFS is calibrated with
	 @param [in] *frame Captured camera frame
	 @param [in] actid ID of the actuated actuator
	 @param [in] actposid ID of the position the actuator was set to (in calib[].meas.actpos)
	 */
	int build_infmat(string wfcname, Camera::frame_t *frame, int actid, int actposid);

	/*! @brief After getting enough data with build_infmat, construct the influence matrix
	 
	 @param [in] wfcname Name of the WFC this WFS is calibrated with
	 */
	int calc_infmat(string wfcname);
	
	/*! @brief Calculate actuation matrix to drive Wfc, using SVD
	 
	 @param [in] wfcname Name of the WFC this WFS is calibrated with
	 @param [in] singval How much singular value to include (0 to 1, 0.8 or lower is generally not recommended)
	 @param [in] basis Basis for which singval counts
	 */	 
	int calc_actmat(string wfcname, double singval=1.0, enum wfbasis basis = SENSOR);
	
	/*! @brief Set this measurement as reference or 'flat' wavefront
	 
	 @param [in] *frame Captured camera frame
	 */
	void set_reference(Camera::frame_t *frame);
	
	void store_reference(); //!< Store reference vector to .csv file in ptc->datadir
	
	// From Wfs::
	wf_info_t* measure(Camera::frame_t *frame=NULL);
	virtual int calibrate();
	
	// From Devices::
	virtual void on_message(Connection *const, string);
};

#endif // HAVE_SHWFS_H

/*!
 \page dev_wfs_shwfs Shack-Hartmann Wavefront sensor devices
 
 The Shwfs class provides control for SH-wavefront sensors.
 
 */ 