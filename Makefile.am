## Process this file with automake to produce Makefile.in
AUTOMAKE_OPTIONS = foreign check-news

EXTRA_DIST = LICENSE

SUBDIRS = compiler tamer test bench ex doc knot

compiler tamer test bench ex doc knot: Makefile config.h
	cd $@ && $(MAKE) $(AM_MAKEFLAGS)
tamer: compiler
test bench ex knot: compiler tamer

check: ex test
	$(srcdir)/test/testie $(srcdir)/test

check-%: ex test
	$(srcdir)/test/testie $(srcdir)/test/$*.testie

.PHONY: check compiler tamer test bench ex doc knot
