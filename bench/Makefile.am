noinst_PROGRAMS = b01-asapwto

b01_asapwto_SOURCES = b01-asapwto.tcc

DRIVER_LIBS = @DRIVER_LIBS@
MALLOC_LIBS = @MALLOC_LIBS@
LDADD = ../tamer/libtamer.la $(DRIVER_LIBS) $(MALLOC_LIBS)

AM_CPPFLAGS = -I$(top_srcdir) -I$(top_builddir)

TAMER = ../compiler/tamer
.tcc.cc:
	$(TAMER) -o $@ -c $<  || (rm $@ && false)

b01-asapwto.cc: $(srcdir)/b01-asapwto.tcc $(TAMER)

TAMED_CXXFILES = b01-asapwto.cc
CLEANFILES = $(TAMED_CXXFILES)
.PRECIOUS: $(TAMED_CXXFILES)
