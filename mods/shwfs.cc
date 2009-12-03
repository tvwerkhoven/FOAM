/*
 Copyright (C) 2009 Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)
 
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
	@file whwfs.ccc
	@author Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)

	@brief This file contains modules and functions related to Shack-Hartmann 
	wavefront sensing.
*/

// HEADERS //
/***********/

#include <stdint.h>
#include <math.h>

#include "config.h"
#include "types.h"
#include "cam.h"
#include "io.h"
#include "wfs.h"

#define SHWFS_TYPE "shwfs"
extern Io *io;

// ROUTINES //
/************/

class Shwfs: public Wfs {
	private:
	
	coord_t subap;			//!< Number of subapertures in X and Y
	coord_t sasize;			//!< Subaperture pixel size
	coord_t track;			//!< Subaperture tracking window size
	int samaxr;					//!< Maximum radius to use, or edge erosion subapertures
	int samini;					//!< Minimum amount of subapertures to use
	
	coord_t *trackpos;
	coord_t *sapos;
	
	int ns;
	
	template <class T>
	int _cog(T *img, int xpos, int ypos, int w, int h, int stride, int samini, float *cog) {
		float fi, csum = 0;
		cog[0] = cog[1] = 0.0;
		img += ypos*stride + xpos;
		for (int iy=0; iy < w; iy++) { // loop over all pixels
			for (int ix=0; ix < h; ix++) {
				fi = img[ix + iy*stride];
				if (fi > samini) {
					csum += fi;
					cog[0] += fi * ix;      // center of gravity of subaperture intensity 
					cog[1] += fi * iy;
				}
			}
		}
		cog[0] /= csum;
		cog[1] /= csum;
		io->msg(IO_DEB2, "cog @ %d,%d got %g,%g (sum=%g).", xpos, ypos, cog[0], cog[1], csum);
		return csum;		
	}
	
	int subapSel() {
		int csum;
		float cog[2], savec[2] = {0};
		int apmap[subap.x][subap.y];
		int apmap2[subap.x][subap.y];
		
		io->msg(IO_DEB2, "Shwfs::subapSel()");
		
		ns = 0;
		
		// Get image first
		if (cam->get_dtype() == DATA_UINT16) {
			io->msg(IO_DEB2, "Shwfs::subapSel() got DATA_UINT16");
			void *tmpimg;
			cam->get_image(&tmpimg);
			uint16_t *img = (uint16_t *) tmpimg;
			for (int isy=0; isy < subap.y; isy++) { // loops over all grid cells
				for (int isx=0; isx < subap.x; isx++) {
					csum = _cog<uint16_t>(img, isx * sasize.x, isy * sasize.y, sasize.x, sasize.y, cam->get_width(), samini, cog);
					if (csum > 0) {
						apmap[isx][isy] = 1;
						sapos[ns].x = isx * sasize.x; // Subap position
						sapos[ns].y = isy * sasize.y;
						savec[0] += sapos[ns].x; // Sum all subap positions
						savec[1] += sapos[ns].y;
						ns++;
					}
					else
						apmap[isx][isy] = 0;
				}
			}
		}
		else if (cam->get_dtype() == DATA_UINT8) {
			io->msg(IO_DEB2, "Shwfs::subapSel() got DATA_UINT8");

			void *tmpimg;
			cam->get_image(&tmpimg);
			uint8_t *img = (uint8_t *) tmpimg;
			
			for (int isy=0; isy < subap.y; isy++) { // loops over all grid cells
				for (int isx=0; isx < subap.x; isx++) {
					csum = _cog<uint8_t>(img, isx * sasize.x, isy * sasize.y, sasize.x, sasize.y, cam->get_width(), samini, cog);
					if (csum > 0) {
						apmap[isx][isy] = 1;
						sapos[ns].x = isx * sasize.x; // Subap position
						sapos[ns].y = isy * sasize.y;
						savec[0] += sapos[ns].x; // Sum all subap positions
						savec[1] += sapos[ns].y;
						ns++;
					}
					else
						apmap[isx][isy] = 0;
				}
			}
		}
		
		// *Average* subaperture position (wrt the image origin)
		savec[0] /= ns;
		savec[1] /= ns;
		
		io->msg(IO_XNFO, "Found %d subaps with I > %d.", ns, samini);
		
		// Find central aperture by minimizing (subap position - average position)
		int csa = 0;
		float dist, rmin;
		rmin = sqrtf( ((sapos[csa].x - savec[0]) * (sapos[csa].x - savec[0])) 
								 + ((sapos[csa].y - savec[1]) * (sapos[csa].y - savec[1])));
		for (int i=1; i < ns; i++) {
			dist = sqrt(
									((sapos[i].x - savec[0]) * (sapos[i].x - savec[0]))
									+ ((sapos[i].y - savec[1]) * (sapos[i].y - savec[1])));
			if (dist < rmin) {
				rmin = dist;
				csa = i;	// new best guess for central subaperture
			}
		}
		
		io->msg(IO_XNFO, "Central subaperture #%d at (%d,%d)", csa,
						sapos[csa].x, sapos[csa].y);
		
		// Edge erosion: erode the outer -samaxr rings of subapertures
		while (samaxr < 0) {
			int isy, isx;
			io->msg(IO_XNFO, "Running edge erosion iteration...");
			for (int i=0; i<ns; i++) {
				isy = sapos[i].y / sasize.y;
				isx = sapos[i].x / sasize.x;
				// If this subaperture is on the edge, or it does not have 
				// neighbours in all directions, cut it out
				if (isy == 0 || isy > subap.y ||
						isx == 0 || isx > subap.x ||
						apmap[isx-1][isy] == 0 ||
						apmap[isx][isy-1] == 0 ||
						apmap[isx+1][isy] == 0 ||
						apmap[isx][isy+1] == 0) {
					// Don't use this subaperture
					apmap2[isx][isy] = 0;
					for (int j=i; j<ns-1; j++)
						sapos[j] = sapos[j+1];
					ns--;
				}
				else {
					apmap2[isx][isy] = 1;
				}
			}
			for (int isy=0; isy < subap.y; isy++)
				for (int isx=0; isx < subap.x; isx++)
					apmap[isx][isy] = apmap2[isx][isy];
			
			samaxr++;
		}
		
		io->msg(IO_INFO, "Finally found %d subapertures", ns);
		
		return ns;
	}
	
	public:
	Shwfs(config &config) {
		io->msg(IO_DEB2, "Shwfs::Shwfs(config &config)");
		
		conffile = config.filename;
		
		wfstype = config.getstring("type");
		if (wfstype != SHWFS_TYPE) throw exception("Type should be " SHWFS_TYPE " for this class.");
		
		int idx = conffile.find_last_of("/");
		string camcfg = config.getstring("camcfg");
		if (camcfg[0] != '/') camcfg = conffile.substr(0, idx) + "/" + camcfg;
		cam = Camera::create(camcfg);
		
		name = config.getstring("name");
		subap.x = config.getint("subapx");
		subap.y = config.getint("subapy");
		
		sasize.x = cam->get_width() / subap.x;
		sasize.y = cam->get_height() / subap.x;
		
		track.x = sasize.x * config.getdouble("trackx", 0.5);
		track.y = sasize.y * config.getdouble("tracky", 0.5);
		
		samaxr = config.getint("samaxr", -1);
		samini = config.getint("samini", 30);
		
		// Positions for the tracking window and the subapertures in pixel coordinates
		trackpos = new coord_t[subap.x * subap.y];
		sapos = new coord_t[subap.x * subap.y];
				
		io->msg(IO_XNFO, "Shwfs init complete got %dx%d subaps.", subap.x, subap.y);
	}
	
	~Shwfs() {
		io->msg(IO_DEB2, "Shwfs::~Shwfs()");
		if (cam) delete cam;
		delete[] trackpos;
		delete[] sapos;
	}
	
	int calibrate(void) {
		// For SH WFS: select the subapertures to use for processing
		subapSel();
	}
};

Wfs *Wfs::create(config &config) {
	io->msg(IO_DEB2, "Wfs::create(config &config)");
	return new Shwfs(config);
}
