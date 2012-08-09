noinst_PROGRAMS = t01 t02 t03 t04

t01_SOURCES = t01.cc
t01_LDADD = ../tamer/libtamer.la $(LIBEVENT_LIBS) $(MALLOC_LIBS)

t02_SOURCES = t02.tt
t02_LDADD = ../tamer/libtamer.la $(LIBEVENT_LIBS) $(MALLOC_LIBS)

t03_SOURCES = t03.tt
t03_LDADD = ../tamer/libtamer.la $(LIBEVENT_LIBS) $(MALLOC_LIBS)

t04_SOURCES = t04.tt
t04_LDADD = ../tamer/libtamer.la $(LIBEVENT_LIBS) $(MALLOC_LIBS)

LIBEVENT_LIBS = @LIBEVENT_LIBS@
MALLOC_LIBS = @MALLOC_LIBS@

INCLUDES = -I$(top_srcdir) -I$(top_builddir)

TAMER = ../compiler/tamer
.tt.cc: $(TAMER)
	$(TAMER) -o $@ -c $<  || (rm $@ && false)
.tcc.cc: $(TAMER)
	$(TAMER) -o $@ -c $<  || (rm $@ && false)

clean-local:
	-rm -f t02.cc t03.cc t04.cc
