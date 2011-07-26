# Install the relevant configuration files to a subdir of the datadir
foamdatadir = $(FOAMDATADIR)/simdata

# Include these files in th distribution (the rest depend on them)
dist_foamdata_DATA = simstatic*.pgm wavefront.fits simulwfc.csv

# These files are generated from the above files and do not need to be included
foamdata_DATA = simstatic*.fits wavefront*

# Make a crop from wavefront.fits. Don't include this in the rep. because
# it's fairly large. Do crop the wavefront to get an asymmetric image which
# is better for testing.

# Loop over all pgms, convert only if fits does not exist.
install-data-local:
	@echo -n Cropping wavefront...
	@if [ ! -f wavefront.png ]; then convert wavefront.fits wavefront.png; fi
	@if [ ! -f wavefront-crop.fits ]; then convert wavefront.fits -depth 8 -crop 1024x768+0+128 wavefront-crop.fits; fi
	@if [ ! -f wavefront-crop.png ]; then convert wavefront-crop.fits wavefront-crop.png; fi
	@echo "done."
	
	@echo Converting datafiles...
	@for f in *pgm; \
		do \
		if [ ! -f $$f.fits ]; then echo "Converting $$f..."; convert $$f $$f.fits; fi; \
	done;
	@echo "done."
