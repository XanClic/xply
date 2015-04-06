DECODERS = ogg-vorbis riff-wave mp3
OUTPUT = null sdl

ADDLIBS =
ADDCFGS =

ifneq (,$(findstring ogg-vorbis,$(DECODERS)))
    ADDLIBS += -lvorbisfile -lvorbis -logg -lm
endif

ifneq (,$(findstring mp3,$(DECODERS)))
    ADDLIBS += -lmpg123
endif

ifneq (,$(findstring sdl,$(OUTPUT)))
    ADDLIBS += -lvorbisfile -lvorbis -logg -lmpg123 -lm `sdl-config --libs`
    ADDCFGS += `sdl-config --cflags`
endif

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

%.o: %.c player.h config.h
	$(O_CC) $@ $< $(MCFLAGS) $(ADDCFGS)

config.h:
ifneq (,$(findstring sdl,$(OUTPUT)))
	echo '#define DEFAULT_OUTPUT_DEVICE "sdl"' > config.h
else
    ifneq (,$(findstring tyn,$(OUTPUT)))
	echo '#define DEFAULT_OUTPUT_DEVICE "tyndur"' > config.h
    else
	echo '#define DEFAULT_OUTPUT_DEVICE "null"' > config.h
    endif
endif

install:
	$(CPTH) $(INSTALL_TO)/bin
	$(COPY) xply $(INSTALL_TO)/bin

clean:
	$(RMFF) *.o xply config.h
