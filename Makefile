PROJECT = files

NCURSES_CFLAGS := $(shell pkg-config --cflags ncursesw)
NCURSES_LIBS := $(shell pkg-config --libs ncursesw)
LIBEV_LIBS := -lev
CHECK_CFLAGS := $(shell pkg-config --cflags check)
CHECK_LIBS := $(shell pkg-config --libs check)

all: $(PROJECT)

-include coverage.mk
COVERAGE = 0
CPPFLAGS =-DNDEBUG -D_XOPEN_SOURCE=700 -D_XOPEN_SOURCE_EXTENDED -DPROJECT=\"$(PROJECT)\"
CFLAGS = -std=c11 -MMD -pedantic -Wall -Wextra $(NCURSES_CFLAGS)
LIBS = $(NCURSES_LIBS) $(LIBEV_LIBS)

ifeq ($(COVERAGE),1)
CFLAGS += --coverage
endif

FORCE:

coverage.mk: FORCE
ifneq ($(COVERAGE),$(LAST_COVERAGE))
	@echo "LAST_COVERAGE = $(COVERAGE)" > coverage.mk
endif

OBJECTS = \
	src/dict.o \
	src/dirmodel.o \
	src/list.o \
	src/listmodel.o \
	src/listview.o \
	src/path.o \
	src/xdg.o \
	src/main.o \

TESTEDOBJECTS = \
	src/dict.o \
	src/dirmodel.o \
	src/list.o \
	src/listmodel.o \
	src/listview.o \
	src/path.o \
	src/xdg.o \

TESTOBJECTS = \
	tests/dict.o \
	tests/dirmodel.o \
	tests/list.o \
	tests/listmodel.o \
	tests/listview.o \
	tests/path.o \
	tests/xdg.o \
	tests/tests.o \
	tests/wrapper/getcwd.o \

DEPS = $(patsubst %.o,%.d,$(OBJECTS) $(TESTOBJECTS))
GCDAS = $(patsubst %.o,%.gcda,$(OBJECTS) $(TESTOBJECTS))
GCNOS = $(patsubst %.o,%.gcno,$(OBJECTS) $(TESTOBJECTS))

$(PROJECT): $(OBJECTS)
	$(LINK.c) -o $(PROJECT) $^ $(LIBS)

test: tests/tests
	rm -f $(patsubst %.o,%.gcda,$(TESTEDOBJECTS))
	tests/tests
ifeq ($(COVERAGE),1)
	gcov -n $(TESTEDOBJECTS)
endif

$(TESTOBJECTS): %.o: %.c
	$(COMPILE.c) $(CHECK_CFLAGS) -c -o $@ $<
tests/tests: $(TESTEDOBJECTS) $(TESTOBJECTS)
	$(LINK.c) -o $@ $^ $(CHECK_LIBS) $(NCURSES_LIBS) $(LIBEV_LIBS) -Wl,--wrap=getcwd

clean:
	rm -f $(PROJECT) $(OBJECTS) $(TESTOBJECTS) $(DEPS) $(GCNOS) $(GCDAS) *.gcov

$(OBJECTS) $(TESTOBJECTS): Makefile coverage.mk
-include $(DEPS)
