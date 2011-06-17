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

# Install man pages using Automake directives
man3_MANS = man/man3/*.3

install-html-local:
	@echo -n Installing documentation to $(htmldir)...
	@mkdir -p $(htmldir)
	@cp -r html $(htmldir)
	@echo "done."
	
endif # HAVE_DOCS

clean-local:
	@echo -n Cleaning up documentatation...
	@rm -r html/
	@rm -r latex/
	@echo "done."

# Always remove documentation: maybe the user re-configured after installing 
# which would leave the documentation installed.
uninstall-local:
	@echo -n Uninstalling documentatation from $(htmldir)...
	@rm -r $(htmldir)
	@echo "done."
