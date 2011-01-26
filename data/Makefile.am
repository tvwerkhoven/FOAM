# Install the relevant configuration files to a subdir of the datadir
foamdatadir = $(FOAMDATADIR)
dist_foamdata_DATA = simstatic*.pgm wavefront.fits

# Make a crop from wavefront.fits. Don't include this in the rep. because
# it's fairly large. Do crop the wavefront to get an asymmetric image which
# is better for testing.

# Loop over all pgms, convert only if fits does not exist.
# @todo is this portable?
all-local:
	@echo -n Cropping wavefront...
	@convert wavefront.fits wavefront.png
	@convert wavefront.png -crop 1024x768+0+128 wavefront-crop.png
	@convert wavefront-crop.png wavefront-crop.fits
	@echo "done."
	
	@echo Converting datafiles...
	@for f in *pgm; \
		do \
		if [ ! -f $$f.fits ]; then echo "Converting $$f..."; convert $$f $$f.fits; fi; \
	done;
	@echo "done."



