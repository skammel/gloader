### gloader - cmdline based GRBL gcode-loader
### Author: Skammel <git@skammel.de>

TRG = gloader
PREFIX = /usr
BINDIR = $(PREFIX)/bin
CC = gcc
SRCS = $(TRG).c
OBJS = $(SRCS:.c=.o)
CFLAGS += -W -Wall 
DIRFLAGS +=
LDLIBS +=
LDFLAGS += -s -Os

all: $(TRG)

$(TRG): $(OBJS)

clean:
	$(RM) -rf $(TRG) *~ *.o jj.jj *.so *.so.*
	
install: $(TRG)
	install --directory $(DESTDIR)$(BINDIR)
	install $(TRG) $(DESTDIR)$(BINDIR)


uninstall:
	$(RM) $(DESTDIR)$(BINDIR)/$(TRG)

%.o: %.c
	$(COMPILE.c) $(DIRFLAGS) $(OUTPUT_OPTION) $<
