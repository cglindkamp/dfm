PROJECT = dfm

SYSCONFDIR = /etc
BINDIR = /usr/local/bin

NCURSES_CFLAGS := $(shell pkg-config --cflags ncursesw)
NCURSES_LIBS := $(shell pkg-config --libs ncursesw)
CHECK_CFLAGS := $(shell pkg-config --cflags check)
CHECK_LIBS := $(shell pkg-config --libs check)

all: $(PROJECT)

-include coverage.mk
COVERAGE = 0
CPPFLAGS =-DNDEBUG -D_XOPEN_SOURCE=700 -D_XOPEN_SOURCE_EXTENDED -DPROJECT=\"$(PROJECT)\"
CFLAGS = -std=c11 -MMD -pedantic -Wall -Wextra $(NCURSES_CFLAGS) -O2 -flto
LIBS = $(NCURSES_LIBS)

ifeq ($(COVERAGE),1)
CFLAGS += --coverage
endif

FORCE:

coverage.mk: FORCE
ifneq ($(COVERAGE),$(LAST_COVERAGE))
	@echo "LAST_COVERAGE = $(COVERAGE)" > coverage.mk
endif

HANDLERS = \
	open \
	delete \
	copy \
	move \
	shell \
	xterminal \

OBJECTS = \
	src/application.o \
	src/clipboard.o \
	src/command.o \
	src/commandline.o \
	src/dict.o \
	src/dirmodel.o \
	src/keymap.o \
	src/filedata.o \
	src/list.o \
	src/listmodel.o \
	src/listview.o \
	src/path.o \
	src/processmanager.o \
	src/xdg.o \
	src/util.o \
	src/main.o \

TESTEDOBJECTS = $(subst src/main.o,,$(OBJECTS))
TESTOBJECTS = \
	tests/commandline.o \
	tests/dict.o \
	tests/dirmodel.o \
	tests/filedata.o \
	tests/keymap.o \
	tests/list.o \
	tests/listmodel.o \
	tests/listview.o \
	tests/path.o \
	tests/processmanager.o \
	tests/xdg.o \
	tests/tests.o \
	tests/wrapper/getcwd.o \
	tests/wrapper/alloc.o \

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

vtest: tests/tests
	CK_FORK=no valgrind --leak-check=full tests/tests

testoom: tests/tests
	rm -f $(patsubst %.o,%.gcda,$(TESTEDOBJECTS))
	tests/tests oom
ifeq ($(COVERAGE),1)
	gcov -n $(TESTEDOBJECTS)
endif

vtestoom: tests/tests
	valgrind --leak-check=full tests/tests oom 2> valgrind-oom.log

$(TESTOBJECTS): %.o: %.c
	$(COMPILE.c) $(CHECK_CFLAGS) -c -o $@ $<
tests/tests: $(TESTEDOBJECTS) $(TESTOBJECTS)
	$(LINK.c) -o $@ $^ $(CHECK_LIBS) $(NCURSES_LIBS) -Wl,--wrap=getcwd -Wl,--wrap=malloc -Wl,--wrap=realloc -Wl,--wrap=strdup

install:
	install -m 755 -D $(PROJECT) $(BINDIR)/$(PROJECT)
	install -m 644 examples/keymap $(SYSCONFDIR)/xdg/$(PROJECT)/keymap
	for FILE in $(HANDLERS); do \
		install -m 755 -D examples/handlers/$$FILE $(SYSCONFDIR)/xdg/$(PROJECT)/handlers/$$FILE; \
	done

uninstall:
	rm $(BINDIR)/$(PROJECT)
	for FILE in $(HANDLERS); do \
		rm $(SYSCONFDIR)/xdg/$(PROJECT)/handlers/$$FILE; \
	done
	rmdir $(SYSCONFDIR)/xdg/$(PROJECT)/handlers;
	rmdir $(SYSCONFDIR)/xdg/$(PROJECT);

clean:
	rm -f $(PROJECT) $(OBJECTS) $(TESTOBJECTS) $(DEPS) $(GCNOS) $(GCDAS) *.gcov

$(OBJECTS) $(TESTOBJECTS): Makefile coverage.mk
-include $(DEPS)
