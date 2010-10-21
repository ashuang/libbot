# Default makefile distributed with pods version: 10.10.08

default_target: all

# Default to a less-verbose build.  If you want all the gory compiler output,
# run "make VERBOSE=1"
$(VERBOSE).SILENT:

# Figure out where to build the software.
#   Use BUILD_PREFIX if it was passed in.
#   If not, search up to four parent directories for a 'build' directory.
#   Otherwise, use ./build.
ifeq "$(BUILD_PREFIX)" ""
BUILD_PREFIX=$(shell for pfx in .. ../.. ../../.. ../../../..; do d=`pwd`/$$pfx/build; \
               if [ -d $$d ]; then echo $$d; exit 0; fi; done; echo `pwd`/build)
endif

# Default to a release build.  If you want to enable debugging flags, run
# "make BUILD_TYPE=Debug"
ifeq "$(BUILD_TYPE)" ""
BUILD_TYPE="Release"
endif

all: pod-build/Makefile
	$(MAKE) -C pod-build all install

pod-build/Makefile:
	$(MAKE) configure

.PHONY: configure
configure:
	@echo "\nBUILD_PREFIX: $(BUILD_PREFIX)\n\n"

	# create the build directories if necessary
	@[ -d $(BUILD_PREFIX) ] || mkdir -p $(BUILD_PREFIX) || exit 1
	@[ -d pod-build ] || mkdir pod-build || exit 1

	@echo "$(BUILD_PREFIX)" > pod-build/build_prefix

	# run CMake to generate and configure the build scripts
	@cd pod-build && cmake -DCMAKE_INSTALL_PREFIX=$(BUILD_PREFIX) \
	                       -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) ..

clean:
	if [ -d pod-build ]; then make -C pod-build clean; fi
	rm -rf pod-build
	rm -rf build #remove the local build directory if it exists

uninstall:
	@echo removing...
	@cat pod-build/install_manifest.txt
	@rm -f `cat pod-build/install_manifest.txt`
