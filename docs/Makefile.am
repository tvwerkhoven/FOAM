# Compile documentation for FOAM if requested. Move to $(htmldir)

if HAVE_DOCS
if HAVE_EXTDOCS
all-local:
	@echo -n Building extended documentation...
	@doxygen doxyfile-debug
	@echo "done."

else
all-local:
	@echo -n Building documentation...
	@doxygen doxyfile
	@echo "done."

endif # HAVE_EXTDOCS

install-data-local:
	@echo -n Installing html documentation to $(htmldir) ...
	@mkdir -p $(htmldir)
	@cp -r html $(htmldir)
	@echo "done."
#	@echo -n Installing man pages to $(mandir) ...
#	@mkdir -p $(mandir)/man3
#	@cp -r man/man3/*3 $(mandir)/man3
#	@echo "done."

# Use automake directive to install / uninstall man pages
# @todo Problem: man3_MANS executed *before* -local targets, and thus 
# man/man3/* files do not exist yet. This hack fixes that by making sure one 
# file (that will be generated anyway) already exists.
man/man3/*.3:
	@mkdir -p man/man3/
	@touch man/man3/FOAM.3

man3_MANS = man/man3/*.3

endif # HAVE_DOCS

clean-local:
	@echo -n Cleaning up documentatation...
	@if [ -d man ]; then rm -r man; fi 
	@if [ -d html ]; then rm -r html; fi 
	@if [ -d latex ]; then rm -r latex; fi 
	@echo "done."

# Always remove documentation: maybe the user re-configured after installing 
# which would leave the documentation installed.
uninstall-local:
	@echo -n Uninstalling documentatation from $(htmldir)...
	@rm -r $(htmldir)
	@echo "done."
