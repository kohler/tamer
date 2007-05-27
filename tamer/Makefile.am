lib_LTLIBRARIES = libtamer.la
libtamer_la_SOURCES = \
	adapter.hh \
	bufferedio.hh bufferedio.tt \
	driver.hh \
	dbase.cc \
	dlibevent.cc \
	dtamer.cc \
	dsignal.cc \
	event.hh \
	fileio.hh fileio.tt \
	lock.hh lock.tt \
	rendezvous.hh \
	tamer.hh \
	util.hh \
	xadapter.hh \
	xbase.hh xbase.cc \
	xdriver.hh \
	xevent.hh
pkginclude_HEADERS = \
	adapter.hh \
	bufferedio.hh \
	driver.hh \
	event.hh \
	fileio.hh \
	lock.hh \
	rendezvous.hh \
	tamer.hh \
	util.hh \
	xadapter.hh \
	xbase.hh \
	xdriver.hh \
	xevent.hh

INCLUDES = -I$(top_srcdir)
TAMER = ../compiler/tamer

.tt.cc:
	$(TAMER) -o $@ -c $< || (rm $@ && false)

fileio.cc: $(TAMER) fileio.tt
lock.cc: $(TAMER) lock.tt
bufferedio.cc: $(TAMER) bufferedio.tt

clean-local:
	-rm -f fileio.cc lock.cc bufferedio.cc
