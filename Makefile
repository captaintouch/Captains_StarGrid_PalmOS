# Top-level build dispatcher.
#
# Each platform has its own makefile; this one just delegates to them:
#   Makefile.palm  -> Palm OS .prc build   (needs the Palm SDK toolchain)
#   Makefile.cli   -> host command-line build (needs gcc)
#   Makefile.web   -> WebAssembly build (needs emcc)
#
# Run `make` with no target to see the list of available builds.

.DEFAULT_GOAL := help

help:
	@echo "Captain's StarGrid - build targets:"
	@echo "  make cli           Build the host command-line binary (./stargrid_cli)"
	@echo "  make web           Build the WebAssembly bundle (stargrid_web.js/.wasm)"
	@echo "  make palm          Build all Palm OS .prc variants (lowres, hires, debug)"
	@echo "  make palm-lowres   Build the low-res Palm .prc"
	@echo "  make palm-hires    Build the hi-res Palm .prc"
	@echo "  make palm-debug    Build the debug Palm .prc"
	@echo "  make all           Build every platform"
	@echo "  make clean         Remove build artifacts for all platforms"

cli:
	$(MAKE) -f Makefile.cli

web:
	$(MAKE) -f Makefile.web

palm:
	$(MAKE) -f Makefile.palm all

palm-lowres:
	$(MAKE) -f Makefile.palm lowres

palm-hires:
	$(MAKE) -f Makefile.palm hires

palm-debug:
	$(MAKE) -f Makefile.palm debug

all: cli web palm

clean:
	$(MAKE) -f Makefile.cli clean
	$(MAKE) -f Makefile.web clean
	$(MAKE) -f Makefile.palm cleanup

.PHONY: help cli web palm palm-lowres palm-hires palm-debug all clean
