CXXFLAGS = -g -O0
CPPFLAGS = -I.. -I.

tamer: y.o tame_driver.o
	$(CXX) -o $@ $^

tame: tame.o parse.o scan.o processor.o const.o output.o
	$(CXX) -o $@ $<

tame_driver.o: tame_driver.cc tame_driver.hh tame_base.hh tame_event.hh tame_rendezvous.hh tame_util.hh
x.o: x.cc tame_driver.hh tame_base.hh tame_event.hh tame_rendezvous.hh tame_util.hh tame_adapter.hh
y.o: y.cc tame_driver.hh tame_base.hh tame_event.hh tame_rendezvous.hh tame_util.hh tame_adapter.hh
y.cc: y.tt ../compiler/tamer
	../compiler/tamer $< > $@

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
		if cmp -s y.tab.h $(srcdir)/parse.h; then	\
			rm -f y.tab.h;				\
		else						\
			mv y.tab.h $(srcdir)/parse.h;		\
		fi;						\
	fi

clean:
	rm -f *.o parse.cc scan.cc
