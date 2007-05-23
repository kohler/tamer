noinst_PROGRAMS = ex1 ex2 ex3

ex1_SOURCES = ex1.tt
ex1_LDADD = ../tamer/libtamer.la $(LIBEVENT_LIBS)

ex2_SOURCES = ex2.tt
ex2_LDADD = ../tamer/libtamer.la $(LIBEVENT_LIBS)

ex3_SOURCES = ex3.tt
ex3_LDADD = ../tamer/libtamer.la $(LIBEVENT_LIBS)

LIBEVENT_LIBS = @LIBEVENT_LIBS@

INCLUDES = -I$(top_srcdir)

TAMER = ../compiler/tamer
.tt.cc: $(TAMER)
	$(TAMER) -g -o $@ -c $<  || (rm $@ && false)

ex1.cc: ex1.tt $(TAMER)
ex2.cc: ex2.tt $(TAMER)
ex3.cc: ex3.tt $(TAMER)

clean-local:
	-rm -f ex1.cc ex2.cc ex3.cc
