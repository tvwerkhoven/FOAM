/*
 shwfs.cc -- Shack-Hartmann utilities class
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

#include <stdint.h>
#include <math.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_vector.h>
#include <gsl/gsl_blas.h>
#include <gsl/gsl_linalg.h>
#include <string.h>


#include "config.h"
#include "csv.h"
#include "types.h"
#include "camera.h"
#include "io.h"
#include "path++.h"

#include "wfs.h"
#include "shwfs.h"
#include "shift.h"

using namespace std;

// PUBLIC FUNCTIONS //
/********************/

Shwfs::Shwfs(Io &io, foamctrl *const ptc, const string name, const string port, Path const &conffile, Camera &wfscam, const bool online):
Wfs(io, ptc, name, shwfs_type, port, conffile, wfscam, online),
shifts(io, 4),
shift_vec(NULL), ref_vec(NULL),
method(Shift::COG)
{
	io.msg(IO_DEB2, "Shwfs::Shwfs()");
	add_cmd("mla generate");
	add_cmd("mla find");
	add_cmd("mla store");
	add_cmd("mla del");
	add_cmd("mla add");
	add_cmd("mla get");
	add_cmd("mla set");

	add_cmd("get shifts");
	
	add_cmd("calibrate");
	add_cmd("measure");
	
	//! @todo Move microlens array configuration to separate class
	mlacfg.reserve(128);
	
	// Micro lens array parameters:	
	sisize.x = cfg.getint("sisizex", 16);
	sisize.y = cfg.getint("sisizey", 16);
	if (cfg.exists("sisize"))
		sisize.x = sisize.y = cfg.getint("sisize");
	
	sipitch.x = cfg.getint("sipitchx", 64);
	sipitch.y = cfg.getint("sipitchy", 64);
	if (cfg.exists("sipitch"))
		sipitch.x = sipitch.y = cfg.getint("sipitch");
	
	disp.x = cfg.getint("dispx", cam.get_width()/2);
	disp.y = cfg.getint("dispy", cam.get_height()/2);
	if (cfg.exists("disp"))
		disp.x = disp.y = cfg.getint("disp");
	
	overlap = cfg.getdouble("overlap", 0.5);
	xoff = cfg.getint("xoff", 0);
	
	string shapestr = cfg.getstring("shape", "square");
	if (shapestr == "circular")
		shape = CIRCULAR;
	else
		shape = SQUARE;
	
	// Other paramters:
	simaxr = cfg.getint("simaxr", -1);
	simini_f = cfg.getdouble("simini_f", 0.6);
	
	// Generate MLA grid
	gen_mla_grid(mlacfg, cam.get_res(), sisize, sipitch, xoff, disp, shape, overlap);
	
	// Initial calibration
	calibrate();
}

Shwfs::~Shwfs() {
	io.msg(IO_DEB2, "Shwfs::~Shwfs()");
	
	gsl_vector_float_free(shift_vec);
	gsl_vector_float_free(ref_vec);
	
	std::map<std::string, infdata_t>::iterator it;
	for (it=calib.begin(); it != calib.end(); it++ ) {
		//cout << (*it).first << " => " << (*it).second << endl;
		infdata_t curdat = (*it).second;
		
		// Delete calib[wfcname].meas.measmat array
		std::vector<gsl_matrix_float *> tmpvec = curdat.meas.measmat;
		std::vector<gsl_matrix_float *>::iterator vit;
		for (vit=tmpvec.begin(); vit < tmpvec.end(); vit++ )
			gsl_matrix_float_free(*vit);
		
		gsl_matrix_free(curdat.meas.infmat);
		gsl_matrix_float_free(curdat.meas.infmat_f);
		
		gsl_matrix_float_free(curdat.actmat.mat);
		gsl_matrix_free(curdat.actmat.U);
		gsl_vector_free(curdat.actmat.s);
		gsl_matrix_free(curdat.actmat.Sigma);
		gsl_matrix_free(curdat.actmat.V);
	}
}

void Shwfs::on_message(Connection *const conn, string line) {
	string orig = line;
	string command = popword(line);
	bool parsed = true;
	
	if (command == "mla") {
		string what = popword(line);

		if (what == "generate") {					// mla generate
			//! @todo get extra options from line
			conn->addtag("mla");
			gen_mla_grid(mlacfg, cam.get_res(), sisize, sipitch, xoff, disp, shape, overlap);
		} else if(what == "find") {				// mla find [sisize] [simini_f] [nmax] [iter]
			conn->addtag("mla");
			int nmax=-1, iter=1, tmp = popint(line);
			if (tmp > 0) sisize = tmp;
			tmp = popdouble(line);
			if (tmp > 0) simini_f = tmp;
			tmp = popint(line);
			if (tmp > 0) nmax = tmp;
			tmp = popint(line);
			if (tmp > 0) iter = tmp;
			
			find_mla_grid(mlacfg, sisize, simini_f, nmax, iter);
		} else if(what == "store") {			// mla store [reserved] [overwrite]
			popword(line);
			if (popword(line) == "overwrite")
				store_mla_grid(true);
			else
				store_mla_grid(false);
		} else if(what == "del") {				// mla del <idx>
			conn->addtag("mla");
			if (mla_del_si(popint(line)))
				conn->write("error mla del :Incorrect subimage index");
		} else if(what == "add") {				// mla add <lx> <ly> <tx> <ty>
			conn->addtag("mla");
			int nx0, ny0, nx1, ny1;
			nx0 = popint(line); ny0 = popint(line); 
			nx1 = popint(line); ny1 = popint(line);
			
			if (mla_update_si(nx0, ny0, nx1, ny1, -1))
				conn->write("error mla add :Incorrect subimage coordinates");
			
		} else if(what == "update") {			// mla update <idx> <lx> <ly> <tx> <ty>
			conn->addtag("mla");
			int idx, nx0, ny0, nx1, ny1;
			idx = popint(line);
			nx0 = popint(line); ny0 = popint(line); 
			nx1 = popint(line); ny1 = popint(line);

			if (mla_update_si(nx0, ny0, nx1, ny1, idx))
				conn->write("error mla update :Incorrect subimage coordinates");
		} else if(what == "set") {				// mla set [mla configuration]
			conn->addtag("mla");
			if (set_mla_str(line))
				conn->write("error mla set :Could not parse MLA string");
		} else if(what == "get") {				// mla get
			conn->write("ok mla " + get_mla_str());
		}
	} else if (command == "get") {
		string what = popword(line);
		
		if (what == "shifts")
			conn->write("ok shifts " + get_shifts_str());
		else 
			parsed = false;
	} else if (command == "set") {
		string what = popword(line);
		
		parsed = false;
	} else if (command == "calibrate") {
		calibrate();
		conn->write("ok calibrate");
	} else if (command == "measure") {
		if (!measure(NULL))
			conn->write("error measure :error in measure()");
		else 
			conn->write("ok measure");
	} else {
		parsed = false;
	}
	
	// If not parsed here, call parent
	if (parsed == false)
		Wfs::on_message(conn, orig);
}

Wfs::wf_info_t* Shwfs::measure(Camera::frame_t *frame) {
	if (!get_calib()) {
		io.msg(IO_WARN, "Shwfs::measure() device not calibrated, should not be.");
		calibrate();
	}
	
	if (!frame) {
		io.msg(IO_WARN, "Shwfs::measure() *frame not available? Auto-acquiring...");
		frame = cam.get_last_frame();
	}
	
	// Calculate shifts
	if (cam.get_depth() == 16) {
		io.msg(IO_DEB2, "Shwfs::measure() got UINT16");
		//shifts.calc_shifts((uint16_t *) frame->image, cam.get_res(), (Shift::crop_t *) mlacfg.ml, mlacfg.size(), shift_vec, method);
	}
	else if (cam.get_depth() == 8) {
		io.msg(IO_DEB2, "Shwfs::measure() got UINT8");
		shifts.calc_shifts((uint8_t *) frame->image, cam.get_res(), mlacfg, shift_vec, method);
	}
	else {
		io.msg(IO_ERR, "Shwfs::measure() unknown camera datatype");
		return NULL;
	}
	
	// Convert shifts to basisfunction
	gsl_vector_float_sub(shift_vec, ref_vec);
	
	// Copy to output
	//! @todo where is this deleted?
	gsl_vector_float_memcpy(wf.wfamp, shift_vec);
	
	return &wf;
}

int Shwfs::shift_to_basis(const gsl_vector_float *const invec, const wfbasis basis, gsl_vector_float *outvec) {
	switch (basis) {
		case SENSOR:
			//! @todo where is this deleted?
			gsl_vector_float_memcpy(outvec, invec);
			break;
		case ZERNIKE:
		case KL:
		case MIRROR:
		case UNDEFINED:
		default:
			//! @todo implement shift_to_basis for KL etc.
			break;
	}
	
	return 0;
}

void Shwfs::init_infmat(string wfcname, size_t nact, vector <float> &actpos) {
	// First delete all data...
	if (calib.find(wfcname) != calib.end()) {
		io.msg(IO_DEB1, "Shwfs::init_infmat(): free()'ing old data.");
		
		calib[wfcname].meas.actpos.clear();
		// Delete calib[wfcname].meas.measmat array
		std::vector<gsl_matrix_float *> tmpvec = calib[wfcname].meas.measmat;
		std::vector<gsl_matrix_float *>::iterator vit;
		for (vit=tmpvec.begin(); vit < tmpvec.end(); vit++ )
			gsl_matrix_float_free(*vit);
		calib[wfcname].meas.measmat.clear();
		
		gsl_matrix_free(calib[wfcname].meas.infmat);
		gsl_matrix_float_free(calib[wfcname].meas.infmat_f);
		
		// Free() .actmat matrices
		gsl_matrix_float_free(calib[wfcname].actmat.mat);
		gsl_matrix_free(calib[wfcname].actmat.U);
		gsl_vector_free(calib[wfcname].actmat.s);
		gsl_matrix_free(calib[wfcname].actmat.Sigma);
		gsl_matrix_free(calib[wfcname].actmat.V);
		
		// Finally erase this entry from the map
		calib.erase(wfcname);
	}
	
	// Store number of actuator positions
	calib[wfcname].meas.actpos = actpos;
	size_t nactpos = actpos.size();
	
	if (nactpos < 2) {
		io.msg(IO_WARN, "Shwfs::init_infmat(): Cannot calibrate with <2 positions.");
		return;
	}
	
	calib[wfcname].nact = nact;
	calib[wfcname].nmeas = mlacfg.size() * 2;
	
	// Init influence measurement data
	for (size_t i=0; i<nactpos; i++) {
		gsl_matrix_float *tmp = gsl_matrix_float_calloc(calib[wfcname].nmeas, nact);
		calib[wfcname].meas.measmat.push_back(tmp);
	}
	calib[wfcname].meas.infmat = gsl_matrix_calloc(calib[wfcname].nmeas, nact);
	calib[wfcname].meas.infmat_f = gsl_matrix_float_calloc(calib[wfcname].nmeas, nact);

	// Init actuation matrices
	calib[wfcname].actmat.mat = gsl_matrix_float_calloc(nact, calib[wfcname].nmeas);

	calib[wfcname].actmat.s = gsl_vector_calloc(nact);

	calib[wfcname].actmat.U = gsl_matrix_calloc(calib[wfcname].nmeas, nact);
	calib[wfcname].actmat.Sigma = gsl_matrix_calloc(nact, nact);
	calib[wfcname].actmat.V = gsl_matrix_calloc(nact, nact);
	
	calib[wfcname].init = true;
}
	
int Shwfs::build_infmat(string wfcname, Camera::frame_t *frame, int actid, int actposid) {
	if (!calib[wfcname].init) {
		io.msg(IO_WARN, "Shwfs::build_infmat(): Call Shwfs::init_infmat() first.");
		return 0;
	}
	
	// Measure shifts
	wf_info_t *m = measure(frame);
	
	// Copy to measmat
	gsl_matrix_float *measmat = calib[wfcname].meas.measmat[actposid];
	for (size_t i=0; i<m->wfamp->size; i++)
		gsl_matrix_float_set(measmat, i, actid, gsl_vector_float_get(m->wfamp, i));
			
	return (int) m->wfamp->size;
}

int Shwfs::calc_infmat(string wfcname) {
	if (!calib[wfcname].init) {
		io.msg(IO_WARN, "Shwfs::calc_infmat(): Call Shwfs::init_infmat() first.");
		return 0;
	}

	/*
	 This code is similar to the matrix method below, but this one might be more
	 illustrative
	 
	// Loop over all actuators
	float dmeas, dact;
	for (size_t actn=0; actn<calib[wfcname].nact; actn++) {
		// Loop over all measurements
		for (size_t measn=0; measn<calib[wfcname].nmeas; measn++) {
			// For this (actuator, measurement) pair, calculate the average slope
			float avgslope = 0.0;
			for (size_t i=0; i<calib[wfcname].meas.actpos.size()-1; i++) {
				dmeas = gsl_matrix_float_get(calib[wfcname].meas.measmat[i+1], measn, actn) - gsl_matrix_float_get(calib[wfcname].meas.measmat[i], measn, actn);
				dact = calib[wfcname].meas.actpos[i+1] - calib[wfcname].meas.actpos[i];
				avgslope += dmeas/dact;
			}
			avgslope /= (calib[wfcname].meas.actpos.size()-1.0);
			gsl_matrix_float_set(calib[wfcname].meas.infmat, measn, actn, avgslope);
		}
	}
	 */
	
	// Init temporary matrices
	gsl_matrix_float *diffmat = gsl_matrix_float_calloc(calib[wfcname].nmeas, calib[wfcname].nact);
	gsl_matrix_float *infmat_f = calib[wfcname].meas.infmat_f;
	double dact=0.0;
	
	// For each measurement position-pair, calculate the slope this generates 
	// for the influence matrix. I.e. calculate (actpos[i+1] - actpos[i] ) /
	// (measmat[i+1] - measmat[i]) for i in [0, actpos.size()-1]
	for (size_t i=0; i<calib[wfcname].meas.actpos.size()-1; i++) {
		gsl_matrix_float_memcpy(diffmat, calib[wfcname].meas.measmat[i+1]);
		gsl_matrix_float_sub (diffmat, calib[wfcname].meas.measmat[i]);
		
		dact = calib[wfcname].meas.actpos[i+1] - calib[wfcname].meas.actpos[i];
		gsl_matrix_float_scale(diffmat, 1.0/dact);
		gsl_matrix_float_add (infmat_f, diffmat);
	}
	gsl_matrix_float_scale(infmat_f, 1.0/(calib[wfcname].meas.actpos.size()-1));
	
	// Copy infmat_f (float) to infmat (double)
	gsl_matrix *infmat = calib[wfcname].meas.infmat;
	for (size_t i=0; i<infmat->size1; i++)
		for (size_t j=0; j<infmat->size2; j++)
			gsl_matrix_set(infmat, i, j, gsl_matrix_float_get(infmat_f, i, j));
	
	// Free temporary matrices
	gsl_matrix_float_free(diffmat);
	
	// Store influence matrix to disk
	Path outf; FILE *fd;
	outf = mkfname(wfcname + format("_infmat_%d_%d.csv", infmat->size1, infmat->size2));
	fd = fopen(outf.c_str(), "w+");
	gsl_matrix_fprintf (fd, infmat, "%.12g");
	fclose(fd);
	
	return 0;
}

/*
int Shwfs::gsl_linalg_SV_decomp_float(gsl_matrix_float *U, gsl_matrix_float *V, gsl_vector_float *s) {
	// Copy matrices to double-precision versions
	gsl_matrix *Ud = gsl_matrix_alloc(U->size1, U->size2);
	gsl_matrix *Vd = gsl_matrix_alloc(V->size1, V->size2);
	gsl_vector *sd = gsl_vector_alloc(s->size);
	gsl_vector *workvec = gsl_vector_alloc(s->size);
	
	// Copy U to Ud
	for (size_t i; i<U->size1; i++)
		for (size_t j; j<U->size2; j++)
			gsl_matrix_set(Ud, i, j, gsl_matrix_float_get(U, i, j));
	
	// Perform gsl_linalg_SV_decomp
	gsl_linalg_SV_decomp(Ud, Vd, sd, workvec);
	
	// Copy back to single-precision
	
}
 */


int Shwfs::calc_actmat(string wfcname, double singval, enum wfbasis /*basis*/) {
	io.msg(IO_XNFO, "Shwfs::calc_actmat(): calc'ing for wfc '%s' with singval cutoff %g.",
				 wfcname.c_str(), singval);
	// Using input:
	// calib[wfcname].meas.infmat
	// Calculating:
	// calib[wfcname].actmat.mat
	// calib[wfcname].actmat.U
	// calib[wfcname].actmat.s
	// calib[wfcname].actmat.Sigma
	// calib[wfcname].actmat.V
	
	
	// Make matrix aliases
	gsl_matrix_float *mat = calib[wfcname].actmat.mat;

	gsl_matrix *infmat = calib[wfcname].meas.infmat;
	gsl_matrix *U = calib[wfcname].actmat.U;
	gsl_matrix *Sigma = calib[wfcname].actmat.Sigma;
	gsl_vector *s = calib[wfcname].actmat.s;
	gsl_matrix *V = calib[wfcname].actmat.V;
	
	gsl_matrix *actmat_d = gsl_matrix_calloc(mat->size1, mat->size2);
	gsl_vector *workvec = gsl_vector_calloc(s->size);

	// Copy input matrix to U (which will be overwritten by gsl_linalg_SV_decomp())
	gsl_matrix_memcpy(U, infmat);
	
	// Singular value decompose this matrix:
	gsl_linalg_SV_decomp(U, V, s, workvec);
	
	// Store decomposition to disk
	Path outf; FILE *fd;
	
	outf = mkfname(wfcname + format("_singval_%zu.csv", s->size));
	fd = fopen(outf.c_str(), "w+");
	gsl_vector_fprintf (fd, s, "%.12g");
	fclose(fd);
	
	outf = mkfname(wfcname + format("_U_%zu_%zu.csv", U->size1, U->size2));
	fd = fopen(outf.c_str(), "w+");
	gsl_matrix_fprintf (fd, U, "%.12g");
	fclose(fd);
	
	outf = mkfname(wfcname + format("_V_%zu_%zu.csv", V->size1, V->size2));
	fd = fopen(outf.c_str(), "w+");
	gsl_matrix_fprintf (fd, V, "%.12g");
	fclose(fd);	
	
	// Calculate sum of singular values
	double sum=0;
	for (size_t j=0; j < s->size; j++)
		sum += gsl_vector_get(s, j);
	
	// Calculate various condition cutoffs
	int acc85=0, acc90=0, acc95=0;
	double sum2=0;
	for (size_t j=0; j < s->size; j++) {
		double sval = gsl_vector_get(s, j);
		sum2 += sval;
//		io.msg(IO_DEB1, "Shwfs::calc_actmat(): singval %zu: %g (cumsum: %g)",  j, sval, sum2/sum);
		if (sum2/sum < 0.85) acc85++;
		if (sum2/sum < 0.9) acc90++;
		if (sum2/sum < 0.95) acc95++;
	}
	io.msg(IO_XNFO, "Shwfs::calc_actmat(): SVD condition: %g, nmodes: %zu", 
				 gsl_vector_get(s, 0)/
				 gsl_vector_get(s, s->size-1), 
				 s->size);
	io.msg(IO_XNFO, "Shwfs::calc_actmat(): cond 0.85 @ %d, 0.90 @ %d, 0.95 @ %d modes", 
				 acc85, acc90, acc95);
	
	// Fill singular value *matrix* Sigma and store to disk
	sum2 = 0.0;
	for (size_t j=0; j < s->size; j++) {
		double sval = gsl_vector_get(s, j);
		sum2 += sval;
		if (sval != 0) sval = 1.0/sval;
		
		gsl_matrix_set(Sigma, j, j, sval);
		//if (sum2/sum <= singval)
	}
	
	outf = mkfname(wfcname + format("_Sigma_%zu_%zu.csv", Sigma->size1, Sigma->size2));
	fd = fopen(outf.c_str(), "w+");
	gsl_matrix_fprintf (fd, Sigma, "%.12g");
	fclose(fd);
	
	// Calculate explicit pseudo-inverse matrix of infmat, store in actmat. The 
	// pseudo-inverse is given by actmat = V . Sigma . U^T
	gsl_matrix *workmat = gsl_matrix_calloc(mat->size1, mat->size2);
	
	// First calculate workmat = (Sigma . U^T)
	gsl_blas_dgemm (CblasNoTrans, CblasTrans, 1.0, Sigma, U, 0.0, workmat);
	// Then calculate actmat_d = (V . workmat)
	gsl_blas_dgemm (CblasNoTrans, CblasNoTrans, 1.0, V, workmat, 0.0, actmat_d);
	
	// Store inverse matrix to disk
	outf = mkfname(wfcname + format("_actmat_%zu_%zu.csv", actmat_d->size1, actmat_d->size2));
	fd = fopen(outf.c_str(), "w+");
	gsl_matrix_fprintf (fd, actmat_d, "%.12g");
	fclose(fd);
	
	// Test inversion, calculate (mat . infmat):
	gsl_matrix_free(workmat);
	workmat = gsl_matrix_calloc(actmat_d->size1, infmat->size2);
	gsl_blas_dgemm (CblasNoTrans, CblasNoTrans, 1.0, actmat_d, infmat, 0.0, workmat);
	
	// Store pseudo-ident matrix to disk
	outf = mkfname(wfcname + format("_pseudo-ident_%zu_%zu.csv", workmat->size1, workmat->size2));
	fd = fopen(outf.c_str(), "w+");
	gsl_matrix_fprintf (fd, workmat, "%.12g");
	fclose(fd);
	
	// Sum all elements in workmat (should be sum(identity(workmat->size1)) = workmat->size1)
	sum=0;
	for (size_t i=0; i<workmat->size1; i++)
		for (size_t j=0; j<workmat->size2; j++)
			sum += gsl_matrix_get(workmat, i, j);
	
	sum -= workmat->size1;
	
	io.msg(IO_XNFO, "Shwfs::calc_actmat(): avg inversion error: %g.", 
				 sum/(workmat->size2*workmat->size1));	
	
	// Test matrix-vector multiplications
	
	// Allocate vectors
	gsl_vector *vecin = gsl_vector_calloc(infmat->size2);
	gsl_vector *vecout = gsl_vector_calloc(infmat->size1);
	gsl_vector *vecrec = gsl_vector_calloc(infmat->size2);
	gsl_vector *vecrec2 = gsl_vector_calloc(infmat->size2);
	
	for (size_t i=0; i<vecin->size; i++)
		gsl_vector_set(vecin, i, drand48()*2.0-1.0);
	
	// Forward calculate
	gsl_blas_dgemv(CblasNoTrans, 1.0, infmat, vecin, 0.0, vecout);
	
	// Backward calculate:
	gsl_linalg_SV_solve (U, V, s, vecout, vecrec);
	
	// Backward calculate with explicit matrix:
	gsl_blas_dgemv(CblasNoTrans, 1.0, actmat_d, vecout, 0.0, vecrec2);
	
	// Print vectors
	double qual1=0, qual2=0;
	for (size_t j=0; j < vecin->size; j++) {
		qual1 += gsl_vector_get(vecin, j)/gsl_vector_get(vecrec, j);
		qual2 += gsl_vector_get(vecin, j)/gsl_vector_get(vecrec2, j);
	}
	qual1 /= vecin->size; qual2 /= vecin->size;

	io.msg(IO_XNFO, "Shwfs::calc_actmat(): average rel. error: %g and %g.",
				 1.-qual1, 1.-qual2);
	
	// Copy actmat_d (double) to mat (float)
	for (size_t i=0; i<mat->size1; i++)
		for (size_t j=0; j<mat->size2; j++)
			gsl_matrix_float_set(mat, i, j, gsl_matrix_get(actmat_d, i, j));

	// Free temporary matrices
	gsl_matrix_free(workmat);
	gsl_matrix_free(actmat_d);

	return 0;
}

gsl_vector_float *Shwfs::comp_ctrlcmd(string wfcname, gsl_vector_float *shift, gsl_vector_float *act) {
	//! @todo comp_ctrlcmd() does not know whether the matrix calib[wfcname].actmat.mat is proper or not. Need better calibration tracking, not at device level but at the top level of the program perhaps.
	if (calib.find(wfcname) == calib.end())
		return NULL;
	if (!get_calib())
		calibrate();
	
	// Compute vector. We apply -1 here because the matrix is the pseudo inverse
	// of infmat, while it should be of -infmat. Alternative explanation: we 
	// need to *correct* the shifts measured, not reproduce them
	gsl_blas_sgemv(CblasNoTrans, -1.0, calib[wfcname].actmat.mat, shift, 0.0, act);

	return act;
}

gsl_vector_float *Shwfs::comp_shift(string wfcname, gsl_vector_float *act, gsl_vector_float *shift) {
	if (calib.find(wfcname) == calib.end())
		return NULL;
	if (!get_calib())
		calibrate();
	
	// Compute vector
	gsl_blas_sgemv(CblasNoTrans, 1.0, calib[wfcname].meas.infmat_f, act, 0.0, shift);
	
	return shift;
}

void Shwfs::set_reference(Camera::frame_t *frame) {
	// Measure shifts
	wf_info_t *m = measure(frame);
	if (!m)
		return;

	// Store these as reference positions
	gsl_vector_float_memcpy(ref_vec, m->wfamp);
	io.msg(IO_DEB2 | IO_NOLF, "Shwfs::set_reference() got: ");
	for (size_t i=0; i<ref_vec->size; i++)
		io.msg(IO_DEB2 | IO_NOLF | IO_NOID, "%.1f ", gsl_vector_float_get(ref_vec, i));
	io.msg(IO_DEB2 | IO_NOLF, "\n");
}

void Shwfs::store_reference() {
//	string outfile = mkfname("ref_vec.csv").str();
//	io.msg(IO_DEB2, "Shwfs::store_reference() to " + outfile);
//	Csv refvecdat(ref_vec);
//	refvecdat.write(outfile, "Shwfs reference vector");

	Path outf; FILE *fd;
	outf = mkfname(format("ref_vec_%zu.csv", ref_vec->size));
	fd = fopen(outf.c_str(), "w+");
	gsl_vector_float_fprintf (fd, ref_vec, "%.12g");
	fclose(fd);
}

int Shwfs::calibrate() {
	if (mlacfg.size() == 0) {
		io.msg(IO_XNFO, "Shwfs::calibrate(): cannot calibrate without subapertures defined.");
		return -1;
	}
	
	gsl_vector_float_free(shift_vec);
	shift_vec = gsl_vector_float_calloc(mlacfg.size() * 2);
	gsl_vector_float_free(ref_vec);
	ref_vec = gsl_vector_float_calloc(mlacfg.size() * 2);
	
	switch (wf.basis) {
		case SENSOR:
			io.msg(IO_XNFO, "Shwfs::calibrate(): Calibrating for basis 'SENSOR'");
			wf.nmodes = mlacfg.size() * 2;
			if (wf.wfamp)
				gsl_vector_float_free(wf.wfamp);
			wf.wfamp = gsl_vector_float_calloc(wf.nmodes);
			break;
		case ZERNIKE:
			//! @todo how many modes do we want if we're using Zernike? Is the same as the number of coordinates ok or not?
//			wf.nmodes = mlacfg.size() * 2;
//			wf.wfamp = gsl_vector_float_calloc(wf.nmodes);
//			zerninfl = gsl_matrix_float_calloc(mlacfg.size() * 2, wf.nmodes);
		case KL:
		case MIRROR:
			//! @todo Implement Zernike, KL & mirror modes
		case UNDEFINED:
		default:
			io.msg(IO_WARN, "Shwfs::calibrate(): This basis is not implemented yet");
			return -1;
			break;
	}
	
	Wfs::calibrate();
	return 0;
}


int Shwfs::gen_mla_grid(std::vector<vector_t> &mlacfg, const coord_t res, const coord_t size, const coord_t pitch, const int xoff, const coord_t disp, const mlashape_t shape, const float overlap) {
	io.msg(IO_DEB2, "Shwfs::gen_mla_grid()");
	
	set_calib(false);

	// How many subapertures would fit in the requested size 'res':
	int sa_range_y = (int) ((res.y/2)/pitch.x + 1);
	int sa_range_x = (int) ((res.x/2)/pitch.y + 1);
	
	vector_t tmpsi;
	mlacfg.clear();
	
	float minradsq = pow(((double) min(res.x, res.y))/2.0, 2);
	
	coord_t sa_c;
	// Loop over potential subaperture positions 
	for (int sa_y=-sa_range_y; sa_y < sa_range_y; sa_y++) {
		for (int sa_x=-sa_range_x; sa_x < sa_range_x; sa_x++) {
			// Centroid position of current subap is 
			sa_c.x = sa_x * pitch.x;
			sa_c.y = sa_y * pitch.y;
			
			// Offset odd rows: 'sa_y % 2' gives 0 for even rows and 1 for odd rows. 
			// Use this to apply a row-offset to even and odd rows
			sa_c.x -= (sa_y % 2) * xoff * pitch.x;
			
			if (shape == CIRCULAR) {
				if (pow(fabs(sa_c.x) + (overlap-0.5)*size.x, 2) + pow(fabs(sa_c.y) + (overlap-0.5)*size.y, 2) < minradsq) {
					tmpsi = vector_t(sa_c.x + disp.x - size.x/2,
									 sa_c.y + disp.y - size.y/2,
									 sa_c.x + disp.x + size.x/2,
									 sa_c.y + disp.y + size.y/2);
					mlacfg.push_back(tmpsi);
				}
			}
			else {
				// Accept a subimage coordinate (center position) the position + 
				// half-size the subaperture is within the bounds
				if ((fabs(sa_c.x + (overlap-0.5)*size.x) < res.x/2) && (fabs(sa_c.y + (overlap-0.5)*size.y) < res.y/2)) {
					tmpsi = vector_t(sa_c.x + disp.x - size.x/2,
													 sa_c.y + disp.y - size.y/2,
													 sa_c.x + disp.x + size.x/2,
													 sa_c.y + disp.y + size.y/2);
					mlacfg.push_back(tmpsi);
				}
			}
		}
	}
	io.msg(IO_XNFO, "Shwfs::gen_mla_grid(): Found %d subapertures.", (int) mlacfg.size());
	netio.broadcast("ok mla " + get_mla_str(), "mla");
	
	// Re-calibrate with new settings
	calibrate();
	
	return (int) mlacfg.size();
}

bool Shwfs::store_mla_grid(const bool overwrite) const {
	// Make path from fileid 
	Path f = ptc->datadir + format("shwfs_mla_cfg_n=%03d.csv", mlacfg.size());
	
	if (f.exists() && !overwrite) {
		io.msg(IO_WARN, "Shwfs::store_mla_grid(): Cannot store MLA grid, file exists.");
		return false;
	}
	if (f.exists() && !f.isfile() && overwrite) {
		io.msg(IO_WARN, "Shwfs::store_mla_grid(): Cannot store MLA grid, path exists, but is not a file.");
		return false;
	}
	
	FILE *fd = f.fopen("w");
	if (fd == NULL) {
		io.msg(IO_ERR, "Shwfs::store_mla_grid(): Could not open file '%s'", f.c_str());
		return false;
	}
	
	fprintf(fd, "# Shwfs:: MLA definition\n");
	fprintf(fd, "# MLA definition, nsi=%zu.\n", mlacfg.size());
	fprintf(fd, "# Columns: x0, y0, x1, y1\n");
	for (size_t n=0; n<mlacfg.size(); n++) {
		fprintf(fd, "%d, %d, %d, %d\n", 
						mlacfg[n].lx,
						mlacfg[n].ly,
						mlacfg[n].tx,
						mlacfg[n].ty);
	}
	fclose(fd);
	
	io.msg(IO_INFO, "Shwfs::store_mla_grid(): Wrote MLA grid to '%s'.", f.c_str());
	io.msg(IO_XNFO | IO_NOID, "Plot these data in gnuplot with:");
	io.msg(IO_XNFO | IO_NOID, "set key");
	io.msg(IO_XNFO | IO_NOID, "set xrange[0:%d]", cam.get_width());
	io.msg(IO_XNFO | IO_NOID, "set yrange[0:%d]", cam.get_height());	
	io.msg(IO_XNFO | IO_NOID, "set obj 1 ellipse at first %d, first %d size %d,%d front fs empty lw 0.8",
				 cam.get_width()/2, cam.get_height()/2, cam.get_width(), cam.get_height());
	io.msg(IO_XNFO | IO_NOID, "plot 'mla_grid' using 1:2:5:6 title 'subap size' with vectors lt -1 lw 1 heads, 'mla_grid' using 3:4 title 'subap center'");
	
	return true;
}

int Shwfs::find_mla_grid(std::vector<vector_t> &mlacfg, const coord_t size, const float mini_f, const int nmax, const int iter) {
	io.msg(IO_DEB2, "Shwfs::find_mla_grid()");

	set_calib(false);

	// Store current camera count, get last frame
	Camera::frame_t *f = cam.get_last_frame();
	void *image;
	size_t imsize;
	
	if (f == NULL) {
		io.msg(IO_WARN, "Shwfs::find_mla_grid() Could not get frame, is the camera running?");
		return 0;
	} else {
		// Copy frame for ourselves
		imsize = f->size;
		image = malloc(imsize);
		memcpy(image, f->image, imsize);
	}

	
	vector_t tmpsi;
	mlacfg.clear();
	
	coord_t sipos;
	
	size_t maxidx = 0;
	int maxi = 0;
	
	if (cam.get_depth() <= 8) {
		maxi = _find_max((uint8_t *)image, imsize, &maxidx);
	} else {
		maxi = _find_max((uint16_t *)image, imsize/2, &maxidx);
	}
	// Minimum intensity
	int mini = maxi * mini_f;
	if (mini <= 0) {
		io.msg(IO_WARN, "Shwfs::find_mla_grid() I_min < 0, something went wrong, aborting.");
		return 0;
	}
	
	io.msg(IO_DEB2, "Shwfs::find_mla_grid(maxi=%d, mini_f=%g, mini=%d)", maxi, mini_f, mini);
	
	// Find maximum intensity pixels & set area around it to zero until there is 
	// no more maximum above mini or we have reached nmax subapertures
	while (true) {
		maxidx = 0;
		maxi = 0;
		
		if (cam.get_depth() <= 8) {
			maxi = _find_max((uint8_t *)image, imsize, &maxidx);
		} else {
			maxi = _find_max((uint16_t *)image, imsize/2, &maxidx);
		}
		
		// Intensity too low, done
		if (maxi < mini)
			break;
		
		// Add new subimage
		tmpsi = vector_t((maxidx % cam.get_width()) - size.x/2,
										 int(maxidx / cam.get_width()) - size.y/2,
										 (maxidx % cam.get_width()) + size.x/2,
										 int(maxidx / cam.get_width()) + size.y/2);
		mlacfg.push_back(tmpsi);
		
		//! @bug Routine seems to block with idx = 0, I=0 sometimes, does not quit when
		io.msg(IO_DEB2, "Shwfs::find_mla_grid(): new! I: %d, idx: %zu, llpos: (%d,%d), (#: %d/%d)", 
					 maxi, maxidx, tmpsi.lx, tmpsi.ly, (int) mlacfg.size(), nmax);

		// If we have enough subimages, done
		if ((int) mlacfg.size() == nmax)
			break;
		if ((int) mlacfg.size() >= cam.get_width()*cam.get_height()/size.x/size.y) {
			io.msg(IO_WARN, "Shwfs::find_mla_grid() subaperture detection overflow, aborting!");
			mlacfg.clear();
			return 0;
		}
		
		// Set the current subimage to zero such that we don't detect it next time
		int xran[] = {max(0, tmpsi.lx), min(cam.get_width(), tmpsi.tx)};
		int yran[] = {max(0, tmpsi.ly), min(cam.get_height(), tmpsi.ty)};
		
		if (cam.get_depth() <= 8) {
			uint8_t *cimg = (uint8_t *)image;
			for (int y=yran[0]; y<yran[1]; y++)
				for (int x=xran[0]; x<xran[1]; x++)
					cimg[y*cam.get_width() + x] = 0;
		} else {
			uint16_t *cimg = (uint16_t *)image;
			for (int y=yran[0]; y<yran[1]; y++)
				for (int x=xran[0]; x<xran[1]; x++)
					cimg[y*cam.get_width() + x] = 0;
		}
	}
	
	// Done! We have found all maximum intensities now.
		
	for (int it=1; it<iter; it++) {
		//! @todo implement iterative updates?
		io.msg(IO_WARN, "Shwfs::find_mla_grid(): iter not yet implemented");
	}
	
	netio.broadcast("ok mla " + get_mla_str(), "mla");
	// Re-calibrate with new settings
	calibrate();
	
	free(image);

	return (int) mlacfg.size();
}

int Shwfs::mla_subapsel() {

	//! @todo implement this with generic MLA setup
	return 0;
}

int Shwfs::mla_update_si(const int nx0, const int ny0, const int nx1, const int ny1, const int idx) {
	// Check bounds
	if (nx0 >= 0 && ny0 >= 0 && 
			nx1 < cam.get_width() && ny1 < cam.get_height() &&
			nx1 > nx0 && ny1 > ny0) {
		
		// Update if idx is valid, otherwise add extra subimage
		if (idx >=0 && idx < (int) mlacfg.size())
			mlacfg[idx] = vector_t(nx0, ny0, nx1, ny1);
		else
			mlacfg.push_back(vector_t(nx0, ny0, nx1, ny1));
		
		set_calib(false);
		calibrate();

		netio.broadcast("ok mla " + get_mla_str(), "mla");
		return 0;
	}
	else {
		return -1;
	}
}

int Shwfs::mla_del_si(const int idx) {
	if (idx >=0 && idx < (int) mlacfg.size()) {
		mlacfg.erase(mlacfg.begin() + idx);
		set_calib(false);
		calibrate();
		netio.broadcast("ok mla " + get_mla_str(), "mla");
		return 0;
	}
	else
		return -1;
}

 
// PRIVATE FUNCTIONS //
/*********************/

template <class T> 
int Shwfs::_find_max(const T *const img, const size_t nel, size_t *const idx) {
	int max=img[0];
	for (size_t p=0; p<nel; p++) {
		if (img[p] > max) {
			max = img[p];
			*idx = p;
		}
	}
	return max;	
}

string Shwfs::get_mla_str() const {
	io.msg(IO_DEB2, "Shwfs::get_mla_str()");
	string ret;
	
	// Return all subimages
	ret = format("%d ", mlacfg.size());
	
	for (size_t i=0; i<mlacfg.size(); i++)
		ret += format("%d %d %d %d %d ", (int) i, mlacfg[i].lx, mlacfg[i].ly, mlacfg[i].tx, mlacfg[i].ty);

	return ret;
}

int Shwfs::set_mla_str(string mla_str) {
	int nsi = popint(mla_str);
	int x0, y0, x1, y1;
	
	set_calib(false);
	
	mlacfg.clear();
	for (int i=0; i<nsi; i++) {
		x0 = popint(mla_str);
		y0 = popint(mla_str);
		x1 = popint(mla_str);
		y1 = popint(mla_str);
		if (x0 <= 0 || y0 <= 0 || x1 <= 0 || y1 <= 0)
			continue;
		
		mlacfg.push_back(vector_t(x0, y0, x1, y1));
	}
	
	calibrate();

	netio.broadcast("ok mla " + get_mla_str(), "mla");
	return (int) mlacfg.size();
}

string Shwfs::get_shifts_str() const {
	io.msg(IO_DEB2, "Shwfs::get_shifts_str()");
	string ret;
	
	// Return all shifts in one string
	//! @bug This might cause problems when others are writing this data!
	ret = format("%d ", (int) shift_vec->size);
	
	for (size_t i=0; i<shift_vec->size/2; i++) {
		fcoord_t vec_origin(mlacfg[i].lx/2 + mlacfg[i].tx/2, mlacfg[i].ly/2 + mlacfg[i].ty/2);
		ret += format("%d %g %g %g %g ", (int) i, 
									vec_origin.x,
									vec_origin.y,
									vec_origin.x + gsl_vector_float_get(shift_vec, i*2+0), 
									vec_origin.y + gsl_vector_float_get(shift_vec, i*2+1));
	}
	
	return ret;
}

