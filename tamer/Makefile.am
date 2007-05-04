lib_LTLIBRARIES = libtamer.la
libtamer_la_SOURCES = \
	base.cc base.hh \
	driver.cc driver.hh \
	event.hh \
	rendezvous.hh \
	util.hh \
	adapter.hh \
	fileio.tt fileio.hh \
	lock.tt lock.hh \
	tamer.hh
include_HEADERS = tamer.hh
pkginclude_HEADERS = base.hh \
	driver.hh \
	event.hh \
	rendezvous.hh \
	util.hh \
	adapter.hh \
	fileio.hh \
	lock.hh

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/tamer
TAMER = ../compiler/tamer

.tt.cc:
	$(TAMER) -o $@ -c $< || (rm $@ && false)

fileio.cc: $(TAMER) fileio.tt
lock.cc: $(TAMER) lock.tt
