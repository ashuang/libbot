default_target: all

$(VERBOSE).SILENT:

all: pod-build/Makefile
	$(MAKE) -C pod-build all install

pod-build/Makefile:
	./configure

clean:
	$(MAKE) -C pod-build clean

distclean:
	rm -rf pod-build

uninstall:
	@echo removing...
	@cat pod-build/install_manifest.txt
	@rm -f `cat pod-build/install_manifest.txt`
