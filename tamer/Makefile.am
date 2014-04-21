pkgconfigdir = ${libdir}/pkgconfig
pkgconfig_DATA = tamer.pc
AUTOMAKE_OPTIONS = subdir-objects

bin_PROGRAMS =

lib_LTLIBRARIES = libtamer.la
libtamer_la_SOURCES = \
	adapter.hh \
	bufferedio.hh bufferedio.tt \
	channel.hh \
	driver.hh \
	dinternal.hh dinternal.cc \
	dlibev.cc \
	dlibevent.cc \
	dtamer.cc \
	dsignal.cc \
	event.hh \
	fd.hh fd.tcc \
	dns.hh dns.tt \
	lock.hh lock.tt \
	ref.hh \
	rendezvous.hh \
	tamer.hh \
	xadapter.hh xadapter.cc \
	xbase.hh xbase.cc \
	xdriver.hh \
	xevent.hh
pkginclude_HEADERS = \
	adapter.hh \
	autoconf.h \
	bufferedio.hh \
	channel.hh \
	driver.hh \
	event.hh \
	fd.hh \
	dns.hh \
	lock.hh \
	ref.hh \
	rendezvous.hh \
	tamer.hh \
	xadapter.hh \
	xbase.hh \
	xdriver.hh

tamerfdh_SOURCES = fdhelp.cc

EXTRA_DIST = autoconf.h.in \
	tamer.pc.in

AM_CPPFLAGS = -I$(top_srcdir) -I$(top_builddir)
TAMER = ../compiler/tamer

if HTTP_PARSER
libtamer_la_SOURCES += ../http-parser/http_parser.c http.tcc http.hh
AM_CPPFLAGS += -I$(top_srcdir)/http-parser
pkginclude_HEADERS += $(top_srcdir)/http-parser/http_parser.h
if TAMER_DEBUG
AM_CPPFLAGS += -DHTTP_PARSER_STRICT=1
else
AM_CPPFLAGS += -DHTTP_PARSER_STRICT=0
endif
endif

if FDHELPER
libtamer_la_SOURCES += fdhmsg.hh fdhmsg.cc fdh.hh fdh.tt
bin_PROGRAMS += tamerfdh
endif

.tt.cc:
	$(TAMER) -g -o $@ -c $< || (rm $@ && false)
.tcc.cc:
	$(TAMER) -g -o $@ -c $< || (rm $@ && false)

fd.cc: $(TAMER) fd.tcc
fdh.cc: $(TAMER) fdh.tt
dns.cc: $(TAMER) dns.tt
lock.cc: $(TAMER) lock.tt
bufferedio.cc: $(TAMER) bufferedio.tt
http.cc: $(TAMER) http.tcc

clean-local:
	-rm -f lock.cc fd.cc fdh.cc dns.cc bufferedio.cc http.cc
