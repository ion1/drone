.PHONY : all build install clean

all : build

build : build/Makefile
	$(MAKE) -C build

install : build/Makefile
	$(MAKE) -C build install

build/Makefile :
	mkdir -p build
	cd build; cmake ..

clean ::
	$(RM) -r build
