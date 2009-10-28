# Include these files in the dist, but don't install them
EXTRA_DIST = *.png *.cfg *.cam *.pgm

# Install the relevant configuration files to a subdir of the datadir
foamconfdir = $(FOAMCONFDIR)
dist_foamconf_DATA = 

### STATIC SIMULATION MODE ###
##############################
if SIMSTAT

dist_foamconf_DATA += simstatic-irr.pgm 

endif SIMSTAT
### END STATIC SIMULATION MODE ###

### DYNAMICAL SIMULATION MODE ###
#################################
if SIMDYN

dist_foamconf_DATA += apert15-256.png apert15-256.pgm dm37-256.png dm37-256.pgm wavefront.png

endif SIMDYN
### END DYNAMICAL SIMULATION MODE ###

### MCMATH MODE ### 
###################
if MCMATH

# Camera configuration for McMath
dist_foamconf_DATA += dalsa-cad6-pcd.cam

endif MCMATH
### END MCMATH MODE ### 
