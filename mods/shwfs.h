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

#include "wfc.h"
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
 microlenses on the CCD). It is the <i>subimages</i> we are interested in when
 processing the CCD data, but the subapertures are the physical apertures 
 (usually lenses) which split up the main telescope aperture. 
 
 This class extends on Wfs:: see the base class for more information.
 
 \section shwfs_netio Network IO
 
 Valid commends include:
 - mla generate: generate microlens array (MLA) pattern
 - mla find [simini_f] [sisize] [nmax] [iter]: heuristically find MLA 
 - mla store: store MLA pattern to disk
 - mla del <idx>: delete MLA subimage 'idx'
 - mla add <lx> <ly> <tx> <ty>: add MLA subimage with given coordinates
 - mla get <idx>: get MLA subimage coordinates
 
 - get shifts: return measured shift vectors
 
 \section shwfs_cfg Configuration parameters
 
 - sisize{x,y}: Shwfs::sisize
 - sipitch{x,y}: Shwfs::sipitch
 - disp{x,y}: Shwfs::disp
 - overlap: Shwfs::overlap
 - xoff: Shwfs::xoff
 - shape: Shwfs::shape
 - simaxr: Shwfs::simaxr
 - simini_f: Shwfs::simini_f
 
 */
class Shwfs: public Wfs {
	//! @todo this is a bit weird, why only SimulCam?
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
	gsl_vector_float *tot_shift_vec;		//!< Total SHWFS shift being corrected, as calculated from the WFC control vector.
	
	typedef struct infdata {
		infdata(): init(false), nact(0), nmeas(0) {  }
		bool init;
		size_t nact;
		size_t nmeas;
		
		struct _meas {
			_meas(): infmat(NULL), infmat_f(NULL) { }
			std::vector<float> actpos;			//!< Actuator positions (voltages) applied for each measmat
			std::vector<gsl_matrix_float *> measmat; //!< Matrices with raw measurements for infmat (should be (nmeas, nact), nmeas > nact)
			gsl_matrix *infmat;							//!< Influence matrix, represents the influence of a WFC on this Wfs (should be (nmeas, nact))
			gsl_matrix_float *infmat_f;			//!< Influence matrix stored as float
		} meas;														//!< Influence measurements
		
		struct _actmat {
			_actmat(): mat(NULL), U(NULL), s(NULL), Sigma(NULL), V(NULL) { }
			gsl_matrix_float *mat;					//!< Actuation matrix = V . Sigma^-1 . U^T (size (nact, nmeas))
			gsl_matrix *U;									//!< SVD matrix U of infmat (size (nmeas, nact))
			gsl_vector *s;									//!< SVD vector s of infmat (size (nact, 1))
			gsl_matrix *Sigma;							//!< SVD matrix Sigma of infmat (size (nact, nact))
			gsl_matrix *V;									//!< SVD matrix V of infmat (size (nact, nact))
			
			int use_nmodes;									//!< Number of modes to use
			double use_singval;							//!< Amount of singular values used (i.e. 1.0 for all, 0.5 for half the power in all modes)
			double condition;								//!< SVD condition, i.e. singval[0]/singval[-1]
		} actmat;													//!< Actuation matrix & related entitites (inverse of influence measurements)
		
	} infdata_t;
	
	std::map<std::string, infdata_t> calib;	//!< Calibration data for a specific WFC.
	// Reminder about (GSL) matrices: 
	// Matrix (10,5) X vector (5,1) gives a (10,1) vector
	// Matrix (10,5) X matrix (5,4) gives a (10,4) matrix
	// See <http://www.gnu.org/software/gsl/manual/html_node/Singular-Value-Decomposition.html>	
	
	Shift::method_t method;							//!< Data processing method (Center of Gravity, Correlation, etc)
	
	// Parameters for dynamic MLA grids:
	int simaxr;													//!< Maximum radius to use, or edge erosion subimages
	float simini_f;											//!< Minimum intensity for a subimage as fraction of the max intensity in a frame
	float shift_mini;										//!< Minimum intensity to consider when calculating centroiding positions
	
	// Parameters for static MLA grids:
	coord_t sisize;											//!< Subimage size in pixels
	coord_t sipitch;										//!< Pitch between subimages in pixels (if sipitch == Shwfs::sisize, the subimages are exactly adjacent to eachother)
	coord_t disp;												//!< Displacement of whole subimage pattern in pixels
	float overlap;											//!< Minimum amount a subimage should be inside the crop area in order to be taken into account.
	int xoff;														//!< Odd row offset between lenses in pixels
	mlashape_t shape;										//!< MLA cropping shape ('square' for SQUARE or 'circular' CIRCULAR)
	
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
	 
	 @param [in] mla_str <N> [idx x0 y0 x1 y1 [idx x0 y0 x1 y1 [...]]]
	 @return Number of subimages successfully added (might be != N)
	 */
	int set_mla_str(string mla_str);
	
	int mla_subapsel();
	
	/*! @brief Represent SHWFS shifts as a string
	 
	 String representing SHWFS shift vector, including subaperture center 
	 coordinates and reference shift. Syntax:
	 
	   [<N> [idx subap0_x subap0_y ref0_x ref0_y sh0_x sh0_y [idx subap1_x subap1_y ref1_x ref1_y sh1_x sh1_y [...]]]
	 
	 The origin of the reference shift is the exact center of the subaperture. 
	 The origin of the shift vector is the end of the reference vector. With 
	 respect to the center of the subaperture, the spot centroid is located at
	 ref_vec + shift_vec.
	 */
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
	
	/*! @brief Calibrate influence function between this WFS and *wfc using *cam
	 
	 @param [in] *wfc Wavefront corrector to calculate influence function for
	 @param [in] *cam Camera to use for influence calculation
	 @param [in] &actpos Actuator positions to use for measuring the influence function
	 */
	int calib_influence(Wfc *wfc, Camera *cam, const vector <float> &actpos, const double sval_cutoff);
	
	/*! @brief Calibrate influence function between this WFS and *wfc using *cam
	 
	 @param [in] *wfc Wavefront corrector to calculate influence function for
	 @param [in] *cam Camera to use for influence calculation
	 */
	int calib_zero(Wfc *wfc, Camera *cam);
	
	/*! @brief Add offset vector to correction
	 
	 This calibration requires ref_vec to be measured and set,
	 
	 @param [in] x x-offset to add
	 @param [in] y y-offset to add
	 @returns 0 if successful, !0 otherwise
	 */
	int calib_offset(double x, double y);

	/*! @brief Given shifts, compute control vector 
	 
	 Calculate control vector for a specific wavefront corrector based on 
	 previously determined influence function (from build_infmat() and
	 calc_actmat()).
	 
	 @param [in] wfcname Name of the wavefront corrector to be used.
	 @param [in] *shift Vector of measured shifts
	 @param [out] *act Generalized actuator commands for wfcname (pre-allocated)
	 @return Computed control vector
	 */
	gsl_vector_float *comp_ctrlcmd(const string &wfcname, const gsl_vector_float *shift, gsl_vector_float *act);
	
	/*! @brief Given a control vector, calculate shifts
	 
	 This is meant for debugging purposes only. Given a calculated control 
	 vector, this routine calculates the shifts that will be generated if the
	 WFC were to apply it, given the actuation matrix. It serves as a test to
	 see if the back-calculated shifts correspond to the measured shifts.
	 
	 If shift is NULL, use tot_shift_vec instead.

	 @param [in] wfcname Name of the wavefront corrector to be used.
	 @param [in] *act Generalized actuator commands for wfcname 
	 @param [out] *shift Vector of calculated shifts (pre-allocated)
	 @return Vector of calculated shifts
	 */
	gsl_vector_float *comp_shift(const string &wfcname, const gsl_vector_float *act, gsl_vector_float *shift);
	
	/*! @brief Given a shift vector, calculate the tip-tilt
	 
	 To off-load tip-tilt to a telescope, we need to get the tip-tilt 
	 information from the system. This is done by simply summing all x- and 
	 y-shifts in a shiftvector. The summed x-, y-shifts is added to tty and tty,
	 so make sure they have sane values before calling this function.
	 
	 If shift is NULL, use tot_shift_vec instead.
	 
	 @param [in] *shift Total shift vector
	 @param [out] *ttx Shift in x-direction
	 @param [out] *tty Shift in y-direction
	 */
	void comp_tt(const gsl_vector_float *shift, float *ttx, float *tty);
	
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
	void init_infmat(const string &wfcname, const size_t nact, const vector <float> &actpos);
	
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
	int calc_infmat(const string &wfcname);
	
	/*! @brief Calculate actuation matrix to drive Wfc, using SVD
	 
	 'Singval' can be used to tweak the SVD results:
	 if singval = 0: abort
	 if singval < 0: drop 'singval' number of modes (i.e. if nmodes = 50, singval = -5, use 45 modes)
	 if singval > 1: use this amount of modes
	 else: singval is between 0 and 1, use this amount of singular value in the reconstruction
	 
	 @param [in] wfcname Name of the WFC this WFS is calibrated with
	 @param [in] singval How much singular value/modes to include
	 @param [in] basis Basis for which singval counts
	 */	 
	int calc_actmat(const string &wfcname, const double singval, const bool check_svd=true, const enum wfbasis basis = SENSOR);
	
	/*! @brief Represent singular value array as string
	 
	 @return <N> <s1> <s2> ... <sN>
	 */
	string get_singval_str(const string &wfcname) const;
	
	/*! @brief Return condition of a specific SVD
	 
	 @return singval[0]/singval[-1]
	 */
	double get_svd_cond(const string &wfcname) const;
	
	/*! @brief Return number of modes used in this decomposition
	 @return <N> number of modes
	 */
	int get_svd_modeuse(const string &wfcname) const;
	
	/*! @brief Return singular value used in this decomposition
	 @return sum i=0...modes_used (singval[i]/sum(singval))
	 */
	double get_svd_singuse(const string &wfcname) const;
	
	/*! @brief Return reference vector as string
	 
	 @return <N> <refx1> <refy1> <refx2> <refy2> ... <refNx> <refNy>
	 */
	string get_refvec_str() const;

	/*! @brief Check subimage sanity, should be inside frame
	 */
	int check_subimgs(const vector_t &bounds) const;
	int check_subimgs(const coord_t &topbounds) const;
	
	/*! @brief Set this measurement as reference or 'flat' wavefront
	 
	 @param [in] *frame Captured camera frame
	 */
	void set_reference(Camera::frame_t *frame);
	
	void store_reference(); //!< Store reference vector to a .csv file in ptc->datadir
	
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