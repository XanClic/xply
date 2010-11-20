DECODERS = ogg-vorbis riff-wave
OUTPUT = null sdl

ADDLIBS = -lvorbisfile -lvorbis -logg -lm `sdl-config --libs`
ADDCFGS = `sdl-config --cflags`

MCFLAGS = $(CFLAGS) -O3 -std=gnu99 -Wall -Wextra -pedantic

CC = gcc

B_CC = $(CC) -o
O_CC = $(CC) -c -o
COPY = cp
CPTH = install -d
RMFF = rm -f

.PHONY: all install clean

all: xply

xply: main.o interface.o $(addsuffix .o,$(DECODERS) $(OUTPUT))
	$(B_CC) $@ $^ $(LINKFLAGS) $(ADDLIBS)

%.o: %.c player.h
	$(O_CC) $@ $< $(MCFLAGS) $(ADDCFGS)

install:
	$(CPTH) $(INSTALL_TO)/bin
	$(COPY) xply $(INSTALL_TO)/bin

clean:
	$(RMFF) *.o xply
