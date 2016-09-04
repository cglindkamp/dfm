NCURSES_CFLAGS := $(shell pkg-config --cflags ncursesw)
NCURSES_LIBS := $(shell pkg-config --libs ncursesw)
LIBEV_LIBS := -lev

CPPFLAGS =-DNDEBUG -D_XOPEN_SOURCE=700 -D_XOPEN_SOURCE_EXTENDED
CFLAGS = -std=c99 -pedantic -Wall -Wextra $(NCURSES_CFLAGS)
LIBS = $(NCURSES_LIBS) $(LIBEV_LIBS)

SOURCES = \
	src/dict.c \
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
