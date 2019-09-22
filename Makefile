PROGNAME := hnc8
BINDIR := bin
OBJDIR := obj

ARCH_PREFIX :=
CC := $(ARCH_PREFIX)clang

VERSION_STR := \"0.1-$(shell git rev-list --count HEAD)\"

LIBS := glfw3 gl
LDFLAGS := $(shell pkg-config --libs $(LIBS)) -flto
CFLAGS := $(shell pkg-config --cflags $(LIBS)) -std=c99 -DHNC8_VERSION=$(VERSION_STR)
CFLAGS_RELEASE := -Wall -Wpedantic -Werror -Wuninitialized -O2 -DNDEBUG
CFLAGS_DEBUG := -ggdb -g3 -gfull -O0 -DDEBUG

SRCS := $(wildcard *.c)
OBJS := $(patsubst %.c, $(OBJDIR)/%.o, $(SRCS))

.PHONY: release
release: CFLAGS += $(CFLAGS_RELEASE)
release: $(BINDIR)/$(PROGNAME)

.PHONY: debug
debug: CFLAGS += $(CFLAGS_DEBUG)
debug: $(BINDIR)/$(PROGNAME)

$(OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(BINDIR)/$(PROGNAME): $(OBJDIR) $(BINDIR) $(OBJS)
	$(CC) -o $(BINDIR)/$(PROGNAME) $(OBJS) $(LDFLAGS)

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(BINDIR):
	mkdir -p $(BINDIR)

.PHONY: clean
clean:
	rm -fv $(OBJDIR)/*.o $(BINDIR)/$(PROGNAME)
