# Install the relevant configuration files to a subdir of the datadir
foamdatadir = $(FOAMDATADIR)
dist_foamdata_DATA =

### STATIC SIMULATION MODE ###
##############################
if SIMSTAT

dist_foamdata_DATA += simstatic*.pgm simstatic*.fits

endif SIMSTAT
### END STATIC SIMULATION MODE ###

### DYNAMICAL SIMULATION MODE ###
#################################
# if SIMDYN
# 
# dist_foamconf_DATA += apert15-256.png apert15-256.pgm dm37-256.png dm37-256.pgm wavefront.png
# 
# endif SIMDYN
### END DYNAMICAL SIMULATION MODE ###

### MCMATH MODE ### 
###################
# if MCMATH
# 
# # Camera configuration for McMath
# dist_foamconf_DATA += dalsa-cad6-pcd.cam
# 
# endif MCMATH
### END MCMATH MODE ### 
