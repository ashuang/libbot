default_target: all

# get a list of pods to build by reading pods/tobuild.txt
PODS:=$(shell grep -v "^\#" pods/tobuild.txt)

# Figure out where to build the software.
#   Use BUILD_PREFIX if it was passed in.
#   If not, search up to two parent directories for a 'build' directory.
#   Otherwise, use ./build.
ifeq "$(BUILD_PREFIX)" ""
BUILD_PREFIX=$(shell for pfx in .. ../..; do d=`pwd`/$$pfx/build; \
               if [ -d $$d ]; then echo $$d; exit 0; fi; done; echo `pwd`/build)
endif

# build quietly by default.  For a verbose build, run "make VERBOSE=1"
$(VERBOSE).SILENT:

all: 
	@[ -d $(BUILD_PREFIX) ] || mkdir -p $(BUILD_PREFIX) || exit 1
	@for pod in $(PODS); do \
		echo "\n-------------------------------------------"; \
		echo "-- POD: $$pod"; \
		echo "-------------------------------------------"; \
		$(MAKE) -C pods/$$pod all || exit 2; \
	done
	@# Place additional commands here if you have any

clean:
	@for pod in $(PODS); do \
		echo "\n-------------------------------------------"; \
		echo "-- POD: $$pod"; \
		echo "-------------------------------------------"; \
		$(MAKE) -C pods/$$pod clean; \
	done
	rm -rf build/bin
	rm -rf build/include
	rm -rf build/lib
	rm -rf build/share

	@# Place additional commands here if you have any

uninstall:
	@for pod in $(PODS); do \
		echo "\n-------------------------------------------"; \
		echo "-- POD: $$pod"; \
		echo "-------------------------------------------"; \
		 $(MAKE) -C pods/$$pod uninstall; \
	done
