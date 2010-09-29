default_target: all

$(VERBOSE).SILENT:

# Figure out where to build the pod. Use BUILD_PREFIX if it was passed in.
# Otherwise, use ../../build if it exists, and ./build if not.
ifeq "$(BUILD_PREFIX)" ""
BUILD_PREFIX=$(shell [ -d ../../build ] && echo `pwd`/../../build || \
			   echo `pwd`/build)
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
	@cd pod-build && \
		cmake -DCMAKE_INSTALL_PREFIX=$(BUILD_PREFIX) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) ..

clean:
	rm -rf pod-build

uninstall:
	@echo removing...
	@cat pod-build/install_manifest.txt
	@rm -f `cat pod-build/install_manifest.txt`
