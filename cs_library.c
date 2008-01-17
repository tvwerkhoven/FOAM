/* 
	Library file for the Control Software
	Tim van Werkhoven, November 13 2007
*/

#include "cs_library.h"

control_t ptc = { //!< Global struct to hold system characteristics and other data. Initialize with complete but minimal configuration
	.mode = AO_MODE_OPEN,
	.wfs_count = 0,
	.wfc_count = 0,
	.frames = 0
};

config_t cs_config = { 
	.listenip = "0.0.0.0",
	.listenport = 10000,
	.infofd = NULL,
//	.infofile = "",
	.errfd = NULL,
//	.errfile = "",
	.debugfd = NULL,
//	.debugfile = "",
	.use_syslog = false,
	.syslog_prepend = "foam",
	.use_stderr = true,
	.loglevel = LOGINFO
};

conntrack_t clientlist;

static int formatLog(char *output, const char *prepend, const char *msg) {
	char timestr[9];
	time_t curtime;
	struct tm *loctime;

	curtime = time (NULL);
	loctime = localtime (&curtime);
	strftime (timestr, 9, "%H:%M:%S", loctime);

	output[0] = '\0'; // reset string
	
	strcat(output,timestr);
	strcat(output,prepend);
	strcat(output,msg);
	strcat(output,"\n");
	
	return EXIT_SUCCESS;
}

/* logging functions */
void logInfo(const char *msg, ...) {
	if (cs_config.loglevel < LOGINFO) 		// Do we need this loglevel?
		return;
	
	va_list ap, aq, ar; // We need three of these because we cannot re-use a va_list variable
	
	va_start(ap, msg);
	va_copy(aq, ap);
	va_copy(ar, ap);
	
	formatLog(logmessage, " <info>: ", msg);
	
	if (cs_config.infofd != NULL) // Do we want to log this to a file?
		vfprintf(cs_config.infofd, logmessage , ap);
	
	if (cs_config.use_stderr == true) // Do we want to log this to syslog
		vfprintf(stderr, logmessage, aq);
		
	if (cs_config.use_syslog == true) 	// Do we want to log this to syslog?
		syslog(LOG_INFO, msg, ar);
	
	va_end(ap);
	va_end(aq);
	va_end(ar);
}

void logErr(const char *msg, ...) {
	if (cs_config.loglevel < LOGERR) 	// Do we need this loglevel?
		return;

	va_list ap, aq, ar;
	
	va_start(ap, msg);
	va_copy(aq, ap);
	va_copy(ar, ap);
	
	formatLog(logmessage, " <error>: ", msg);

	
	if (cs_config.errfd != NULL)	// Do we want to log this to a file?
		vfprintf(cs_config.errfd, logmessage, ap);

	if (cs_config.use_stderr == true) // Do we want to log this to syslog?
		vfprintf(stderr, logmessage, aq);
	
	if (cs_config.use_syslog == true) // Do we want to log this to syslog?
		syslog(LOG_ERR, msg, ar);

	va_end(ap);
	va_end(aq);
	va_end(ar);
}

void logDebug(const char *msg, ...) {
	if (cs_config.loglevel < LOGDEBUG) 	// Do we need this loglevel?
		return;
		
	va_list ap, aq, ar;
	
	va_start(ap, msg);
	va_copy(aq, ap);
	va_copy(ar, ap);

	formatLog(logmessage, " <debug>: ", msg);
	
	if (cs_config.debugfd != NULL) 	// Do we want to log this to a file?
		vfprintf(cs_config.debugfd, logmessage, ap);
	
	if (cs_config.use_stderr == true)	// Do we want to log this to stderr?
		vfprintf(stderr, logmessage, aq);

	
	if (cs_config.use_syslog == true) 	// Do we want to log this to syslog?
		syslog(LOG_DEBUG, msg, ar);

	va_end(ap);
	va_end(aq);
	va_end(ar);
}

void selectSubapts(float *image, float samini, int samxr, int wfs) {
	// stolen from ao3.c by CUK
	int isy, isx, iy, ix, i, sn, nsubap;
	float sum, fi;			// check 'intensity' of a subapt
	float csum, cs[2]; 		// for center of gravity
	int shsize[2];			// size of one subapt (in pixels)
	int res[2], cells[2];	// size of whole image, nr of cells
	int cx, cy;				// for CoG
	float dist, rmin;		// minimum distance
	int csa;				// estimate for best subapt
	int (*subc)[2] = ptc.wfs[wfs].subc;	// TODO: does this work?

	res[0] = ptc.wfs[wfs].res[0];	// shortcuts
	res[1] = ptc.wfs[wfs].res[1];
	cells[0] = ptc.wfs[wfs].cells[0];
	cells[1] = ptc.wfs[wfs].cells[1];
	
	// TODO: what's this? why do we use this?
	int apmap[cells[0]][cells[1]];		// aperture map
	int apmap2[cells[0]][cells[1]];		// aperture map 2
	int apcoo[cells[0] * cells[1]][2];  // subaperture coordinates in apmap
	
	shsize[0] = res[0]/cells[0]; 		// size of subapt cell in x
	shsize[1] = res[1]/cells[1];		// size of subapt cell in y
	
	for (isy=0; isy<cells[1]; isy++) { // loops over all potential subapertures
		for (isx=0; isx<cells[0]; isx++) {
			// check one potential subapt
			
			for (iy=0; iy<shsize[1]; iy++) { // sum all pixels in the subapt
				for (ix=0; ix<shsize[0]; ix++) {
					fi = (float) image[isy*shsize[1]*res[0] + isx*shsize[0] + ix+ iy*res[0]];
					sum += fi;
					/* for center of gravity, only pixels above the threshold are used;
					otherwise the position estimate always gets pulled to the center;
					good background elimination is crucial for this to work !!! */
					fi -= samini;    		// subtract threshold
					if (fi<0.0) fi=0.0;		// clip
					csum = csum + fi;		// add this pixel's intensity to sum
					cs[0] = cs[0] + fi * ix;	// center of gravity of subaperture intensity 
					cs[1] = cs[1] + fi * iy;
				}
			}
			
			// check if the summed subapt intensity is above zero (e.g. do we use it?)
			if (csum > 0.0) { // good as long as pixels above background exist
				subc[sn][0] = isx*shsize[0]+4 + (int) (cs[0]/csum) - shsize[0]/2;	// subapt coordinates
				subc[sn][1] = isy*shsize[1]+4 + (int) (cs[1]/csum) - shsize[1]/2;	// TODO: how does this work? 
																				// why 4? (partial subapt?)
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
	nsubap = sn; // nsubap: global variable that contains number of subapertures
	cx = cx / (float) sn; // TODO what does this do? why?
	cy = cy / (float) sn;

	// determine central aperture that will be reference
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
	cs[0] = 0.0; cs[1] = 0.0; csum = 0.0;
	for (iy=0; iy<shsize[1]; iy++) {
		for (ix=0; ix<shsize[0]; ix++) {
			fi = (float) image[(subc[0][1]-4+iy)*res[0]+subc[0][0]-4+ix]; // TODO: fix the static '4'
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
	
	printf("old subx=%d, old suby=%d\n",subc[0][0],subc[0][1]);
	subc[0][0] += (int) (cs[0]/csum+0.5) - shsize[0]/2;
	subc[0][1] += (int) (cs[1]/csum+0.5) - shsize[1]/2;
	printf("new subx=%d, new suby=%d\n",subc[0][0],subc[0][1]);

	// enforce maximum radial distance from center of gravity of all
	// potential subapertures if samxr is positive
	if (samxr > 0) {
		sn = 1;
		while (sn < nsubap) {
			if (sqrt((subc[sn][0]-cx)*(subc[sn][0]-cx) + \
				(subc[sn][1]-cy)*(subc[sn][1]-cy)) > samxr) { // TODO: why remove subapts?
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
	logInfo("Selected %d usable subapertures",nsubap);
	
	// set remaining subaperture coordinates to 0
	for (sn=nsubap; sn<cells[0]*cells[1]; sn++) {
		subc[sn][0] = 0; 
		subc[sn][1] = 0;
	}

	int cfd;
	char *cfn;

	// write subaperture image into file
	cfn = "ao_saim.dat";
	cfd = creat(cfn,0);
	if (write(cfd, image, res[0]*res[1]) != res[0]*res[1])
		logErr("Unable to write subapt img to file");

	close(cfd);
	logInfo("Subaperture definition image saved in file %s",cfn);
}