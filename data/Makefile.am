# Install the relevant configuration files to a subdir of the datadir
foamdatadir = $(FOAMDATADIR)
dist_foamdata_DATA =

### STATIC SIMULATION MODE ###
##############################
if SIMSTAT

dist_foamdata_DATA += simstatic*.pgm #simstatic*.fits

# Loop over all pgms, convert only if fits does not exist.
# @todo is this portable?
all-local:
	@echo Converting datafiles...
	@for f in *pgm; \
		do \
		if [ ! -f $$f.fits ]; then echo "Converting $$f..."; convert $$f $$f.fits; fi; \
	done;
	@echo "done."

endif SIMSTAT
### END STATIC SIMULATION MODE ###
