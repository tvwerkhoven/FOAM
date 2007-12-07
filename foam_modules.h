/*! 
	@file foam_modules.h
	@author Tim van Werkhoven
	@date November 30 2007

	@brief This header file prototypes driver and module functions.
*/
#ifndef FOAM_MODULES
#define FOAM_MODULES

/*!
@brief Reads out the sensor(s) and outputs to ptc.wfs[n].image.
*/
int drvReadSensor();

/*!
@brief This sets the various actuators (WFCs).
*/
int drvSetActuator();

/*!
@brief Parses output from Shack-Hartmann WFSs.
*/
int modParseSH();

/*!
@brief Calculates DM output voltages.
*/
int modCalcDMVolt();

int simDM(char *boundarymask, char *actuatorpat, int nact, float *ctrl, float *image, int niter);
int simObj(char *file, float *image);
int simAtm(char *file, int resx, int resy, float *image);
int simTel(char *file, int resx, int resy, float *image);
int simWFC(int wfcid, int nact, float *ctrl, float *image);


#endif /* FOAM_MODULES */