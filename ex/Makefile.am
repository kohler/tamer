noinst_PROGRAMS = ex1 ex2

ex1_SOURCES = ex1.tt
ex1_LDADD = ../tamer/libtamer.la

ex2_SOURCES = ex2.tt
ex2_LDADD = ../tamer/libtamer.la

TAMER = ../compiler/tamer
.tt.cc: $(TAMER)
	$(TAMER) -c $<

ex1.cc: ex1.tt $(TAMER)
ex2.cc: ex2.tt $(TAMER)

INCLUDES = -I../tamer -I..
