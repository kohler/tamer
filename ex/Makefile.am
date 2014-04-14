noinst_PROGRAMS = ex6 tamer-yes tamer-echosrv
EXTRA_PROGRAMS = osptracker

ex6_SOURCES = ex6.tt
ex6_LDADD = ../tamer/libtamer.la $(DRIVER_LIBS) $(MALLOC_LIBS)

osptracker_SOURCES = osptracker.tcc md5.c md5.h
osptracker_LDADD = ../tamer/libtamer.la $(DRIVER_LIBS) $(MALLOC_LIBS)

tamer_yes_SOURCES = tamer-yes.tcc
tamer_yes_LDADD = ../tamer/libtamer.la $(DRIVER_LIBS) $(MALLOC_LIBS)

tamer_echosrv_SOURCES = tamer-echosrv.tcc
tamer_echosrv_LDADD = ../tamer/libtamer.la $(DRIVER_LIBS) $(MALLOC_LIBS)

tamer_httpd_SOURCES = tamer-httpd.tcc
tamer_httpd_LDADD = ../tamer/libtamer.la $(DRIVER_LIBS) $(MALLOC_LIBS)

DRIVER_LIBS = @DRIVER_LIBS@
MALLOC_LIBS = @MALLOC_LIBS@

AM_CPPFLAGS = -I$(top_srcdir) -I$(top_builddir)

if HTTP_PARSER
noinst_PROGRAMS += tamer-httpd
AM_CPPFLAGS += -I$(top_srcdir)/http-parser
endif

TAMER = ../compiler/tamer
.tt.cc: $(TAMER)
	$(TAMER) -g -o $@ -c $<  || (rm $@ && false)
.tcc.cc: $(TAMER)
	$(TAMER) -g -o $@ -c $<  || (rm $@ && false)

ex6.cc: $(srcdir)/ex6.tt $(TAMER)
osptracker.cc: $(srcdir)/osptracker.tcc $(TAMER)
tamer-yes.cc: $(srcdir)/tamer-yes.tcc $(TAMER)
tamer-echosrv.cc: $(srcdir)/tamer-echosrv.tcc $(TAMER)
tamer-httpd.cc: $(srcdir)/tamer-httpd.tcc $(TAMER)

clean-local:
	-rm -f ex6.cc osptracker.cc tamer-yes.cc tamer-echosrv.cc tamer-httpd.cc
