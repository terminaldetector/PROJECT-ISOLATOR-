# SYNTAXIS: FRACTAL GENESIS - Sega Mega Drive tech demo
#
# Two ways to build:
#   1. If you have SGDK installed (GDK env var set): make
#   2. With docker:                                  make docker

ifdef GDK

all:
	$(MAKE) -f $(GDK)/makefile.gen

clean:
	$(MAKE) -f $(GDK)/makefile.gen clean

else

all: docker

clean:
	rm -rf out

endif

docker:
	docker run --rm -v "$(CURDIR)":/src ghcr.io/stephane-d/sgdk:latest

.PHONY: all clean docker
