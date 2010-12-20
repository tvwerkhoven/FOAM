# Install the relevant configuration files to a subdir of the datadir
foamdatadir = $(FOAMDATADIR)
dist_foamdata_DATA =

### STATIC SIMULATION MODE ###
##############################
if SIMSTAT

dist_foamdata_DATA += simstatic*.pgm #simstatic*.fits

all-local:
	@echo -n "Converting datafiles..."
	@for f in *pgm; \
		do convert $$f $$f.fits; \
	done;
	@echo "done."

endif SIMSTAT
### END STATIC SIMULATION MODE ###
