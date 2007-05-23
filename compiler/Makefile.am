## Process this file with automake to produce Makefile.in

bin_PROGRAMS = tamer

noinst_HEADERS = tame.hh parse.hh

tamer_SOURCES = parse.yy scan.ll processor.cc const.cc output.cc tame.cc
tamer_LDADD = $(LDADD) $(LDADD_THR)

YYFLAGS = -d

parse.hh: parse.cc

scan.cc: scan.ll
	$(LEX) $(AM_LFLAGS) $(LFLAGS) $(srcdir)/scan.ll \
		&& mv $(LEX_OUTPUT_ROOT).c $(srcdir)/scan.cc

parse.cc: parse.yy
	rm -f y.tab.c
	$(YACC) $(AM_YFLAGS) $(YYFLAGS) $(srcdir)/parse.yy
	case "$(YACC)" in					\
	*bison*)						\
		sed -e 's/union yyalloc/struct yyalloc/g'	\
			< y.tab.c > $(srcdir)/parse.cc ;	\
		rm -f y.tab.c					\
		;;						\
	*)							\
		mv -f y.tab.c $(srcdir)/parse.cc		\
		;;						\
	esac
	if test -f y.tab.h; then				\
		if cmp -s y.tab.h $(srcdir)/parse.hh; then	\
			rm -f y.tab.h;				\
		else						\
			mv y.tab.h $(srcdir)/parse.hh;		\
		fi;						\
	fi

parse.o: parse.cc tame.hh
# Note: certain versions of bison put __attribute__((unused)) after
# unused goto labels, which doesn't agree well with gcc 3.2.1.
# If you have problems, try the next line instead of the following one.
#	$(CXXCOMPILE) $(CXXNOERR) -D'__attribute__(x)=' -c $(srcdir)/parse.cc
	$(CXXCOMPILE) $(CXXNOERR) -c $(srcdir)/parse.cc

scan.o: parse.o scan.cc tame.hh
	$(CXXCOMPILE) $(CXXNOERR) -c $(srcdir)/scan.cc

.PHONY: parseclean
parseclean:
	rm -f $(srcdir)/parse.cc $(srcdir)/parse.hh $(srcdir)/scan.cc

BUILT_SOURCES = parse.cc parse.hh scan.cc

CLEANFILES = core *.core *~ *.rpo

EXTRA_DIST = .cvsignore
MAINTAINERCLEANFILES = parse.hh parse.cc scan.cc
