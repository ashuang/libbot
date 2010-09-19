default_target: all

# get a list of pods to build by reading pods/tobuild.txt
PODS:=$(shell grep -v "^\#" pods/tobuild.txt)

# build quietly by default.  For a verbose build, run "make VERBOSE=1"
$(VERBOSE).SILENT:

all: 
	@for pod in $(PODS); do echo $$pod; $(MAKE) -C pods/$$pod all ||exit 2; done

	@# Place additional commands here if you have any

clean:
	@for pod in $(PODS); do echo $$pod; $(MAKE) -C pods/$$pod clean; done
	rm -rf build/*

	@# Place additional commands here if you have any

distclean:
	@for pod in $(PODS); do echo $$pod; $(MAKE) -C pods/$$pod distclean; done
	rm -rf build/bin
	rm -rf build/include
	rm -rf build/lib
	rm -rf build/share

	@# Place additional commands here if you have any
