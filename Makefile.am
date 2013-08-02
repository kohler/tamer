## Process this file with automake to produce Makefile.in
AUTOMAKE_OPTIONS = foreign check-news

EXTRA_DIST = LICENSE

SUBDIRS = compiler tamer ex knot doc test

compiler tamer ex knot doc test:
	cd $@ && $(MAKE) $(AM_MAKEFLAGS)
tamer: compiler
ex knot test: compiler tamer

check: ex test
	$(srcdir)/test/testie $(srcdir)/test

.PHONY: check compiler tamer ex knot doc test
