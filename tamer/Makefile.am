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
	fd.hh fd.tt \
	dns.hh dns.tt \
	lock.hh lock.tt \
	ref.hh \
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
	fd.hh \
	dns.hh \
	lock.hh \
	ref.hh \
	rendezvous.hh \
	tamer.hh \
	util.hh \
	xadapter.hh \
	xbase.hh \
	xdriver.hh \
	xevent.hh

INCLUDES = -I$(top_srcdir) -I$(top_builddir)
TAMER = ../compiler/tamer

.tt.cc:
	$(TAMER) -o $@ -c $< || (rm $@ && false)

fd.cc: $(TAMER) fd.tt
dns.cc: $(TAMER) dns.tt
lock.cc: $(TAMER) lock.tt
bufferedio.cc: $(TAMER) bufferedio.tt

clean-local:
	-rm -f lock.cc fd.cc dns.cc bufferedio.cc
