default_target: all

$(VERBOSE).SILENT:

# Figure out where to build the pod.  If BUILD_PREFIX was passed in, then use
# that.  Otherwise, use ../../build if it exists, and ./build otherwise.
ifeq "$(BUILD_PREFIX)" ""
BUILD_PREFIX=$(shell [ -d ../../build ] && echo `pwd`/../../build || \
			   echo `pwd`/build)
endif

all: pod-build/Makefile
	$(MAKE) -C pod-build all install

pod-build/Makefile:
	@echo "\nBUILD_PREFIX: $(BUILD_PREFIX)\n\n"

	# create the build directories if necessary
	@[ -d $(BUILD_PREFIX) ] || mkdir -p $(BUILD_PREFIX) || exit 1
	@[ -d pod-build ] || mkdir pod-build || exit 1

	# run CMake to generate and configure the build scripts
	@cd pod-build && cmake -DCMAKE_INSTALL_PREFIX=$(BUILD_PREFIX) $* ..

clean:
	[ -d pod-build ] && $(MAKE) -C pod-build clean

distclean:
	[ -d pod-build ] && rm -rf pod-build

uninstall:
	@echo removing...
	@cat pod-build/install_manifest.txt
	@rm -f `cat pod-build/install_manifest.txt`
