NCURSES_CFLAGS := $(shell pkg-config --cflags ncursesw)
NCURSES_LIBS := $(shell pkg-config --libs ncursesw)
LIBEV_LIBS := -lev
CHECK_CFLAGS := $(shell pkg-config --cflags check)
CHECK_LIBS := $(shell pkg-config --libs check)

CPPFLAGS =-DNDEBUG -D_XOPEN_SOURCE=700 -D_XOPEN_SOURCE_EXTENDED
CFLAGS = -std=c99 -pedantic -Wall -Wextra $(NCURSES_CFLAGS)
LIBS = $(NCURSES_LIBS) $(LIBEV_LIBS)

OBJECTS = \
	src/dict.o \
	src/dirmodel.o \
	src/list.o \
	src/listmodel.o \
	src/listview.o \
	src/main.o \

TESTEDOBJECTS = \
	src/list.o \

TESTOBJECTS = \
	tests/list.o \
	tests/tests.o \

DEPS = $(patsubst %.o,%.d,$(OBJECTS) $(TESTOBJECTS))

files: $(OBJECTS)
	$(LINK.c) -o files $^ $(LIBS)

test: tests/tests
	tests/tests
$(TESTOBJECTS): %.o: %.c
	$(COMPILE.c) $(CHECK_CFLAGS) -c -o $@ $<
tests/tests: $(TESTEDOBJECTS) $(TESTOBJECTS)
	$(LINK.c) -o $@ $^ $(CHECK_LIBS)

clean:
	rm -f files $(OBJECTS) $(TESTOBJECTS) $(DEPS)

$(OBJECTS) $(TESTOBJECTS): Makefile
-include $(DEPS)
