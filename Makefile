NCURSES_CFLAGS := $(shell pkg-config --cflags ncursesw)
NCURSES_LIBS := $(shell pkg-config --libs ncursesw)

CFLAGS = -MMD -Wall $(NCURSES_CFLAGS)
LIBS = $(NCURSES_LIBS)

SOURCES = \
	src/listmodel.c \
	src/listview.c \
	src/main.c \
	src/testmodel.c

OBJECTS = $(patsubst %.c,%.o,$(SOURCES))
DEPS = $(patsubst %.c,%.d,$(SOURCES))

files: $(OBJECTS)
	$(LINK.c) -o files $^ $(LIBS)

clean:
	rm -f files $(OBJECTS) $(DEPS)

$(OBJECTS): Makefile
-include $(DEPS)
