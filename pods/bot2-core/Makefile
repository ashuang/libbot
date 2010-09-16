default_target: all

$(VERBOSE).SILENT:

all: cmake-build/Makefile
	$(MAKE) -C cmake-build all install

cmake-build/Makefile:
	./configure

clean:
	$(MAKE) -C cmake-build clean

distclean:
	rm -rf cmake-build
