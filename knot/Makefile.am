noinst_PROGRAMS = knot.tamer

knot_tamer_SOURCES = cache2.tt cache2.hh http.tt http.hh httphdrs.h \
	input.tt input.hh knot.tt refptr.hh
knot_tamer_LDADD = ../tamer/libtamer.la @DRIVER_LIBS@ @MALLOC_LIBS@

TAMER = ../compiler/tamer
.tt.cc:
	$(TAMER) -o $@ -c $< || (rm $@ && false)

cache2.cc: $(srcdir)/cache2.tt $(TAMER)
http.cc: $(srcdir)/http.tt $(TAMER)
input.cc: $(srcdir)/input.tt $(TAMER)
knot.cc: $(srcdir)/knot.tt $(TAMER)

AM_CPPFLAGS = -I$(top_srcdir) -I$(top_builddir)

clean-local:
	-rm -f cache2.cc http.cc input.cc knot.cc
