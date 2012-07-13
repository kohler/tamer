noinst_PROGRAMS = t01 t02

t01_SOURCES = t01.cc
t01_LDADD = ../tamer/libtamer.la $(LIBEVENT_LIBS)

t02_SOURCES = t02.tt
t02_LDADD = ../tamer/libtamer.la $(LIBEVENT_LIBS)

LIBEVENT_LIBS = @LIBEVENT_LIBS@

INCLUDES = -I$(top_srcdir)

TAMER = ../compiler/tamer
.tt.cc: $(TAMER)
	$(TAMER) -g -o $@ -c $<  || (rm $@ && false)
.tcc.cc: $(TAMER)
	$(TAMER) -g -o $@ -c $<  || (rm $@ && false)

clean-local:
	-rm -f t02.cc
