## Process this file with automake to produce Makefile.in	-*- makefile -*-
#   $Source: /var/local/cvs/gasnet/other/Makefile.am,v $
#     $Date: 2011/04/22 10:31:28 $
# $Revision: 1.37 $
# Description: Makefile for GASNet other 
# Copyright 2002, Dan Bonachea <bonachea@cs.berkeley.edu>
# Terms of use are as specified in license.txt

AUTOMAKE_OPTIONS = foreign 1.4

# GASNET_TOOLSONLY_TRUE_BEGIN
# CONDUITMODE_SUBDIRS =
# CONDUITMODE_EXTRADIST =
# GASNET_TOOLSONLY_TRUE_END
# GASNET_TOOLSONLY_FALSE_BEGIN
CONDUITMODE_SUBDIRS = \
        ammpi      \
	amudp	   \
	contrib    \
	firehose   \
	testspawn
CONDUITMODE_EXTRADIST = \
	amxtests    \
	ssh-spawner \
	mpi-spawner \
	smp-collectives	\
	myxml	\
	detect-pshm.c \
  # 	exclude these from distribution for now
  # 	cross-configure-help.c
# GASNET_TOOLSONLY_FALSE_END
SUBDIRS = $(CONDUITMODE_SUBDIRS)

EXTRA_DIST =        			\
        $(CONDUITMODE_EXTRADIST) 	\
	detect-cachesz.c 		\
	tools-toggle.pl

include_HEADERS = \
	portable_inttypes.h \
	portable_platform.h

if CONDUIT_MODE
if USE_UDP_CONDUIT                           
  UDP_dir=$(top_builddir)/other/amudp
endif
if USE_AMMPI                           
  MPI_dir=$(top_builddir)/other/ammpi
endif
testdirs= $(UDP_dir) $(MPI_dir)

tests tests-clean run-tests check-exports: 
	@list='$(testdirs)'; for dir in $$list ; do \
	  ( cd "$$dir" && $(MAKE) $@ ) || exit $$?; \
	done
CONDUITMODE_TARGETS = Makefile-conduit.mak
endif

# If we add plpa to EXTRA_DIST, then automake-1.9 tries to package
# plpa/config/acinclude.m4 more than once.
dist-hook:
	rm -f $(distdir)/plpa/config/acinclude.m4
	cp -pR $(srcdir)/plpa $(distdir)/

all-local: $(CONDUITMODE_TARGETS) perlstart

# GASNET_TOOLSONLY_FALSE_BEGIN
## Docs to install from any subdirs
docdir = $(datadir)/doc/gasnet
install-data-local:
	$(INSTALL_DATA) $(srcdir)/ssh-spawner/README $(DESTDIR)$(docdir)/README-ssh-spawner
uninstall-local:
	rm -Rf $(DESTDIR)$(docdir)
# GASNET_TOOLSONLY_FALSE_END

.PHONY: force clean tests tests-clean run-tests

force:
	
