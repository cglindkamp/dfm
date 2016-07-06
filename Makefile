NCURSES_CFLAGS := $(shell pkg-config --cflags ncursesw)
NCURSES_LIBS := $(shell pkg-config --libs ncursesw)
LIBEV_CFLAGS := $(shell pkg-config --cflags libev)
LIBEV_LIBS := $(shell pkg-config --libs libev)

CFLAGS = -MMD -Wall -Wextra -DNDEBUG $(NCURSES_CFLAGS) $(LIBEV_CFLAGS)
LIBS = $(NCURSES_LIBS) $(LIBEV_LIBS)

SOURCES = \
	src/dirmodel.c \
	src/list.c \
	src/listmodel.c \
	src/listview.c \
	src/main.c \

OBJECTS = $(patsubst %.c,%.o,$(SOURCES))
DEPS = $(patsubst %.c,%.d,$(SOURCES))

files: $(OBJECTS)
	$(LINK.c) -o files $^ $(LIBS)

clean:
	rm -f files $(OBJECTS) $(DEPS)

$(OBJECTS): Makefile
-include $(DEPS)
