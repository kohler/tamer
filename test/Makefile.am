noinst_PROGRAMS = t02 t03 t04 t05 t06 t07 t08 t09

t02_SOURCES = t02.tt
t02_LDADD = ../tamer/libtamer.la $(DRIVER_LIBS) $(MALLOC_LIBS)

t03_SOURCES = t03.tt
t03_LDADD = ../tamer/libtamer.la $(DRIVER_LIBS) $(MALLOC_LIBS)

t04_SOURCES = t04.tt
t04_LDADD = ../tamer/libtamer.la $(DRIVER_LIBS) $(MALLOC_LIBS)

t05_SOURCES = t05.tt
t05_LDADD = ../tamer/libtamer.la $(DRIVER_LIBS) $(MALLOC_LIBS)

t06_SOURCES = t06.tcc
t06_LDADD = ../tamer/libtamer.la $(DRIVER_LIBS) $(MALLOC_LIBS)

t07_SOURCES = t07.tcc
t07_LDADD = ../tamer/libtamer.la $(DRIVER_LIBS) $(MALLOC_LIBS)

t08_SOURCES = t08.tcc
t08_LDADD = ../tamer/libtamer.la $(DRIVER_LIBS) $(MALLOC_LIBS)

t09_SOURCES = t09.tcc
t09_LDADD = ../tamer/libtamer.la $(DRIVER_LIBS) $(MALLOC_LIBS)

TAMED_CXXFILES = t02.cc t03.cc t04.cc t05.cc t06.cc t07.cc t08.cc t09.cc

DRIVER_LIBS = @DRIVER_LIBS@
MALLOC_LIBS = @MALLOC_LIBS@

AM_CPPFLAGS = -I$(top_srcdir) -I$(top_builddir)
DEFS = -DTAMER_DEBUG

TAMER = ../compiler/tamer
.tt.cc:
	$(TAMER) -o $@ -c $<  || (rm $@ && false)
.tcc.cc:
	$(TAMER) -o $@ -c $<  || (rm $@ && false)

t02.cc: $(srcdir)/t02.tt $(TAMER)
t03.cc: $(srcdir)/t03.tt $(TAMER)
t04.cc: $(srcdir)/t04.tt $(TAMER)
t05.cc: $(srcdir)/t05.tt $(TAMER)
t06.cc: $(srcdir)/t06.tcc $(TAMER)
t07.cc: $(srcdir)/t07.tcc $(TAMER)
t08.cc: $(srcdir)/t08.tcc $(TAMER)
t09.cc: $(srcdir)/t09.tcc $(TAMER)

clean-local:
	-rm -f $(TAMED_CXXFILES)

.PRECIOUS: $(TAMED_CXXFILES)
