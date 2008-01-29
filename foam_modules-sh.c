/*! 
	@file foam_modules-sh.c
	@author @authortim
	@date January 28, 2008

	@brief This file contains modules and functions related to Shack-Hartmann wavefront sensing used in 
	adaptive optics setups.
	
*/

// HEADERS //
/***********/

#include "foam_modules-sh.h"

void selectSubapts(float *image, float samini, int samxr, int wfs) {
	// stolen from ao3.c by CUK
	int isy, isx, iy, ix, i, sn=0, nsubap=0; //init sn to zero!!
	float sum=0.0, fi;			// check 'intensity' of a subapt
	float csum=0.0, cs[] = {0.0,0.0}; // for center of gravity
	int shsize[2];			// size of one subapt (in pixels)
	int res[2], cells[2];	// size of whole image, nr of cells
	float cx=0, cy=0;		// for CoG
	float dist, rmin;		// minimum distance
	int csa=0;				// estimate for best subapt
	int (*subc)[2] = ptc.wfs[wfs].subc;	// lower left coordinates of the tracker windows

	res[0] = ptc.wfs[wfs].res[0];	// shortcuts
	res[1] = ptc.wfs[wfs].res[1];
	cells[0] = ptc.wfs[wfs].cells[0];
	cells[1] = ptc.wfs[wfs].cells[1];

	shsize[0] = res[0]/cells[0]; 		// size of subapt cell in x
	shsize[1] = res[1]/cells[1];		// size of subapt cell in y
	
	// we store our subaperture map in here when deciding which subapts to use
	int apmap[cells[0]][cells[1]];		// aperture map
	int apmap2[cells[0]][cells[1]];		// aperture map 2
	int apcoo[cells[0] * cells[1]][2];  // subaperture coordinates in apmap
	
	for (isy=0; isy<cells[1]; isy++) { // loops over all potential subapertures
		for (isx=0; isx<cells[0]; isx++) {
			// check one potential subapt (isy,isx)

			sum=0.0; cs[0] = 0.0; cs[1] = 0.0; csum = 0.0;
			for (iy=0; iy<shsize[1]; iy++) { // sum all pixels in the subapt
				for (ix=0; ix<shsize[0]; ix++) {
					fi = (float) image[isy*shsize[1]*res[0] + isx*shsize[0] + ix+ iy*res[0]];
					sum += fi;
					/* for center of gravity, only pixels above the threshold are used;
					otherwise the position estimate always gets pulled to the center;
					good background elimination is crucial for this to work !!! */
					fi -= samini;    		// subtract threshold
					if (fi<0.0) fi = 0.0;	// clip
					csum = csum + fi;		// add this pixel's intensity to sum
					cs[0] += + fi * ix;	// center of gravity of subaperture intensity 
					cs[1] += + fi * iy;
				}
			}

			// check if the summed subapt intensity is above zero (e.g. do we use it?)
			if (csum > 0.0) { // good as long as pixels above background exist
				subc[sn][0] = isx*shsize[0]+shsize[0]/4 + (int) (cs[0]/csum) - shsize[0]/2;	// subapt coordinates
				subc[sn][1] = isy*shsize[1]+shsize[1]/4 + (int) (cs[1]/csum) - shsize[1]/2;	// TODO: sort this out
							// ^^ coordinate in big image,  ^^ CoG of one subapt, 							
																				// why /4? (partial subapt?)
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
	// put reference subaperture as subaperture 0
	cs[0]        = subc[0][0]; 		cs[1] 			= subc[0][1];
	subc[0][0]   = subc[csa][0]; 	subc[0][1] 		= subc[csa][1];
	subc[csa][0] = cs[0]; 			subc[csa][1] 	= cs[1];

	// and fix aperture map accordingly
	cs[0]         = apcoo[0][0];	cs[1] 			= apcoo[0][1];
	apcoo[0][0]   = apcoo[csa][0]; 	apcoo[0][1] 	= apcoo[csa][1];
	apcoo[csa][0] = cs[0];			apcoo[csa][1] 	= cs[1];

	// center central subaperture; it might not be centered if there is
	// a large shift between the expected and the actual position in the
	// center of gravity approach above
	// TODO: might not be necessary? leftover?
	cs[0] = 0.0; cs[1] = 0.0; csum = 0.0;
	for (iy=0; iy<shsize[1]; iy++) {
		for (ix=0; ix<shsize[0]; ix++) {
			fi = (float) image[(subc[0][1]-0+iy)*res[0]+subc[0][0]-0+ix]; // TODO: fix the static '4' --> -0 works, but *why*?
			
			/* for center of gravity, only pixels above the threshold are used;
			otherwise the position estimate always gets pulled to the center;
			good background elimination is crucial for this to work !!! */
			fi -= samini;
			if (fi < 0.0) fi=0.0;
			csum += fi;
			cs[0] += fi * ix;
			cs[1] += fi * iy;
		}
	}
	
	logDebug("old subx=%d, old suby=%d",subc[0][0],subc[0][1]);
	subc[0][0] += (int) (cs[0]/csum+0.5) - shsize[0]/4; // +0.5 rounding error
	subc[0][1] += (int) (cs[1]/csum+0.5) - shsize[1]/4; // also the reference subapt is the same size as the rest
	logDebug("new subx=%d, new suby=%d",subc[0][0],subc[0][1]);

	// enforce maximum radial distance from center of gravity of all
	// potential subapertures if samxr is positive
	if (samxr > 0) {
		sn = 1;
		while (sn < nsubap) {
			if (sqrt((subc[sn][0]-cx)*(subc[sn][0]-cx) + \
				(subc[sn][1]-cy)*(subc[sn][1]-cy)) > samxr) { // TODO: why remove subapts? might be bad subapts
				for (i=sn; i<(nsubap-1); i++) {
					subc[i][0] = subc[i+1][0];
					subc[i][1] = subc[i+1][1];
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
				if (apmap[isx][isy] == 0) printf(" "); 
				else printf("X");
			printf("\n");
		}
		
		sn = 1; // start with subaperture 1 since subaperture 0 is the reference
		while (sn < nsubap) {
			isx = apcoo[sn][0]; isy = apcoo[sn][1];
			if ((isx == 0) || (isx >= 15) || (isy == 0) || (isy >= 15) ||
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
	ptc.wfs[wfs].nsubap = nsubap; // store to global configuration struct
	logInfo("Selected %d usable subapertures", nsubap);
	
	// set remaining subaperture coordinates to 0
	for (sn=nsubap; sn<cells[0]*cells[1]; sn++) {
		subc[sn][0] = 0; 
		subc[sn][1] = 0;
	}
	
/*	int cfd;
	char *cfn;

	// write subaperture image into file
	cfn = "ao_saim.dat";
	cfd = creat(cfn,0);
	if (write(cfd, image, res[0]*res[1]) != res[0]*res[1])
		logErr("Unable to write subapt img to file");

	close(cfd);
	logInfo("Subaperture definition image saved in file %s",cfn);*/
}

void cogTrack(int wfs, float *aver, float *max, float coords[][2]) {
	// center of gravity tracking here (easy)
	int ix, iy, sn=0;
	float csx, csy, csum, fi; 			// variables for center-of-gravity
	float sum = 0;

	float *image = ptc.wfs[wfs].image;
//	float *dark = ptc.wfs[wfs].darkim
//	float *flat = ptc.wfs[wfs].flatim
	float *corr = ptc.wfs[wfs].corrim;
	int nsubap = ptc.wfs[wfs].nsubap;
	int (*subc)[2] = ptc.wfs[wfs].subc;	// TODO: does this work?
	
	int res[2], cells[2], shsize[2];
	res[0] = ptc.wfs[wfs].res[0];		// shortcuts, TODO: can be done faster like res = ptc.wfs.res ?
	res[1] = ptc.wfs[wfs].res[1];
	cells[0] = ptc.wfs[wfs].cells[0];
	cells[1] = ptc.wfs[wfs].cells[1];

	shsize[0] = res[0]/cells[0]; 		// size of subapt cell in x (width)
	shsize[1] = res[1]/cells[1];		// size of subapt cell in y (height)
	int track[] = {shsize[0]/2, shsize[1]/2}; // size of the tracker windows. TODO: GLOBALIZE THIS!!

	*max = 0; // set maximum of all raw subapertures to 0
	
//	uint16_t *gp, *dp; // pointers to gain and dark
	float  *ip, *cp; // pointers to raw, processed image

	logDebug("Starting cogTrack for %d subapts (CoG mode)", nsubap);
	
	// Process the rest of the subapertures here:		
	// loop over all subapertures, except sn=0 (ref subapt)
	for (sn=0;sn<nsubap;sn++) {

		// --->>> might use some pointer incrementing instead of setting
		//        addresses from scratch every time
		// set pointers to various 'images'
		ip = &image[subc[sn][1]*res[0]+subc[sn][0]]; // raw image (input)
//		fprintf(stderr, "i: %d", subc[sn][0]*res[0]+subc[sn][1]);
//		dp = &dark[sn*shsize[0]*shsize[1]]; // dark (input)
//		gp = &gain[sn*shsize[0]*shsize[1]]; // gain (output)
		cp = &corr[sn*track[0]*track[1]]; // calibrated image (output)

		// dark and flat correct subaperture, determine statistical quantities
		imcal(cp, ip, NULL, NULL, wfs, &sum, max, track);

		// center-of-gravity
		csx = 0.0; csy = 0.0; csum = 0.0;
		for (iy=0; iy<track[1]; iy++) {
			for (ix=0; ix<track[0]; ix++) {
				fi = (float) corr[sn*track[0]*track[1]+iy*track[0]+ix];

				csum += + fi;    // add this pixel's intensity to sum
				csx += + fi * ix; // center of gravity of subaperture intensity 
				csy += + fi * iy;
			}

			if (csum > 0.0) { // if there is any signal at all
				coords[sn][0] = -csx/csum + track[0]/2; // negative for consistency with CT 
				coords[sn][1] = -csy/csum + track[1]/2; // /2 because our tracker cells are track[] wide and high
			//	if (sn<0) printf("%d %f %f\n",sn,stx[sn],sty[sn]);
			} 
			else {
				coords[sn][0] = 0.0;
				coords[sn][1] = 0.0;
			}
		}

	} // end of loop over all subapertures

	// average intensity over all subapertures
	// this is incorrect, should be shsize[0]*shsize[1] + track[0]*track[1]*(nsubap-1)
	// TODO:
	*aver = sum / ((float) (track[0]*track[1]*nsubap));
}

void corrTrack(int wfs, float *aver, float *max, float coords[][2]) {
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
//	if (ptc.wfs[wfs].scandir != AO_AXES_XY) corr = 1.0;

	float *image = ptc.wfs[wfs].image;		// the real image (input)
//	float *dark = ptc.wfs[wfs].darkim
//	float *flat = ptc.wfs[wfs].flatim
	float *corr = ptc.wfs[wfs].corrim;		// the corrected image (output)
	float *refim = ptc.wfs[wfs].refim;		// the reference image (input)
	float *ip, *cp, *rp;					// pointers to various images (temp)
	int res[] = {ptc.wfs[wfs].res[0], ptc.wfs[wfs].res[1]};	// resolution of *image and *refim
	int track[] = {ptc.wfs[wfs].res[0]/ptc.wfs[wfs].cells[0]/2, ptc.wfs[wfs].res[1]/ptc.wfs[wfs].cells[1]/2};
											// tracking windows for the subapertures
	int (*subc)[2] = ptc.wfs[wfs].subc;		// coordinates of the tracker windows 
	
	int nsubap = ptc.wfs[wfs].nsubap;		// number of subapts
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
	/**************************/
	for (sn=0;sn<nsubap;sn++) {

		// --->>> might use some pointer incrementing instead of setting
		//        addresses from scratch every time
		ip = &image[subc[sn][1]*res[0]+subc[sn][0]]; // raw image (input)
//		dp = &dark[sn*shsize[0]*shsize[1]]; // dark (input)
//		gp = &gain[sn*shsize[0]*shsize[1]]; // gain (output)
		cp = &corr[sn*track[0]*track[1]]; // calibrated image (output)
		

		// dark and flat correct subaperture, determine statistical quantities
		imcal(cp, ip, NULL, NULL, wfs, &sum, &tmpmax, track);
		
		// update the maximum value
		if (tmpmax > *max) 
			*max = tmpmax;
			
		// add the sum of this subapt to the average (divide by nsubapt later)
		*aver += sum;
		
	    cmin = 1000000; /* large positive integer */
		// TODO: ugly, how to fix?

	    switch (ptc.wfs[wfs].scandir) {
			case AO_AXES_XY:
			//*************//

			ip = &corr[sn*track[0]*track[1]]; // first pixel of each corrected subaperture
			for (ix=-NO; ix<=NO; ix++) {
				// --->>> might explicitely unroll the inner loop
				for (iy=-NO;iy<=NO;iy++) {
					rp = &refim[(iy+track[1]/2)*res[0]+ix+track[0]/2]; // first pixel of shifted reference
					d = sae(ip,rp, track[1]*track[0]);
					// --->>> squaring could be done in assembler code
					diff[ix+NO][iy+NO] = d*d; // square of SAD
				}
			}
			break;

			case AO_AXES_X:
			//*************//
			
			ip = &corr[sn*track[0]*track[1]]; // first pixel of each corrected subaperture
			for (ix=-NO; ix<=NO; ix++) {
				rp = &refim[(iy+track[1]/2)*res[0]+ix+track[0]/2]; // first pixel of shifted reference
				d = sae(ip,rp, track[1]*track[0]);
				diff[ix+NO][NO] = d;
				if (cmin>d) {
					cmin = d;
				//  ndx = dx+ix;
				}
			}
			break;

			case AO_AXES_Y:
			//*************//
			
			ip = &corr[sn*track[0]*track[1]]; // first pixel of each corrected subaperture
			for (iy=-NO; iy<=NO; iy++) {
				rp = &refim[(iy+track[1]/2)*res[0]+ix+track[0]/2]; // first pixel of shifted reference
				d = sae(ip,rp, track[1]*track[0]);
				diff[NO][iy+NO] = d;
				if (cmin>d) {
					cmin = d;
				//  ndy = dy+iy;
				}
			}
			break;
		} // end of switch(axes) statement


	    msae[sn] = cmin;            /* minimum sum of absolute differences */

	    // subpixel interpolation of minimum position
	    // currently uses two 1-D approaches

		// Interpolation along the X direction
		//**********************************//
		if ((ptc.wfs[wfs].scandir==AO_AXES_X) || (ptc.wfs[wfs].scandir==AO_AXES_XY)) {
			if (ptc.wfs[wfs].scandir==AO_AXES_XY) { // average values over the y-axis
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
		} // End for: if ((ptc.wfs[wfs].scandir==AO_AXES_X) || (ptc.wfs[wfs].scandir==AO_AXES_XY))
		else {
			coords[sn][0] = 0.0;
		}

		// Interpolation along the Y direction
		//**********************************//
		if ((ptc.wfs[wfs].scandir==AO_AXES_Y) || (ptc.wfs[wfs].scandir==AO_AXES_XY)) {
			if (ptc.wfs[wfs].scandir==AO_AXES_XY) { // average values over the x-axis
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
		} // End for: if ((ptc.wfs[wfs].scandir==AO_AXES_Y) || (ptc.wfs[wfs].scandir==AO_AXES_XY))
	}
	// END SUBAPERTURE LOOP //
	/************************/
	
	// TvW: why do we use corr?
	// what is si?
	// what do we need the statistics for?

	// average intensity over all subapertures
	// divide by number of pixels, we already summed it above
	*aver /= (track[0]*track[1]*nsubap);


	// calculate average reference subaperture intensity if new reference 
	// was just taken
	// TvW: what's this then?
/*	
	if (*newref == AO_REFIM_NONE) { // new reference is valid
		printf("old correction factor: %f\n",corr);
		refaver = 0.0;  // average reference subaperture intensity
		for (ix=0;ix<SX;ix++)
			for (iy=0;iy<SY;iy++)
				refaver = refaver + (float) refim[(iy+4)*RX+ix+4];
		
		refaver = refaver/(float) (SX*SY);
		corr = 1.0; // reset brightness correction
		*newref = AO_REFIM_GOOD;
		printf("new reference, correction factor: %f\n",corr);
	}
*/

	// calculate intensity correction factor based on average intensity
/*	if (refaver == 0.0)
		corr = 1.0;
	else {
		if (*aver>0) corr = corr * refaver / *aver; // update correction factor
		// if correction factor is 2 or larger, then the multiplication
		// with 2^15 above 'flips' over to something close to zero; we should
		// allow at least a factor of 4, but we still need to catch that case
		// (maybe using saturated multiplication)
		if (corr>1.999) corr=1.999;
	}
	*/
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

void procRefim(float *image, float *ref, float *sharp, float *aver) {
	
}

int modParseSH(int wfs) {
	// now calculate the offsets 
	float aver=0.0, max=0.0;
	float coords[ptc.wfs[wfs].nsubap][2];
	int i;
	
	// track the maxima
	cogTrack(wfs, &aver, &max, coords);
	
	// note: coords is relative to the center of the tracker window
	// therefore we can simply update the lower left coord by subtracting the coordinates.
	
	logInfo("Coords: ");
	for (i=0; i<ptc.wfs[wfs].nsubap; i++) {
		ptc.wfs[wfs].subc[i][0] -= coords[i][0];//-ptc.wfs[wfs].res[0]/ptc.wfs[wfs].cells[0]/4;
		ptc.wfs[wfs].subc[i][1] -= coords[i][1];//-ptc.wfs[wfs].res[1]/ptc.wfs[wfs].cells[1]/4;
		logDirect("(%d, %d) ", ptc.wfs[wfs].subc[i][0]+ptc.wfs[wfs].res[0]/ptc.wfs[wfs].cells[0]/4, \
			ptc.wfs[wfs].subc[i][1]+ptc.wfs[wfs].res[1]/ptc.wfs[wfs].cells[1]/4);
	}
	logDirect("\n");
	
	return EXIT_SUCCESS;
}

// TODO: on the one hand depends on int wfs, on the other hand needs int window[2], fix that
void imcal(float *corrim, float *image, float *darkim, float *flatim, int wfs, float *sum, float *max, int window[2]) {
	// substract the dark, multiply with the flat (right?)
	// TODO: dark and flat currently ignored, fix that
	int i,j;
	
	// corrim comes from:
	// cp = &corr[sn*shsize[0]*shsize[1]]; // calibrated image (output)
	// image comes form:
	// ip = &image[subc[sn][0]*res[0]+subc[sn][1]]; // raw image (input)
		
	// we do shsize/2 because the tracker windows are only a quarter of the
	// total tracker area
	for (i=0; i<window[1]; i++) {
		for (j=0; j<window[0]; j++) {
			// We correct the image stored in 'image' by applying some dark and flat stuff,
			// and copy it to 'corrim' which has a different format than image.
			// Image is a row-major array of res[0] * res[1], where subapertures are hard to
			// read out (i.e. you have to skip pixels and stuff), while corrim is reformatted
			// in such a way that the first shsize[0]*shsize[1] pixels are exactly one subapt,
			// and each consecutive set is again one seperate subapt. This way you can loop
			// through subapts with only one counter (right?)

			corrim[i*window[0] + j] = image[i*ptc.wfs[wfs].res[0] + j];			
			*sum += corrim[i*window[0] + j];
			if (corrim[i*window[0] + j] > *max) *max = corrim[i*window[0] + j];
		}
	}			
	
}

int drawSubapts(int wfs, SDL_Surface *screen) {
	if (ptc.wfs[wfs].nsubap == 0)
		return EXIT_SUCCESS;	// if there's nothing to draw, don't draw (shouldn't happen)
		
	int shsize[2];			// size of one subapt (in pixels)
	int res[2], cells[2];	// size of whole image, nr of cells
	int (*subc)[2] = ptc.wfs[wfs].subc;	// TODO: does this work?

	res[0] = ptc.wfs[wfs].res[0];	// shortcuts
	res[1] = ptc.wfs[wfs].res[1];
	cells[0] = ptc.wfs[wfs].cells[0];
	cells[1] = ptc.wfs[wfs].cells[1];	
	shsize[0] = res[0]/cells[0]; 		// size of subapt cell in x
	shsize[1] = res[1]/cells[1];		// size of subapt cell in y
	
	int sn=0;
	Slock(screen);
	
	// this is the size for the tracker rectangles
	int subsize[2] = {shsize[0]/2, shsize[1]/2};
	
	// we draw the reference subaperture rectangle bigger than the rest, with lower left coord:
	int refcoord[] = {subc[0][0]-shsize[0]/4, subc[0][1]-shsize[1]/4};
	drawRect(refcoord, shsize, screen);
	
	for (sn=1; sn<ptc.wfs[wfs].nsubap; sn++) {
		// subapt with lower coordinates (subc[sn][0],subc[sn][1])
		// first subapt has size (shsize[0],shsize[1]),
		// the rest are (shsize[0]/2,shsize[1]/2)
		drawRect(subc[sn], subsize, screen);
	}
	
	Sulock(screen);
	SDL_Flip(screen);	
	return EXIT_SUCCESS;
}
