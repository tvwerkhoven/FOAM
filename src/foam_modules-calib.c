/*! 
	@file foam_modules-calib.c
	@author @authortim
	@date 2008-02-07

	@brief This file is used to calibrate actuators like the DM and TT mirrors used in AO systems.
	
*/

// HEADERS //
/***********/

#define NS 256 // maximum number of subapts (TODO: do we need this?)
#define DM_MAXVOLT 1	// these are normalized and converted to the right values by drvSetActuator()
#define DM_MIDVOLT 0
#define DM_MINVOLT -1

// TODO: document
// TODO: what arguments do we *at least* need?
// TODO: only works with ONE wfs (wfs 0)
int modCalDM(control_t *ptc, int nact, int nsubapt, float *ctrl) {
	int j, i, k, skip;
	float q0x[nsubapt], q0oy[nsubapt];
	float origvol;
	int measurecount=10;
	int skipframes=5;
	
	for (j=0; j<nact; j++) { /* loop over all actuators */
		
		for (i=0; i<nsubapt; i++) { // set averaging buffer to zero
			q0x[i] = 0.0;
			q0y[i] = 0.0;
		}
	
		logInfo("Measuring actuator %d",j);
		
		for (k=0; k < measurecount; k++) {
			ctrl[j] = DM_MAXVOLT;
			// run the open loop *once*, which will approx do the following:
			// read sensor, apply some SH WF sensing, set actuators, 
			// TODO: open or closed loop?
			for (skip=0; skip<skipframes; skip++) // skip some frames here
				modClosedLoop(*ptc);
			
			// take the shifts and store those
			// SPEED: we could divide after the loop, slightly faster
    		for (i=0;i<NS;i++) { 
				q0x[i] += ptc->wfs[0].subc[i][0] / (float) IFMN;
				q0y[i] += ptc->wfs[0].subc[i][1] / (float) IFMN;
			}
			
			ctrl[j] = DM_MINVOLT;
			for (skip=0; skip<skipframes; skip++) // skip some frames here
				modClosedLoop(*ptc);
			
			// take the shifts and store those
			// SPEED: we could divide after the loop, slightly faster
    		for (i=0;i<NS;i++) { 
				q0x[i] -= ptc->wfs[0].subc[i][0] / (float) IFMN;
				q0y[i] -= ptc->wfs[0].subc[i][1] / (float) IFMN;
			}

		} // end measurecount loop
	} // end loop over actuators
	
}

for (j=1;j<=DM_ACTUATORS+2;j++) { /* loop over all actuators */
  for (i=0;i<NS;i++) { // set averaging buffer to zero
    q0x[i] = 0.0;
    q0y[i] = 0.0;
  }
  printf("measuring actuator %d\n",j);
  if (j<=DM_ACTUATORS)
        origvol=ptc->vol[j]; /* temporarily store current voltage */
  else
    origvol=128;
  for (k=0;k<IFMN;k++) { // switch back and forth a few times
    /* set actuator voltage to high voltage */
    ptc->actnum = j;
    ptc->actvol = DM_MAXVOLT;
    ptc->iskip = 5; /* skip images before taking measurement */
    ptc->mode = AO_MODE_DMIF;
    while (ptc->mode == AO_MODE_DMIF) usleep(1000); //TvW: is this necessary?
    for (i=0;i<NS;i++) { // add shifts to average
      q0x[i] = q0x[i] + ptc->qx[i] / (float) IFMN;
      q0y[i] = q0y[i] + ptc->qy[i] / (float) IFMN;
    }
    if (j>37)
      if (write(cfd,ptc->image,SX*SY*NS*4) != SX*SY*NS*4)
	printf("unable to write image file\n");
    
    /* set actuator voltage to low voltage */
    ptc->actnum = j;
    ptc->actvol = DM_MINVOLT;
    ptc->iskip = 5; /* skip images before taking measurement */
    ptc->mode = AO_MODE_DMIF;
    while (ptc->mode == AO_MODE_DMIF) usleep(1000);
    for (i=0;i<NS;i++) { // subtract shifts from average
      q0x[i] = q0x[i] - ptc->qx[i] / (float) IFMN;
      q0y[i] = q0y[i] - ptc->qy[i] / (float) IFMN;
    }
    if (j>37)
      if (write(cfd,ptc->image,SX*SY*NS*4) != SX*SY*NS*4)
	printf("unable to write image file\n");

  } // end of switching back and forth loop
  for (i=0;i<NS;i++) { // print info to file
    fprintf(fp,"%.12g\n%.12g\n",(double) q0x[i], (double) q0y[i]);
  }

  /* reset actuator voltage to original value */
  ptc->actnum = j;
  ptc->actvol = origvol;
      ptc->iskip = 0; /* no waiting here */
  ptc->mode = AO_MODE_DMIF;
  while (ptc->mode == AO_MODE_DMIF) usleep(1000);
} /* end of loop over all actuators */
close(cfd);
fclose(fp);
printf("DM influence function saved in file ao_dmif0.txt\n");
  }