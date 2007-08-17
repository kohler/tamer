noinst_PROGRAMS = t01

t01_SOURCES = t01.cc
ex1_LDADD = ../tamer/libtamer.la $(LIBEVENT_LIBS)

LIBEVENT_LIBS = @LIBEVENT_LIBS@

INCLUDES = -I$(top_srcdir)

TAMER = ../compiler/tamer
.tt.cc: $(TAMER)
	$(TAMER) -g -o $@ -c $<  || (rm $@ && false)
.tcc.cc: $(TAMER)
	$(TAMER) -g -o $@ -c $<  || (rm $@ && false)
