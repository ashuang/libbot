default_target: all

$(VERBOSE).SILENT:

all: pod-build/Makefile  pod-build/build_prefix
	$(MAKE) -C pod-build all install
	 cd python && python setup.py install --prefix=`cat ../pod-build/build_prefix`

pod-build/Makefile:
	./configure

pod-build/build_prefix: pod-build/Makefile

clean:
	$(MAKE) -C pod-build clean

distclean:
	rm -rf pod-build
