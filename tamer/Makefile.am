lib_LTLIBRARIES = libtamer.la
libtamer_la_SOURCES = \
	adapter.hh \
	base.cc base.hh \
	driver.cc driver.hh \
	event.hh \
	fileio.tt fileio.hh \
	lock.tt lock.hh \
	rendezvous.hh \
	tamer.hh \
	util.hh \
	xadapter.hh \
	xdriver.hh \
	xevent.hh
include_HEADERS = tamer.hh
pkginclude_HEADERS = \
	adapter.hh \
	base.hh \
	driver.hh \
	event.hh \
	fileio.hh \
	lock.hh \
	rendezvous.hh \
	util.hh \
	xadapter.hh \
	xdriver.hh \
	xevent.hh

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/tamer
TAMER = ../compiler/tamer

.tt.cc:
	$(TAMER) -o $@ -c $< || (rm $@ && false)

fileio.cc: $(TAMER) fileio.tt
lock.cc: $(TAMER) lock.tt

clean-local:
	-rm -f fileio.cc lock.cc
