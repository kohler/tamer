noinst_PROGRAMS = ex1 ex2 ex3 ex4 ex5 ex6
EXTRA_PROGRAMS = osptracker

ex1_SOURCES = ex1.tt
ex1_LDADD = ../tamer/libtamer.la $(LIBEVENT_LIBS) $(MALLOC_LIBS)

ex2_SOURCES = ex2.tt
ex2_LDADD = ../tamer/libtamer.la $(LIBEVENT_LIBS) $(MALLOC_LIBS)

ex3_SOURCES = ex3.tt
ex3_LDADD = ../tamer/libtamer.la $(LIBEVENT_LIBS) $(MALLOC_LIBS)

ex4_SOURCES = ex4.tt
ex4_LDADD = ../tamer/libtamer.la $(LIBEVENT_LIBS) $(MALLOC_LIBS)

ex5_SOURCES = ex5.tt
ex5_LDADD = ../tamer/libtamer.la $(LIBEVENT_LIBS) $(MALLOC_LIBS)

ex6_SOURCES = ex6.tt
ex6_LDADD = ../tamer/libtamer.la $(LIBEVENT_LIBS) $(MALLOC_LIBS)

osptracker_SOURCES = osptracker.tcc md5.c md5.h
osptracker_LDADD = ../tamer/libtamer.la $(LIBEVENT_LIBS) $(MALLOC_LIBS)

LIBEVENT_LIBS = @LIBEVENT_LIBS@
MALLOC_LIBS = @MALLOC_LIBS@

INCLUDES = -I$(top_srcdir)

TAMER = ../compiler/tamer
.tt.cc: $(TAMER)
	$(TAMER) -g -o $@ -c $<  || (rm $@ && false)
.tcc.cc: $(TAMER)
	$(TAMER) -g -o $@ -c $<  || (rm $@ && false)

ex1.cc: ex1.tt $(TAMER)
ex2.cc: ex2.tt $(TAMER)
ex3.cc: ex3.tt $(TAMER)
ex4.cc: ex4.tt $(TAMER)
ex5.cc: ex5.tt $(TAMER)
ex6.cc: ex6.tt $(TAMER)
osptracker.cc: osptracker.tcc $(TAMER)

clean-local:
	-rm -f ex1.cc ex2.cc ex3.cc ex4.cc ex5.cc ex6.cc osptracker.cc
