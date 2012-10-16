#!/usr/bin/env python2.7
# -*- coding: utf-8 -*-

"""
@file 20121013_foam_inspect_calib.py -- Inspect calibration data
@author Tim van Werkhoven
@date 20121013
@copyright Copyright (c) 2012 Tim van Werkhoven <timvanwerkhoven@gmail.com>
"""

import numpy as np
import os
import pylab as plt
from fnmatch import filter

# Compare with previously acquired data
datadir = './FOAM_data_20121012_130907/'

# Find calibration files
calibfiles = os.listdir(datadir)

# Find influence matrix, get number of modes and data from this
# like: dev.wfs.shwfs.ixonwfs_alpao_dm97_infmat_80_50.csv
infmatf = filter(calibfiles, "*_infmat_*.csv")[0][:-4]
devname = infmatf.split("_infmat_")[0].split('.')[-1]

# Get the geometry from the last part of the filename
geometrys = infmatf.split("_infmat_")[1]
nmeas, nmodes = (int(g) for g in geometrys.split("_"))

def show_calib(filepath, title, devname, shape, dir='./'):
	"""Load data stored in **filepath**, and plot for user"""
	
	# load data, reshape
	thiscal = np.loadtxt(os.path.join(dir, filepath)).reshape(*shape)
	
	plot_calib(thiscal, title, devname)
		
	return thiscal

def plot_calib(thiscal, title, devname):
	# Plot 2D
	if (len(thiscal.shape) == 2):
		plt.figure(0)
		plt.clf()
		plt.title(title + " [" + devname + "]")
		plt.imshow(thiscal)
		plt.colorbar()
		plt.savefig(title+devname)
	elif (len(thiscal.shape) == 1):
		plt.figure(0)
		plt.clf()
		plt.title(title +  " [" + devname + "]")
		plt.plot(thiscal)
		plt.savefig(title+devname)
	

infmat = show_calib(filter(calibfiles, "*_infmat_*.csv")[0], "Influence matrix", devname, (nmeas, nmodes), dir=datadir)

pident = show_calib(filter(calibfiles, "*_pseudo-ident_*.csv")[0], "Pseudo ident", devname, (nmodes, nmodes), dir=datadir)

svd_U = show_calib(filter(calibfiles, "*_U_*.csv")[0], "U", devname, (nmeas, nmodes), dir=datadir)

svd_V = show_calib(filter(calibfiles, "*_V_*.csv")[0], "V", devname, (nmodes, nmodes), dir=datadir)

svd_sing = show_calib(filter(calibfiles, "*_singval_*.csv")[0], "Singular values", devname, (nmodes,), dir=datadir)

svd_Sigma = np.diag(1.0/svd_sing)

# Calculate full actuation matrix
actmat_full = np.dot(svd_V.T, np.dot(svd_Sigma, svd_U.T))

plot_calib(actmat_full, "Actuation matrix", devname)

############################################################################
# Check tip-tilt (vector of 1,0,1,0,1,0 ... times actmat)
############################################################################

tipvec = np.zeros(nmeas).reshape(-1,1)
tipvec[::2] = 1

tiltvec = np.zeros(nmeas).reshape(-1,1)
tiltvec[1::2] = 1

tipact = np.dot(actmat_full, tipvec)
tiltact = np.dot(actmat_full, tiltvec)

plt.figure(1)
plt.clf()
plt.title('Tip/tilt actuation')
plt.plot(tipact)
plt.plot(tiltact)
plt.xlabel("Mode [#]")
plt.ylabel("Amplitude [AU]")
plt.savefig("calib_tip_tilt_actuation.pdf")
