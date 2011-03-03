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

#include "config.h"
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
	simini_f = cfg.getdouble("simini_f", 0.8);
	
	// Generate MLA grid
	gen_mla_grid(mlacfg, cam.get_res(), sisize, sipitch, xoff, disp, shape, overlap);
	
	// Initial calibration
	calibrate();
}

Shwfs::~Shwfs() {
	io.msg(IO_DEB2, "Shwfs::~Shwfs()");
}

//int Shwfs::calc_zern_infl(int nmodes) {
//	// Re-compute Zernike basis functions if necessary
//	if (nmodes > zernbasis.get_nmodes() || cam.get_width() != zernbasis.get_size())
//		zernbasis.setup(nmodes, cam.get_width());
//	
//	// Calculate the slope in each of the subapertures for each wavefront mode
//	double slope[2];
//	for (int m=0; m <= nmodes; m++) {
//		gsl_matrix *tmp = zernbasis.get_mode(m);
//		calc_slope(tmp, mlacfg, slope);
//		
//		//! @todo implement this
//		
//	}
//	return 0;
//}
//
//int Shwfs::calc_slope(gsl_matrix *tmp, std::vector<vector_t> &mlacfg, double *slope) {
//	double sum;
//	for (size_t si=0; si < mlacfg.size(); si++) {
//		sum=0;
//		for (int x=mlacfg[i].lx; x < mlacfg[i].tx; x++) {
//			for (int y=mlacfg[i].ly; y < mlacfg[i].ty; y++) {
//				// Calculate (x,y)-slope in each subaperture
//				//! @todo implement this
//				gsl_matrix_get(tmp, y, x);
//			}
//		}
//	}
//	return 0;
//}

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
		if (measure())
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

int Shwfs::measure() {
	if (!is_calib)
		return io.msg(IO_ERR, "Shwfs::measure() calibrate sensor first!");
	
	Camera::frame_t *tmp = cam.get_last_frame();

	if (!tmp)
		return io.msg(IO_ERR, "Shwfs::measure() couldn't get frame, is camera running?");
	
	// Calculate shifts
	
	if (cam.get_depth() == 16) {
		io.msg(IO_DEB2, "Shwfs::measure() got UINT16");
		//shifts.calc_shifts((uint16_t *) tmp->image, cam.get_res(), (Shift::crop_t *) mlacfg.ml, mlacfg.size(), shift_vec, method);
	}
	else if (cam.get_depth() == 8) {
		io.msg(IO_DEB2, "Shwfs::measure() got UINT8");
		shifts.calc_shifts((uint8_t *) tmp->image, cam.get_res(), mlacfg, shift_vec, method);
	}
	else
		return io.msg(IO_ERR, "Shwfs::measure() unknown camera datatype");
	
	// Convert shifts to basisfunction
	shift_to_basis(shift_vec, wf.basis, wf.wfamp);
	
	return 0;
}

int Shwfs::shift_to_basis(const gsl_vector_float *const invec, const wfbasis basis, gsl_vector_float *outvec) {
	switch (basis) {
		case SENSOR:
			gsl_vector_float_memcpy(outvec, invec);
			break;
		case ZERNIKE:
		case KL:
		case MIRROR:
		case UNDEFINED:
		default:
			break;
	}
	
	return 0;
	
}
int Shwfs::calibrate() {
	shift_vec = gsl_vector_float_calloc(mlacfg.size() * 2);
	
	switch (wf.basis) {
		case SENSOR:
			io.msg(IO_XNFO, "Shwfs::calibrate(): Calibrating for basis 'SENSOR'");
			wf.nmodes = mlacfg.size() * 2;
			wf.wfamp = gsl_vector_float_calloc(wf.nmodes);
			break;
		case ZERNIKE:
			//! @todo how many modes do we want if we're using Zernike? Is the same as the number of coordinates ok or not?
			wf.nmodes = mlacfg.size() * 2;
			wf.wfamp = gsl_vector_float_calloc(wf.nmodes);
			zerninfl = gsl_matrix_float_calloc(mlacfg.size() * 2, wf.nmodes);
		case KL:
		case MIRROR:
			//! @todo Implement KL & mirror modes
		case UNDEFINED:
		default:
			io.msg(IO_WARN, "Shwfs::calibrate(): This basis is not implemented yet");
			return -1;
			break;
	}
	
	is_calib = true;
	return 0;
}


int Shwfs::gen_mla_grid(std::vector<vector_t> &mlacfg, const coord_t res, const coord_t size, const coord_t pitch, const int xoff, const coord_t disp, const mlashape_t shape, const float overlap) {
	io.msg(IO_DEB2, "Shwfs::gen_mla_grid()");
	
	is_calib = false;

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

	is_calib = false;

	// Store current camera count, get last frame
	size_t bcount = cam.get_count();
	Camera::frame_t *f = cam.get_last_frame();
	
	if (f == NULL) {
		io.msg(IO_WARN, "Shwfs::find_mla_grid() Could not get frame, is the camera running?");
		return NULL;
	}
	
	vector_t tmpsi;
	mlacfg.clear();
	
	coord_t sipos;
	
	size_t maxidx = 0;
	int maxi = 0;
	
	if (cam.get_depth() <= 8) {
		uint8_t *image = (uint8_t *)f->image;
		maxi = _find_max(image, f->size, &maxidx);
	} else {
		uint16_t *image = (uint16_t *)f->image;
		maxi = _find_max(image, f->size/2, &maxidx);
	}
	// Minimum intensity
	int mini = maxi * mini_f;
	io.msg(IO_DEB2, "Shwfs::find_mla_grid(maxi=%d, mini_f=%g, mini=%d)", maxi, mini_f, mini);

	
	// Find maximum intensity pixels & set area around it to zero until there is 
	// no more maximum above mini or we have reached nmax subapertures
	while (true) {
		maxidx = 0;
		maxi = 0;
		
		if (cam.get_depth() <= 8) {
			uint8_t *image = (uint8_t *)f->image;
			maxi = _find_max(image, f->size, &maxidx);
		} else {
			uint16_t *image = (uint16_t *)f->image;
			maxi = _find_max(image, f->size/2, &maxidx);
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
		
		io.msg(IO_DEB2, "Shwfs::find_mla_grid(): new! I: %d, idx: %zu, llpos: (%d,%d)", maxi, maxidx, tmpsi.lx, tmpsi.ly);

		// If we have enough subimages, done
		if ((int) mlacfg.size() == nmax)
			break;
		
		// Set the current subimage to zero such that we don't detect it next time
		int xran[] = {max(0, tmpsi.lx), min(cam.get_width(), tmpsi.tx)};
		int yran[] = {max(0, tmpsi.ly), min(cam.get_height(), tmpsi.ty)};
		
		if (cam.get_depth() <= 8) {
			uint8_t *image = (uint8_t *)f->image;
			for (int y=yran[0]; y<yran[1]; y++)
				for (int x=xran[0]; x<xran[1]; x++)
					image[y*cam.get_width() + x] = 0;
		} else {
			uint16_t *image = (uint16_t *)f->image;
			for (int y=yran[0]; y<yran[1]; y++)
				for (int x=xran[0]; x<xran[1]; x++)
					image[y*cam.get_width() + x] = 0;
		}
	}
	
	// Done! We have found all maximum intensities now.
		
	for (int it=1; it<iter; it++) {
		//! @todo implement iterative updates?
		io.msg(IO_WARN, "Shwfs::find_mla_grid(): iter not yet implemented");
	}
	
	// Get latest camera count. If the difference between begin and end is 
	// bigger than the size of the camera ringbuffer, we were too slow
	size_t ecount = cam.get_count();
	if (ecount - bcount >= cam.get_bufsize()) {
		//! @todo This poses possible problems, what to do?
		io.msg(IO_WARN, "Shwfs::find_mla_grid(): got camera buffer overflow, data might be inaccurate");
	}
	
	netio.broadcast("ok mla " + get_mla_str(), "mla");
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
	
	is_calib = false;
	
	for (int i=0; i<nsi; i++) {
		x0 = popint(mla_str);
		y0 = popint(mla_str);
		x1 = popint(mla_str);
		y1 = popint(mla_str);
		if (x0 <= 0 || y0 <= 0 || x1 <= 0 || y1 <= 0)
			continue;
		
		mlacfg.push_back(vector_t(x0, y0, x1, y1));
	}
	
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

