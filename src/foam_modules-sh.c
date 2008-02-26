/*! 
	@file foam_modules-sh.c
	@author @authortim
	@date January 28, 2008

	@brief This file contains modules and functions related to Shack-Hartmann wavefront sensing used in 
	adaptive optics setups.
	
	\section Info
	This module can be used to do Shack Hartmann wavefront sensing.
	
	\section Functions
	
	The functions provided to the outside world are:
	\li modSelSubapts
	\li modCogTrack
	\li modCorrTrack (broken atm)
	\li modGetRef
	\li modParseSH
	
	
	\section License
	This code is licensed under the GPL, version 2.
	
*/

// HEADERS //
/***********/

#include "foam_modules-sh.h"

int modSelSubapts(wfs_t *wfsinfo, float samini, int samxr) {

	// stolen from ao3.c by CUK
	int isy, isx, iy, ix, i, sn=0, nsubap=0; //init sn to zero!!
	float sum=0.0, fi;					// check 'intensity' of a subapt
	float csum=0.0, cs[] = {0.0, 0.0}; 	// for center of gravity
	float cx=0, cy=0;					// for CoG
	float dist, rmin;					// minimum distance
	int csa=0;							// estimate for best subapt
	int (*subc)[2] = wfsinfo->subc;		// lower left coordinates of the tracker windows
	int (*apcoo)[2] = wfsinfo->gridc;	// lower left coordinates of the tracker windows

	float *image = wfsinfo->image;		// source image from sensor
	coord_t res = wfsinfo->res;			// image resolution
	int *cells = wfsinfo->cells;		// cell resolution used (i.e. 8x8)
	int *shsize = wfsinfo->shsize;		// subapt pixel resolution
	
	// we store our subaperture map in here when deciding which subapts to use
	int apmap[cells[0]][cells[1]];		// aperture map
	int apmap2[cells[0]][cells[1]];		// aperture map 2
//	int apcoo[cells[0] * cells[1]][2];  // subaperture coordinates in apmap
	
	logInfo("Selecting subapertures.");
	for (isy=0; isy<cells[1]; isy++) { // loops over all potential subapertures
		for (isx=0; isx<cells[0]; isx++) {
			// check one potential subapt (isy,isx)
			sum=0.0; cs[0] = 0.0; cs[1] = 0.0; csum = 0.0;
			for (iy=0; iy<shsize[1]; iy++) { // sum all pixels in the subapt
				for (ix=0; ix<shsize[0]; ix++) {
					fi = (float) image[isy*shsize[1]*res.x + isx*shsize[0] + ix+ iy*res.x];
					sum += fi;
					// for center of gravity, only pixels above the threshold are used;
					// otherwise the position estimate always gets pulled to the center;
					// good background elimination is crucial for this to work !!!
					fi -= samini;    		// subtract threshold
					if (fi<0.0) fi = 0.0;	// clip
					csum = csum + fi;		// add this pixel's intensity to sum
					cs[0] += + fi * ix;	// center of gravity of subaperture intensity 
					cs[1] += + fi * iy;
				}
			}
			// check if the summed subapt intensity is above zero (e.g. do we use it?)
			if (csum > 0.0) { // good as long as pixels above background exist
				// we add 0.5 to make sure the integer division is 'fair' (e.g. simulate round(), but faster)
				subc[sn][0] = isx*shsize[0]+shsize[0]/4 + (int) (cs[0]/csum+0.5) - shsize[0]/2;	// subapt coordinates
				subc[sn][1] = isy*shsize[1]+shsize[1]/4 + (int) (cs[1]/csum+0.5) - shsize[1]/2;	// TODO: sort this out
							// ^^ coordinate in big image,  ^^ CoG of one subapt, /4 because that's half a trakcer windows

				cx += isx*shsize[0];
				cy += isy*shsize[1];
				apmap[isx][isy] = 1; // set aperture map
				apcoo[sn][0] = isx;
				apcoo[sn][1] = isy;
				sn++;
			} else {
				apmap[isx][isy] = 0; // don't use this subapt
			}
		}
	}
	logInfo("CoG for subapts done, found %d with intensity > 0.", sn);
	
	nsubap = sn; 			// nsubap: variable that contains number of subapertures
	cx = cx / (float) sn; 	// TODO what does this do? why?
	cy = cy / (float) sn;
	
	// determine central aperture that will be reference
	// initial value for rmin is the distance of the first subapt. TODO: this should work, right?
	csa = 0;
	rmin = sqrt((subc[0][0]-cx)*(subc[0][0]-cx) + (subc[0][1]-cy)*(subc[0][1]-cy));

	for (i=0; i<nsubap; i++) {
		dist = sqrt((subc[i][0]-cx)*(subc[i][0]-cx) + (subc[i][1]-cy)*(subc[i][1]-cy));
		if (dist < rmin) {
			rmin = dist;
			csa = i; 		// new best guess for central subaperture
		}
	}
	logDebug("ref best guess: %d (%d,%d)", csa, subc[csa][0], subc[csa][1]);
	// put reference subaperture as subaperture 0
	cs[0]        = subc[0][0]; 		cs[1] 			= subc[0][1];
	subc[0][0]   = subc[csa][0]; 	subc[0][1] 		= subc[csa][1];
	subc[csa][0] = cs[0]; 			subc[csa][1] 	= cs[1];

	// and fix aperture map accordingly
	cs[0]         = apcoo[0][0];	cs[1] 			= apcoo[0][1];
	apcoo[0][0]   = apcoo[csa][0]; 	apcoo[0][1] 	= apcoo[csa][1];
	apcoo[csa][0] = cs[0];			apcoo[csa][1] 	= cs[1];

	logDebug("refapt in 0: (%d,%d), nsubap %d", subc[0][0], subc[0][1], nsubap);
	// center central subaperture; it might not be centered if there is
	// a large shift between the expected and the actual position in the
	// center of gravity approach above
	// TODO: might not be necessary? leftover?
	cs[0] = 0.0; cs[1] = 0.0; csum = 0.0;
	for (iy=0; iy<shsize[1]; iy++) {
		for (ix=0; ix<shsize[0]; ix++) {
			// loop over the whole shsize^2 big ref subapt here, so subc-shsize/4 is the beginning coordinate
			fi = (float) image[(subc[0][1]-shsize[1]/4+iy)*res.x+subc[0][0]-shsize[0]/4+ix];
						
			// for center of gravity, only pixels above the threshold are used;
			// otherwise the position estimate always gets pulled to the center;
			// good background elimination is crucial for this to work !!!
			fi -= samini;
			if (fi < 0.0) fi=0.0;
			csum += fi;
			cs[0] += fi * ix;
			cs[1] += fi * iy;
		}
	}
	
	logDebug("old subx=%d, old suby=%d",subc[0][0],subc[0][1]);
	subc[0][0] += (int) (cs[0]/csum+0.5) - shsize[0]/2; // +0.5 to prevent integer cropping rounding error
	subc[0][1] += (int) (cs[1]/csum+0.5) - shsize[1]/2; // use shsize/2 because or CoG is for a larger subapt (shsize and not shsize/2)
	logDebug("new subx=%d, new suby=%d",subc[0][0],subc[0][1]);

	// enforce maximum radial distance from center of gravity of all
	// potential subapertures if samxr is positive
	if (samxr > 0) {
		sn = 1;
		while (sn < nsubap) {
			if (sqrt((subc[sn][0]-cx)*(subc[sn][0]-cx) + \
				(subc[sn][1]-cy)*(subc[sn][1]-cy)) > samxr) { // TODO: why remove subapts? might be bad subapts
				for (i=sn; i<(nsubap-1); i++) {
					subc[i][0] = subc[i+1][0]; // remove erroneous subapts
					subc[i][1] = subc[i+1][1];
					apcoo[i][0] = apcoo[i+1][0]; // and remove erroneous grid coordinates
					apcoo[i][1] = apcoo[i+1][1];
				}
				nsubap--;
			} else {
				sn++;
			}
		}
	}

	// edge erosion if samxr is negative; 
	// this is good for non-circular apertures
	while (samxr < 0) {
		samxr = samxr + 1;
		// print ASCII map of aperture
		for (isy=0; isy<cells[1]; isy++) {
			for (isx=0; isx<cells[0]; isx++)
				if (apmap[isx][isy] == 0) logDirect(" "); 
				else logDirect("X");
			logDirect("\n");
		}
		
		sn = 1; // start with subaperture 1 since subaperture 0 is the reference
		while (sn < nsubap) {
			isx = apcoo[sn][0]; isy = apcoo[sn][1];
			if ((isx == 0) || (isx >= cells[0]) || (isy == 0) || (isy >= cells[1]) ||
				(apmap[isx-1][isy] == 0) ||
				(apmap[isx+1][isy] == 0) ||
				(apmap[isx][isy-1] == 0) ||
				(apmap[isx][isy+1] == 0)) {
					
				apmap2[isx][isy] = 0;
				for (i=sn; i<(nsubap-1); i++) {
					subc[i][0] = subc[i+1][0];
					subc[i][1] = subc[i+1][1];
					apcoo[i][0] = apcoo[i+1][0];
					apcoo[i][1] = apcoo[i+1][1];
				}
				nsubap--;
			} else {
				apmap2[isx][isy] = 1;
				sn++;
			}
		}

		// copy new aperture map to old aperture map for next iteration
		for (isy=0; isy<cells[1]; isy++)
			for (isx=0; isx<cells[0]; isx++)
				apmap[isx][isy] = apmap2[isx][isy];

	}
	wfsinfo->nsubap = nsubap; // store to global configuration struct
	logInfo("Selected %d usable subapertures", nsubap);
	
	// set remaining subaperture coordinates to 0
	for (sn=nsubap; sn<cells[0]*cells[1]; sn++) {
		subc[sn][0] = 0; 
		subc[sn][1] = 0;
		apcoo[sn][0] = 0; 
		apcoo[sn][1] = 0;
	}
	
	logDebug("final gridcs:");
	for (sn=0; sn<nsubap; sn++) {
		apcoo[sn][1] *= shsize[1];
		apcoo[sn][0] *= shsize[0];
		logDirect("%d (%d,%d) ", sn, wfsinfo->gridc[sn][0], wfsinfo->gridc[sn][1]);
	}
	logDirect("\n");
	
/*	int cfd;
	char *cfn;

	// write subaperture image into file
	cfn = "ao_saim.dat";
	cfd = creat(cfn,0);
	if (write(cfd, image, res.x*res.y) != res.x*res.y)
		logErr("Unable to write subapt img to file");

	close(cfd);
	logInfo("Subaperture definition image saved in file %s",cfn);*/
	return EXIT_SUCCESS;
}

void modCogTrack(wfs_t *wfsinfo, float *aver, float *max, float coords[][2]) {
	// center of gravity tracking here (easy)
	int ix, iy, sn=0;
	float csx, csy, csum, fi; 			// variables for center-of-gravity
	float sum = 0;

	float *image = wfsinfo->image;
//	float *dark = wfsinfo->darkim
//	float *flat = wfsinfo->flatim
	float *corr = wfsinfo->corrim;
	int nsubap = wfsinfo->nsubap;
	int (*subc)[2] = wfsinfo->subc;		// shortcut to the coordinates of the tracker windows
	
	int *cells, *shsize;
	coord_t track;
	
	coord_t res = wfsinfo->res;			// shortcuts to resolution
	cells = wfsinfo->cells;				// number of x-cells y-cells
	shsize = wfsinfo->shsize;			// subapt pixel resolution

	track.x = shsize[0]/2;				// size of the tracker windows. TODO: GLOBALIZE THIS!!
	track.y = shsize[1]/2; 

	*max = 0; 							// store maximum here (init to 0 pointer)
	
//	uint16_t *gp, *dp; // pointers to gain and dark
	float  *ip, *cp; // pointers to raw, processed image

	logDebug("Starting modCogTrack for %d subapts (CoG mode)", nsubap);
	
	// Process the rest of the subapertures here:		
	// loop over all subapertures, except sn=0 (ref subapt)
	for (sn=0;sn<nsubap;sn++) {

		// --->>> might use some pointer incrementing instead of setting
		//        addresses from scratch every time
		// set pointers to various 'images'
		ip = &image[subc[sn][1]*res.x+subc[sn][0]]; // raw image (input)
//		fprintf(stderr, "i: %d", subc[sn][0]*res.x+subc[sn][1]);
//		dp = &dark[sn*shsize[0]*shsize[1]]; // dark (input)
//		gp = &gain[sn*shsize[0]*shsize[1]]; // gain (output)
		cp = &corr[sn*track.x*track.y]; // calibrated image (output)

		// dark and flat correct subaperture, determine statistical quantities
		imcal(cp, ip, NULL, NULL, &sum, max, res, track);

		// center-of-gravity
		csx = 0.0; csy = 0.0; csum = 0.0;
		for (iy=0; iy<track.y; iy++) {
			for (ix=0; ix<track.x; ix++) {
				fi = (float) corr[sn*track.x*track.y+iy*track.x+ix];

				csum += + fi;    // add this pixel's intensity to sum
				csx += + fi * ix; // center of gravity of subaperture intensity 
				csy += + fi * iy;
			}
		} // end loop over pixels to get CoG

		if (csum > 0.0) { // if there is any signal at all
			coords[sn][0] = -csx/csum + track.x/2; // negative for consistency with CT 
			coords[sn][1] = -csy/csum + track.y/2; // /2 because our tracker cells are track[] wide and high
		//	if (sn<0) printf("%d %f %f\n",sn,stx[sn],sty[sn]);
		} 
		else {
			coords[sn][0] = 0.0;
			coords[sn][1] = 0.0;
		}

	} // end of loop over all subapertures

	// average intensity over all subapertures
	// this is incorrect, should be shsize[0]*shsize[1] + track.x*track.y*(nsubap-1)
	// TODO:
	*aver = sum / ((float) (track.x*track.y*nsubap));
}

void modCogFind(wfs_t *wfsinfo, int xc, int yc, int width, int height, float samini, float *sumout, float *cog) {
	int ix, iy;
	coord_t res = wfsinfo->res;			// image resolution
	float *image = wfsinfo->image;		// source image from sensor
	image += yc * res.x + xc;				// forward the pointer to the right coordinate

	float sum=0.0; float cs[] = {0.0, 0.0}; float csum = 0.0;
	float fi;
	
	for (iy=0; iy<height; iy++) { // sum all pixels in the subapt
		for (ix=0; ix<width; ix++) {
			fi = (float) image[ix+ iy*res.x];
			sum += fi;
			// for center of gravity, only pixels above the threshold are used;
			// otherwise the position estimate always gets pulled to the center;
			// good background elimination is crucial for this to work !!!
			fi -= samini;    		// subtract threshold
			if (fi<0.0) fi = 0.0;	// clip
			csum = csum + fi;		// add this pixel's intensity to sum
			cs[0] += + fi * ix;	// center of gravity of subaperture intensity 
			cs[1] += + fi * iy;
		}
	}
	*sumout = csum;
	cog[0] = cs[0]/csum;
	cog[1] = cs[1]/csum;
}

int modParseSH(wfs_t *wfsinfo) {
	// now calculate the offsets 
	float aver=0.0, max=0.0;
	float coords[wfsinfo->nsubap][2];
	int i;

	// track the maxima
	modCogTrack(wfsinfo, &aver, &max, coords);
	
	// note: coords is relative to the center of the tracker window
	// therefore we can simply update the lower left coord by subtracting the coordinates.
	float sum, cog[2];
	float rmsx=0.0, rmsy=0.0;
	//logInfo("Coords: ");
	
	for (i=0; i<wfsinfo->nsubap; i++) {
		// store the displacement vectors (wrt center of subaperture grid):
		// we get subc which is the coordinate of the tracker window wrt the complete sensor image
		// we calculate the remainder of this coordinate with the subaperture grid, such that we get the 
		// coordinate of the tracker window wrt the subaperture grid.
		// After we have this coordinate, we add the displacement we just found which is wrt the center of 
		// the tracker window.
		// Finally we subtract half the size of the tracker window  ( = subaperture grid/4) to fix everything.

//		logDirect("%d (%d,%d) ", i, wfsinfo->gridc[i][0], wfsinfo->gridc[i][1]);
		wfsinfo->disp[i][0] = (wfsinfo->subc[i][0] - wfsinfo->gridc[i][0]);
		wfsinfo->disp[i][0] -= coords[i][0];
		wfsinfo->disp[i][0] -= wfsinfo->shsize[0]/4;

		wfsinfo->disp[i][1] = (wfsinfo->subc[i][1] - wfsinfo->gridc[i][1]);
		wfsinfo->disp[i][1] -= coords[i][1];
		wfsinfo->disp[i][1] -= wfsinfo->shsize[1]/4;

		//logDirect("(%f, %f) ", wfsinfo->disp[i][0], wfsinfo->disp[i][1]);
		rmsx += wfsinfo->disp[i][0] * wfsinfo->disp[i][0];
		rmsy += wfsinfo->disp[i][1] * wfsinfo->disp[i][1];
		
		// check for runaway subapts and recover them:
		if (wfsinfo->disp[i][0] > wfsinfo->shsize[0]/2 || wfsinfo->disp[i][0] < -wfsinfo->shsize[0]/2
			|| wfsinfo->disp[i][1] > wfsinfo->shsize[1]/2 || wfsinfo->disp[i][1] < -wfsinfo->shsize[0]/2) {
		
//			logDebug("Runaway subapt detected! (%f,%f)", wfsinfo->disp[i][0], wfsinfo->disp[i][1]);
			// xc = (wfsinfo->subc[i][0] / wfsinfo->shsize[0]) * wfsinfo->shsize[0];
			// yc = (wfsinfo->subc[i][1] / wfsinfo->shsize[1]) * wfsinfo->shsize[1];
			// modCogFind(wfsinfo, xc, yc, wfsinfo->shsize[0], wfsinfo->shsize[1], 0.0, &sum, cog);
			// 		
			// wfsinfo->subc[i][0] = xc + (int) (cog[0]+0.5) - wfsinfo->shsize[0]/4;	// subapt coordinates
			// wfsinfo->subc[i][1] = yc + (int) (cog[1]+0.5) - wfsinfo->shsize[1]/4;	// subapt coordinates
			// 
			// wfsinfo->disp[i][0] = cog[0];
			// coords[i][1] = cog[1];
			// logDebug("Runaway subapt saved! (%f,%f)", cog[0], cog[1]);
			// exit(0);
		}
		
		// update the tracker window coordinates:
		// +0.5 to make sure rounding (integer clipping) is fair. 
		// Don't forget to cast to int, otherwise problems occur!
		// TODO: this works, is this fast?
		wfsinfo->subc[i][0] -= (int) (coords[i][0]+0.5);//-wfsinfo->res.x/wfsinfo->cells[0]/4;
		wfsinfo->subc[i][1] -= (int) (coords[i][1]+0.5);//-wfsinfo->res.y/wfsinfo->cells[1]/4;

	}
	//logDirect("\n");
	rmsx = sqrt(rmsx/wfsinfo->nsubap);
	rmsy = sqrt(rmsy/wfsinfo->nsubap);
	logInfo("RMS displacement: x: %f, y: %f", rmsx, rmsy);
	
	return EXIT_SUCCESS;
}

int modCalcCtrl(control_t *ptc, int wfs, int nmodes) {
	logDebug("Calculating WFC ctrls");
	// function assumes presence of dmmodes, 
	if (ptc->wfs[wfs].dmmodes == NULL || ptc->wfs[wfs].singular == NULL || ptc->wfs[wfs].wfsmodes == NULL) {
		logWarn("Cannot calculate WFC ctrls, calibration incomplete.");
		return EXIT_FAILURE;
	}
	
	int i,j, wfc, act;   		// index variables
	float sum; 					// temporary sum
	int nacttot=0;
	int nsubap = ptc->wfs[wfs].nsubap;
	
	// calculate total nr of act for all wfc
	for (wfc=0; wfc< ptc->wfc_count; wfc++)
		nacttot += ptc->wfc[wfc].nact;

	if (nmodes > nacttot) {
		logWarn("nmodes cannot be higher than the total number of actuators, cropping");
		nmodes = nacttot;
	}
	
	float modeamp[nacttot];
	
	logDebug("calculating control stuff for WFS %d (modes: %d)", wfs, nmodes);
	
	i=0;
	// first calculate mode amplitudes
	for (wfc=0; wfc< ptc->wfc_count; wfc++) { // loop over all WFCs
		for (act=0; act < ptc->wfc[wfc].nact; act++) { // loop over all acts for WFC 'wfc'
			sum=0.0;

			// TODO: what coordinate of wfsmodes do we need?
			for (j=0; j<nsubap; j++) // loop over all subapertures
				sum = sum + ptc->wfs[wfs].wfsmodes[i*2*nsubap+j*2] * (ptc->wfs[wfs].disp[j][0]-ptc->wfs[wfs].refc[j][0]) \
					+ ptc->wfs[wfs].wfsmodes[i*2*nsubap+j*2+1] * (ptc->wfs[wfs].disp[j][1]-ptc->wfs[wfs].refc[j][1]);
			
			logDirect("%f ", sum);
			modeamp[i] = sum;
			i++;
		}
	}
	
	// apply inverse singular values and calculate actuator amplitudes	
	i=0;
	for (wfc=0; wfc< ptc->wfc_count; wfc++) { // loop over all WFCs
		for (act=0; act < ptc->wfc[wfc].nact; act++) { // loop over all acts for WFC 'wfc'
			sum=0.0;
			for (j=0;j<nmodes;j++) // loop over all system modes that are used
				sum += ptc->wfs[wfs].dmmodes[i*nacttot+j]*modeamp[j]/ptc->wfs[wfs].singular[j];

			logDirect("%f ", sum);
			ptc->wfc[wfc].ctrl[act] = sum;
			i++;
		}
	}
	logDirect("\n");
	
	return EXIT_SUCCESS;
}

float sae(float *subapt, float *refapt, int len) {
	// *ip     pointer to first pixel of each subaperture
	// *rp     pointer to first pixel of (shifted) reference

	float sum=0;
	int i;
	
	// loop over all pixels
	for (i=0; i < len; i++)
		sum += fabs(subapt[i] - refapt[i]);

	return sum;
}

void procRef(wfs_t *wfsinfo, float *sharp, float *aver) {
//	uint8_t  pixel;
//	uint16_t in16, d16, g16;
	float in, dd, gg;  // these types should be higher than the image type for internal calculation
	float pixel;

	float *image = wfsinfo->image;	// full image
	float *ref = wfsinfo->refim;	// reference image
	float *dark = wfsinfo->darkim; 	// dark image
	float *flat = wfsinfo->flatim; 	// flat img
	float si=0, sqi=0;				// some summing variables
	float a=0, b=0, c=0, d=0; 		// summed quadrant intensities
	
	int ix, iy;	// counters

	coord_t res = wfsinfo->res;		// resolution of *image
	int *shsize = wfsinfo->shsize;	// subapt pixel resolution
	int (*subc)[2] = wfsinfo->subc;		// coordinates of the tracker windows 
	
	for (iy=0; iy<shsize[1]; iy++) {
		for (ix=0; ix<shsize[0]; ix++) {
			// TvW: why *256 and /4?
			// TvW: TODO this! 
			// Also: combine this with imcal? same procedure right?
			/*
			in = (float) image[(subc[0][1]-shsize[1]/4+iy)*res.x+subc[0][0]-shsize[0]/4+ix]*256;
			dd = (float) dark[(subc[0][1]-shsize[1]/4+iy)*res.x+subc[0][0]-shsize[0]/4+ix]/4;
			if (in > dd) in -= dd;
			else in = 0;
			gg = (float) (1.07374182E+09 / (float) 
				( flat[(subc[0][1]-shsize[1]/4+iy)*res.x+subc[0][0]-shsize[0]/4+ix] - 
				dark[(subc[0][1]-shsize[1]/4+iy)*res.x+subc[0][0]-shsize[0]/4+ix]));
			// 1.07374182E+09 is 2^30 
			// What's this? Only works on ints?
			pixel = (float) (((double) in * (double) gg) / (1 << 21));
			*/
			pixel = image[(subc[0][1]-shsize[1]/4+iy)*res.x+subc[0][0]-shsize[0]/4+ix];
			
			ref[iy* shsize[0] + ix] = pixel;
			// TvW: this is not used at the moment:
			// si += pixel;
			// sqi += pixel;
		}
	}

	switch (wfsinfo->scandir) {
		case AO_AXES_XY:
		    // the following defines the sharpness as the rms contrast
		    // --->>> maybe we should only use the SX*SY subaperture and not the full
		    //        RX*RY reference aperture because of flatfielding at the edges
		    //  *sharp = sqrt( (sqi - ((float) si *(float) si)/((float) (RX*RY))) 
		    //		 / (float) (RX*RY-1) );

		    // the following is useful on spots, which returns the inverse
		    // distance from a centered spot using a quad-cell approach
	
			// TvW: check this!
			for (iy=shsize[1]/4; iy<shsize[1]/2; iy++) {
				for (ix=shsize[0]/4; ix<shsize[0]/2; ix++) a=a+ref[iy*shsize[0]+ix]; // lower left
				for (ix=shsize[0]/2; ix<shsize[0]*3/4; ix++) b=b+ref[iy*shsize[0]+ix]; // lower right 
			}
			for (iy=shsize[1]/2; iy<shsize[1]*3/4; iy++) {
				for (ix=shsize[0]/4; ix<shsize[0]/2; ix++) c=c+ref[iy*shsize[0]+ix]; // upper left
				for (ix=shsize[0]/2; ix<shsize[0]*3/4; ix++) d=d+ref[iy*shsize[0]+ix]; // upper right
			}
		    // adding +1 prevents division by zero
		    *sharp = (float) (a+b+c+d) / (float) (abs(a+b-c-d)+abs(a+c-b-d)+1); 
		    break;

		case AO_AXES_X: // vertical limb, maximize intensity difference 
			// between left and right
			for (iy=shsize[1]/4; iy<shsize[1]*3/4; iy++) {
				for (ix=shsize[0]/4; ix<shsize[0]/2; ix++) a=a+ref[iy*shsize[0]+ix]; //  left
				for (ix=shsize[0]/2; ix<shsize[0]*3/4; ix++) b=b+ref[iy*shsize[0]+ix]; //  right 
			}
			// TvW: why /64?
			*sharp = (float) fabs(a-b)/64.0;
			break;

		case AO_AXES_Y: // horizontal limb, maximize intensity difference 
			// between top and bottom
			// TvW: continue
			for (ix=shsize[0]/4; ix<shsize[0]*3/4; ix++) {
				for (iy=shsize[1]/4; iy<shsize[1]/2; iy++) a=a+ref[iy*shsize[0]+ix]; //  left
				for (iy=shsize[1]/2; iy<shsize[1]*3/4; iy++) b=b+ref[iy*shsize[0]+ix]; //  right 
			}
			*sharp = (float) fabs(a-b)/64.0; 
			break;

	} // end switch (wfsinfo->scandir)

	// average intensity over subaperture
	si=0;
	for (iy=shsize[1]/4; iy<shsize[1]*3/4; iy++)
		for (ix=shsize[0]/4; ix<shsize[0]*3/4; ix++)
			si += ref[iy*shsize[0]+ix];
			
	*aver = (float) (si) / (float) (shsize[0]/2*shsize[1]/2);

} // end of procref

// TODO: on the one hand depends on int wfs, on the other hand needs int window[2], fix that
void imcal(float *corrim, float *image, float *darkim, float *flatim, float *sum, float *max, coord_t res, coord_t window) {
	// substract the dark, multiply with the flat (right?)
	// TODO: dark and flat currently ignored, fix that
	int i,j;
	
	// corrim comes from:
	// cp = &corr[sn*shsize[0]*shsize[1]]; // calibrated image (output)
	// image comes form:
	// ip = &image[subc[sn][0]*res.x+subc[sn][1]]; // raw image (input)
		
	// we do shsize/2 because the tracker windows are only a quarter of the
	// total tracker area
	for (i=0; i<window.y; i++) {
		for (j=0; j<window.x; j++) {
			// We correct the image stored in 'image' by applying some dark and flat stuff,
			// and copy it to 'corrim' which has a different format than image.
			// Image is a row-major array of res.x * res.y, where subapertures are hard to
			// read out (i.e. you have to skip pixels and stuff), while corrim is reformatted
			// in such a way that the first shsize[0]*shsize[1] pixels are exactly one subapt,
			// and each consecutive set is again one seperate subapt. This way you can loop
			// through subapts with only one counter (right?)

			corrim[i*window.x + j] = image[i*res.x + j];
			*sum += corrim[i*window.x + j];
			if (corrim[i*window.x + j] > *max) *max = corrim[i*window.x + j];
		}
	}			
	
}

/*
void modCorrTrack(wfs_t *wfsinfo, float *aver, float *max, float coords[][2]) {
	// this will contain correlation tracking, but first we need to get reference images

#define NP 5
#define NO NP/2

	// when working at the limb, we do not correct for transparency 
	// variations since there is a coupling between subaperture intensity
	// and overall shift
	//
	// --->>> but we should really figure out a way to correct anyway at
	// the limb since the transparency variations influence the limb
	// position; we might use pixels well within the disk to separate the
	// transparency fluctuations from the limb position

	// TvW: what does this mean again?
//	if (wfsinfo->scandir != AO_AXES_XY) corr = 1.0;

	float *image = wfsinfo->image;		// the real image (input)
//	float *dark = wfsinfo->darkim
//	float *flat = wfsinfo->flatim
	float *corr = wfsinfo->corrim;		// the corrected image (output)
	float *refim = wfsinfo->refim;		// the reference image (input)
	float *ip, *cp, *rp;				// pointers to various images (temp)
	coord_t res = wfsinfo->res;			// resolution of *image and *refim
	int track[] = {res.x/wfsinfo->cells[0]/2, res.y/wfsinfo->cells[1]/2};
										// tracking windows for the subapertures
	int (*subc)[2] = wfsinfo->subc;		// coordinates of the tracker windows 
	
	int nsubap = wfsinfo->nsubap;		// number of subapts
	float d, diff[NP][NP], cmin, msae[nsubap];	// variables to hold the SAE as function of displacement
	int sn;

	float sig[NP];
	float tmpmax, sum;

	static float sxx, sxxxx, rnp;     		// fixed values for parabola fit
	static float sy,sxy,sxxy,x,y,a,b,da,db; // variable values for parabola fit
	int ix=0, iy=0;
	
	// precompute some values needed to fit a parabola
	// TODO: this is unecessary, rewrite this parabola fitting
	// --->>> we could do this only once, not every time shtracker is called
	sxx   = 0.0;
	sxxxx = 0.0;
	for (ix=0; ix<NP; ix++) {
		x = ix - NO;
		sxx   = sxx   + x*x;
		sxxxx = sxxxx + x*x*x*x;
	}
	rnp = 1.0 / (float) NP;
	da = 1.0 / (sxxxx-rnp*sxx*sxx);
	db = 1.0 / sxx;

	// correction factor for transparency fluctuations (e.g. cirrus)
	// 32768 = 2^15
	// TvW: why?
//	for (i=0;i<4;i++) tmpcorr4[i] = (uint16_t) (corr * 32768.0);

	// set max to zero manually, init some other vars
	*max = 0;
	tmpmax = 0;
	sum = 0;
	
	
	// BEGIN SUBAPERTURE LOOP //
	////////////////////////////
	for (sn=0;sn<nsubap;sn++) {

		// --->>> might use some pointer incrementing instead of setting
		//        addresses from scratch every time
		ip = &image[subc[sn][1]*res.x+subc[sn][0]]; // raw image (input)
//		dp = &dark[sn*shsize[0]*shsize[1]]; // dark (input)
//		gp = &gain[sn*shsize[0]*shsize[1]]; // gain (output)
		cp = &corr[sn*track[0]*track[1]]; // calibrated image (output)
		

		// dark and flat correct subaperture, determine statistical quantities
		imcal(cp, ip, NULL, NULL, &sum, max, res, track);
		
		// update the maximum value
		if (tmpmax > *max)
			*max = tmpmax;
			
		// add the sum of this subapt to the average (divide by nsubapt later)
		*aver += sum;
		
	    cmin = 1000000; // large positive integer 
		// TODO: ugly, how to fix?

	    switch (wfsinfo->scandir) {
			case AO_AXES_XY:
			////////////////

			ip = &corr[sn*track[0]*track[1]]; // first pixel of each corrected subaperture
			for (ix=-NO; ix<=NO; ix++) {
				// --->>> might explicitely unroll the inner loop
				for (iy=-NO;iy<=NO;iy++) {
					rp = &refim[(iy+track[1]/2)*track[0]*2+ix+track[0]/2]; // first pixel of shifted reference
					d = sae(ip,rp, track[1]*track[0]);
					// --->>> squaring could be done in assembler code
					diff[ix+NO][iy+NO] = d*d; // square of SAD
				}
			}
			break;

			case AO_AXES_X:
			////////////////
			
			ip = &corr[sn*track[0]*track[1]]; // first pixel of each corrected subaperture
			for (ix=-NO; ix<=NO; ix++) {
				rp = &refim[(iy+track[1]/2)*track[0]*2+ix+track[0]/2]; // first pixel of shifted reference
				d = sae(ip,rp, track[1]*track[0]);
				diff[ix+NO][NO] = d;
				if (cmin>d) {
					cmin = d;
				//  ndx = dx+ix;
				}
			}
			break;

			case AO_AXES_Y:
			////////////////
			
			ip = &corr[sn*track[0]*track[1]]; // first pixel of each corrected subaperture
			for (iy=-NO; iy<=NO; iy++) {
				rp = &refim[(iy+track[1]/2)*track[0]*2+ix+track[0]/2]; // first pixel of shifted reference
				d = sae(ip,rp, track[1]*track[0]);
				diff[NO][iy+NO] = d;
				if (cmin>d) {
					cmin = d;
				//  ndy = dy+iy;
				}
			}
			break;
		} // end of switch(axes) statement


	    msae[sn] = cmin;            // minimum sum of absolute differences 

	    // subpixel interpolation of minimum position
	    // currently uses two 1-D approaches

		// Interpolation along the X direction //
		/////////////////////////////////////////
		if ((wfsinfo->scandir==AO_AXES_X) || (wfsinfo->scandir==AO_AXES_XY)) {
			if (wfsinfo->scandir==AO_AXES_XY) { // average values over the y-axis
				for (ix=0; ix<NP; ix++) { 
					sig[ix] = 0.0;
					for (iy=0; iy<NP; iy++) sig[ix] = sig[ix] + (float) diff[ix][iy];
				}
			} else {
				for (ix=0; ix<NP; ix++) {
					sig[ix] = diff[ix][NO];
				}
			}
			sy   = 0.0;
			sxy  = 0.0;
			sxxy = 0.0;
			
			for (ix=0; ix<NP; ix++) {
				x = ix - NO;
				y = sig[ix];
				sy   = sy + y;			// First Moment (sum or avg)
				sxy  = sxy + x*y;		// Second moment
				sxxy = sxxy + x*x*y;	// Third moment
			}

			if (sy > 0.0) {
				a = (sxxy-rnp*sxx*sy) * da;
				b = sxy * db;
				if (a != 0.0) 
					coords[sn][0] = -0.5 * b/a;
				else 
					coords[sn][0] = 0.0;
			} 
			else {
				coords[sn][0] = 0.0;
			}
		} // End for: if ((wfsinfo->scandir==AO_AXES_X) || (wfsinfo->scandir==AO_AXES_XY))
		else {
			coords[sn][0] = 0.0;
		}

		// Interpolation along the Y direction //
		/////////////////////////////////////////
		if ((wfsinfo->scandir==AO_AXES_Y) || (wfsinfo->scandir==AO_AXES_XY)) {
			if (wfsinfo->scandir==AO_AXES_XY) { // average values over the x-axis
				for (iy=0; iy<NP; iy++) {
					sig[iy] = 0.0;
					for (ix=0; ix<NP; ix++) sig[iy] = sig[iy] + (float) diff[ix][iy];
				}
			} 
			else {
				for (iy=0; iy<NP; iy++) {
					sig[iy] = diff[NO][iy];
				}
			}
		
			sy   = 0.0;
			sxy  = 0.0;
			sxxy = 0.0;
			for (ix=0; ix<NP; ix++) {
				x = ix - NO;
				y = sig[ix];
				sy   = sy   + y;
				sxy  = sxy  + x*y;
				sxxy = sxxy + x*x*y;
			}
			if (sy > 0.0) {
				a = (sxxy-rnp*sxx*sy) * da;
				b = sxy * db;
				if (a != 0.0) 
					coords[sn][1] = -0.5 * b/a; // +(float) (dy); 
				else 
					coords[sn][1] = 0.0;
			} 
			else {
				coords[sn][1] = 0.0;
			}
		} 
		else {
			coords[sn][1] = 0.0;
		} // End for: if ((wfsinfo->scandir==AO_AXES_Y) || (wfsinfo->scandir==AO_AXES_XY))
	}
	// END SUBAPERTURE LOOP //
	//////////////////////////

	// TvW: why do we use corr?
	// what is si?
	// what do we need the statistics for?

	// average intensity over all subapertures
	// divide by number of pixels, we already summed it above
	*aver /= (track[0]*track[1]*nsubap);


	// calculate average reference subaperture intensity if new reference 
	// was just taken
	// TvW: what's this then?
	
	// if (*newref == AO_REFIM_NONE) { // new reference is valid
	// 	printf("old correction factor: %f\n",corr);
	// 	refaver = 0.0;  // average reference subaperture intensity
	// 	for (ix=0;ix<SX;ix++)
	// 		for (iy=0;iy<SY;iy++)
	// 			refaver = refaver + (float) refim[(iy+4)*RX+ix+4];
	// 	
	// 	refaver = refaver/(float) (SX*SY);
	// 	corr = 1.0; // reset brightness correction
	// 	*newref = AO_REFIM_GOOD;
	// 	printf("new reference, correction factor: %f\n",corr);
	// }

	// calculate intensity correction factor based on average intensity
	// if (refaver == 0.0)
	// 	corr = 1.0;
	// else {
	// 	if (*aver>0) corr = corr * refaver / *aver; // update correction factor
	// 	// if correction factor is 2 or larger, then the multiplication
	// 	// with 2^15 above 'flips' over to something close to zero; we should
	// 	// allow at least a factor of 4, but we still need to catch that case
	// 	// (maybe using saturated multiplication)
	// 	if (corr>1.999) corr=1.999;
	// }

}

void modGetRef(wfs_t *wfsinfo) {
	float sharp, aver, bestsharp=0;			// store some stats in here
	int i, refcount;
	int *shsize = wfsinfo->shsize;			// subapt pixel resolution
	
	float refbest[shsize[0] * shsize[1]]; 	// best reference image storage

	for (refcount=0; refcount<1024; refcount++) {
		procRef(wfsinfo, &sharp, &aver);
		if (sharp > bestsharp) { // the new potential reference is sharper
			for (i=0; i<shsize[1]*shsize[0]; i++)
				refbest[i] = wfsinfo->refim[i];
				
		}
	}
	
	// store the best reference image
	for (i=0; i<shsize[0]*shsize[1]; i++)
		wfsinfo->refim[i] = refbest[i];

	logInfo("Got new reference image, sharp: %f, aver: %f", sharp, aver);
	// We have a new ref, check the displacement vectors for this new ref
	// TvW: TODO
}

void modCalcWFCVolt(control_t *ptc, int wfs, int nmodes) {
	//, float sdx[NS], float sdy[NS], float actvol[DM_ACTUATORS],
	//                float modeamp[DM_ACTUATORS], )
		// sdx     subaperture shifts in x
		// sdy     subaperture shifts in y
		// actvol  desired change in actuator voltage squared
		// modeamp system mode amplitudes
		// nmodes  number of system modes to use

	int i,j;   // index variables
	float sum; // temporary sum

	int nact;
	
	// get total nr of actuators
	for (i=0; i < ptc->wfc_count; i++)
		nact += ptc->wfc[i].nact;

	// TvW continue here:
	/////////////////////
	
	// new code using system mode amplitudes
	// first calculate mode amplitudes
	for (i=0; i<nact ;i++) { // loop over all actuators
		sum=0.0;
		for (j=0;j<ptc->wfs[wfs].nsubap;j++) // loop over all subapertures
			sum = sum + wfsmodes[i*2*NS+j*2]*sdx[j] + wfsmodes[i*2*NS+j*2+1]*sdy[j];
			
		modeamp[i] = sum;
	}
	
	// apply inverse singular values and calculate actuator amplitudes
	for (i=0;i<nact;i++) { // loop over all actuators
		sum=0.0;
		for (j=0;j<nmodes;j++) // loop over all system modes that are used
			sum = sum + dmmodes[i*DM_ACTUATORS+j]*modeamp[j]/singval[j];
			
		ptc->wfs[wfs].ctrl[i] = sum;
	}


}
*/
