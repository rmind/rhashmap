#
# This file is in the Public Domain.
#

PROJ=		rhashmap

SYSNAME:=	$(shell uname -s)

CFLAGS+=	-std=c23 -O2 -g -Wall -Wextra -Werror

#
# Extended warning flags.
#
CFLAGS+=	-Wno-unknown-warning-option # gcc vs clang
CFLAGS+=	-Wstrict-prototypes -Wmissing-prototypes -Wpointer-arith
CFLAGS+=	-Wmissing-declarations -Wredundant-decls -Wnested-externs
CFLAGS+=	-Wshadow -Wcast-qual -Wcast-align -Wwrite-strings
CFLAGS+=	-Wold-style-definition
CFLAGS+=	-Wsuggest-attribute=noreturn -Wjump-misses-init
CFLAGS+=	-Wduplicated-cond -Wmisleading-indentation -Wnull-dereference
CFLAGS+=	-Wduplicated-branches -Wrestrict

#
# Standard vs debug build flags.
#

ifeq ($(MAKECMDGOALS),tests)
DEBUG=		1
endif

ifdef DEBUG
CFLAGS+=	-O0 -ggdb -fno-omit-frame-pointer -DDEBUG
CFLAGS+=	-fsanitize=address -fsanitize=undefined
LDFLAGS+=	-fsanitize=address -fsanitize=undefined
else
CFLAGS+=	-DNDEBUG
CFLAGS+=	-fvisibility=hidden
endif

#
# System-specific compiler flags.
#
ifeq ($(SYSNAME),Linux)
CFLAGS+=	-D_POSIX_C_SOURCE=200809L -D_GNU_SOURCE -D_DEFAULT_SOURCE
endif
ifeq ($(SYSNAME),Darwin)
LIBTOOL=	glibtool
LDFLAGS+=	-no-undefined
endif

LIBTOOL?=	libtool

#
# Main targets.
#

LIB=		lib$(PROJ)
INCS=		rhashmap.h

OBJS=		rhashmap.o
OBJS+=		murmurhash.o
OBJS+=		siphash.o

$(LIB).la:	LDFLAGS+=	-rpath $(LIBDIR) -version-info 1:0:0
install/%.la:	ILIBDIR=	$(DESTDIR)/$(LIBDIR)
install:	IINCDIR=	$(DESTDIR)/$(INCDIR)/
#install:	IMANDIR=	$(DESTDIR)/$(MANDIR)/man3/

obj: $(OBJS)

lib: $(LIB).la

%.lo: %.c
	$(LIBTOOL) --mode=compile --tag CC $(CC) $(CFLAGS) -c $<

$(LIB).la: $(shell echo $(OBJS) | sed 's/\.o/\.lo/g')
	$(LIBTOOL) --mode=link --tag CC $(CC) $(LDFLAGS) -o $@ $(notdir $^)

install/%.la: %.la
	mkdir -p $(ILIBDIR)
	$(LIBTOOL) --mode=install install -c $(notdir $@) $(ILIBDIR)/$(notdir $@)

install: $(addprefix install/,$(LIB).la)
	$(LIBTOOL) --mode=finish $(LIBDIR)
	mkdir -p $(IINCDIR) && install -c $(INCS) $(IINCDIR)
	#mkdir -p $(IMANDIR) && install -c $(MANS) $(IMANDIR)

tests: $(OBJS) t_$(PROJ).o
	$(CC) $(CFLAGS) $^ -o t_$(PROJ)
	MALLOC_CHECK_=3 ./t_$(PROJ)

clean:
	$(LIBTOOL) --mode=clean rm
	rm -rf .libs *.o *.lo *.la t_$(PROJ)

.PHONY: all obj lib install tests clean
