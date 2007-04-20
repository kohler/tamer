lib_LTLIBRARIES = libtamer.la
libtamer_la_SOURCES = \
	_base.cc _base.hh \
	_driver.cc _driver.hh \
	_event.hh \
	_rendezvous.hh \
	_util.hh \
	adapter.hh \
	fileio.tt fileio.hh \
	tamer.hh
include_HEADERS = tamer.hh
pkginclude_HEADERS = _base.hh \
	_driver.hh \
	_event.hh \
	_rendezvous.hh \
	_util.hh \
	adapter.hh \
	fileio.hh

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/tamer
TAMER = ../compiler/tamer

.tt.cc:
	$(TAMER) -o $@ -c $< || (rm $@ && false)

fileio.cc: $(TAMER) fileio.tt
