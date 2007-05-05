noinst_PROGRAMS = ex1 ex2 ex3

ex1_SOURCES = ex1.tt
ex1_LDADD = ../tamer/libtamer.la

ex2_SOURCES = ex2.tt
ex2_LDADD = ../tamer/libtamer.la

ex3_SOURCES = ex3.tt
ex3_LDADD = ../tamer/libtamer.la

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/tamer

TAMER = ../compiler/tamer
.tt.cc: $(TAMER)
	$(TAMER) -g -o $@ -c $<  || (rm $@ && false)

ex1.cc: ex1.tt $(TAMER)
ex2.cc: ex2.tt $(TAMER)
ex3.cc: ex3.tt $(TAMER)

clean-local:
	-rm -f ex1.cc ex2.cc ex3.cc
