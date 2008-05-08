/*! 
 @file foam_modules-okodm.h
 @author @authortim
 @date 2008-03-20 10:07
 
 @brief This file contains prototypes for routines to drive a 37 actuator Okotech DM using PCI interface
 
 \section Info
 
 The Okotech 37ch DM has 38 actuators (one being the substrate) leaving 37 for AO. The mirror
 is controlled through a PCI board. This requires setting some hardware addresses, but not much
 more. See mirror.h and rotate.c supplied on CD with the Okotech mirror for examples.
 
 Manufacturers website:
 <tt>http://www.okotech.com/content/oko/pics/gallery/Typical%20PDM%2037%20passport_.pdf</tt>
 
 This module also compiles on its own like:\n
 <tt>gcc foam_modules-okodm.c -lm -lc -lgslcblas -lgsl -Wall -DFOAM_MODOKODM_ALONE=1 -std=c99</tt>
 
 \section Functions
 
 \li drvInitOkoDM() - Initialize the Okotech DM (call this first!)
 \li drvSetOkoDM() - Sets the Okotech 37ch DM to a certain voltage set.
 \li drvRstOkoDM() - Resets the Okotech DM to FOAM_MODOKODM_RSTVOLT
 \li drvCloseOkoDM() - Calls drvRstOkoDM, then closes the Okotech DM (call this at the end!)
 
 \section Configuration
 
 There are several things that can be configured about this module. The following defines are used:
 \li \b FOAM_MODOKODM_MAXVOLT (255), the maximum voltage allowed (all voltages are logically AND'd with this value)
 \li \b FOAM_MODOKODM_ALONE (*undef*), ifdef, this module will compile on it's own, imlies FOAM_DEBUG
 \li \b FOAM_DEBUG (*undef*), ifdef, this module will give lowlevel debugs through printf
 
 \section Dependencies
 
 This module depends on GSL because it uses the gsl_vector* datatypes to store DM commands in. 
 This is done because this this format is suitable for doing fast calculations, and
 the control vector is usually the output of some matrix multiplication.
 
 \section History
 
 \li 2008-04-14: api update, defines deprecated, replaced by struct
 \li 2008-04-02: init
 
 */

#ifndef FOAM_MODULES_OKODM
#define FOAM_MODULES_OKODM

// HEADERS //
/***********/

#include <gsl/gsl_vector.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <math.h>

// DEFINES //
/***********/

#define FOAM_MODOKODM_MAXVOLT 255		//!< Maximum voltage to set to the PCI card

#ifdef FOAM_MODOKODM_ALONE
#define FOAM_DEBUG 1			//!< set to 1 for debugging, in that case this module compiles on its own
#endif

// DATATYPES //
/*************/

/*!
 @brief Metadata on DM operations, typically an Okotech DM using a PCI board
 
 This struct holds metadata on DM operations. The fields denoted '(user)' must
 be supplied in advance by the user, while '(mod)' will be filled in during
 operation.
 
 Note: To set maxvolt above 255, modify the FOAM_MODOKODM_MAXVOLT define. This define
 overrides maxvolt if it's greater than 255 for your safety. 
 */
typedef struct {
	int minvolt;		//!< (user) minimum voltage that can be applied to the mirror
	int midvolt;		//!< (user) mid voltage (i.e. 'flat')
	int maxvolt;		//!< (user) maximum voltage. note: this value is overridden by FOAM_MODOKODM_MAXVOLT if greater than 255!
	int nchan;			//!< (user) number of channels, including substrate (i.e. 38)
	int *addr;			//!< (mod) pointer to array storing hardware addresses
	int fd;				//!< (mod) fd used with a mirror (to open the port)
	char *port;			//!< (user) port to use (i.e. "/dev/port")
	int pcioffset;		//!< (user) pci offset to use (4 on 32 bit systems)
	int pcibase[4];		//!< (user) max 4 PCI base addresses
} mod_okodm_t;

// PUBLIC PROTOTYPES //
/*********************/

/*!
 @brief This function sets the DM to the values given by *ctrl to the DM at *dm
 
 The vector/array *ctrl should hold control values for the mirror which
 range from -1 to 1, the domain being linear (i.e. from 0.5 to 1 gives twice
 the stroke). Since the mirror is actually only linear in voltage^2, this
 routine maps [-1,1] to [0,255] appropriately.
 
 @param [in] *ctrl Holds the controls to send to the mirror in the -1 to 1 range
 @param [in] *dm DM configuration information, filled by drvInitOkoDM()
 @return EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
 */
int drvSetOkoDM(gsl_vector_float *ctrl, mod_okodm_t *dm);

/*!
 @brief Resets all actuators on the DM to dm->minvolt
 
 @param [in] *dm DM configuration information, filled by drvInitOkoDM()
 @return EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
 */
int drvRstOkoDM(mod_okodm_t *dm);

/*!
 @brief Sets all actuators on the substrate to a certain voltage.
 
 @param [in] volt The voltage to set on all actuators
 @param [in] *dm DM configuration information, filled by drvInitOkoDM()
 @return EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
 */
int drvSetAllOkoDM(mod_okodm_t *dm, int volt);

/*!
 @brief Initialize the module (software and hardware)
 
 You need to call this function *before* any other function, otherwise it will
 not work. The struct pointed to by *dm must hold some information to 
 init the DM with, see mod_okodm_t.
 
 @param [in] *dm DM configuration information
 @return EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
 */
int drvInitOkoDM(mod_okodm_t *dm);

/*!
 @brief Close the module (software and hardware)
 
 You need to call this function *after* the last DM command. This
 resets the mirror and closes the file descriptor.
 
 @param [in] *dm DM configuration information, filled by drvInitOkoDM()
 @return EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
 */
int drvCloseOkoDM(mod_okodm_t *dm);

#endif //#ifndef FOAM_MODULES_OKODM
