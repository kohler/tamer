noinst_PROGRAMS = t02 t03 t04 t05 t06

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

TAMED_CXXFILES = t02.cc t03.cc t04.cc t05.cc t06.cc

DRIVER_LIBS = @DRIVER_LIBS@
MALLOC_LIBS = @MALLOC_LIBS@

INCLUDES = -I$(top_srcdir) -I$(top_builddir)
DEFS = -DTAMER_DEBUG

TAMER = ../compiler/tamer
.tt.cc: $(TAMER)
	$(TAMER) -o $@ -c $<  || (rm $@ && false)
.tcc.cc: $(TAMER)
	$(TAMER) -o $@ -c $<  || (rm $@ && false)

clean-local:
	-rm -f $(TAMED_CXXFILES)

.PRECIOUS: $(TAMED_CXXFILES)
