# Makefile.am - main makefile for GnuPG
#     	Copyright (C) 2001, 2004, 2010 Free Software Foundation, Inc.
#
# This file is part of GnuPG.
#
# GnuPG is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# GnuPG is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, see <http://www.gnu.org/licenses/>.

## Process this file with automake to produce Makefile.in

ACLOCAL_AMFLAGS = -I m4 -I gl/m4
DISTCHECK_CONFIGURE_FLAGS = --enable-symcryptrun --enable-mailto --enable-gpgtar

GITLOG_TO_CHANGELOG=gitlog-to-changelog

EXTRA_DIST = build-aux/config.rpath build-aux/potomo autogen.sh autogen.rc \
	     ChangeLog-2011 po/ChangeLog-2011 build-aux/ChangeLog-2011 \
	     VERSION README.GIT build-aux/gitlog-to-changelog \
	     build-aux/git-log-fix build-aux/git-log-footer \
	     build-aux/getswdb.sh                           \
	     build-aux/speedo.mk                            \
             build-aux/speedo/zlib.pc			    \
             build-aux/speedo/w32			    \
             build-aux/speedo/w32/inst-options.ini	    \
             build-aux/speedo/w32/inst.nsi		    \
             build-aux/speedo/w32/pkg-copyright.txt	    \
             build-aux/speedo/w32/g4wihelp.c		    \
             build-aux/speedo/w32/pango.modules		    \
             build-aux/speedo/w32/gdk-pixbuf-loaders.cache  \
             build-aux/speedo/w32/exdll.h		    \
             build-aux/speedo/w32/README.txt		    \
             build-aux/speedo/patches			    \
             build-aux/speedo/patches/atk-1.32.0.patch	    \
             build-aux/speedo/patches/libiconv-1.14.patch   \
             build-aux/speedo/patches/pango-1.29.4.patch


DISTCLEANFILES = g10defs.h

if BUILD_GPGSM
kbx = kbx
else
kbx =
endif


if BUILD_GPG
gpg = g10
else
gpg =
endif
if BUILD_GPGSM
sm = sm
else
sm =
endif
if BUILD_AGENT
agent = agent
else
agent =
endif
if BUILD_SCDAEMON
scd = scd
else
scd =
endif
if BUILD_G13
g13 = g13
else
g13 =
endif
if BUILD_DIRMNGR
dirmngr = dirmngr
else
dirmngr =
endif
if BUILD_TOOLS
tools = tools
else
tools =
endif
if BUILD_DOC
doc = doc
else
doc =
endif

if RUN_TESTS
tests = tests
else
tests =
endif

SUBDIRS = m4 gl common ${kbx} \
          ${gpg} ${sm} ${agent} ${scd} ${g13} ${dirmngr} \
          ${tools} po ${doc} ${tests}

dist_doc_DATA = README


dist-hook: gen-ChangeLog

if HAVE_W32_SYSTEM
install-data-hook:
	set -e; \
	for i in $$($(top_srcdir)/build-aux/potomo \
                     --get-linguas $(top_srcdir)/po) ; do \
           $(MKDIR_P) "$(DESTDIR)$(localedir)/$$i/LC_MESSAGES" || true; \
           rm -f "$(DESTDIR)$(localedir)/$$i/LC_MESSAGES/gnupg2.mo" \
                                                 2>/dev/null || true; \
           $(top_srcdir)/build-aux/potomo $(top_srcdir)/po/$$i.po \
              "$(DESTDIR)$(localedir)/$$i/LC_MESSAGES/gnupg2.mo" ; \
	done
endif


gen_start_date = 2011-12-01T06:00:00
.PHONY: gen-ChangeLog
gen-ChangeLog:
	if test -d $(top_srcdir)/.git; then				\
	  (cd $(top_srcdir) &&                      			\
	    $(GITLOG_TO_CHANGELOG) --append-dot --tear-off		\
	    --amend=build-aux/git-log-fix 				\
	    --since=$(gen_start_date) ) > $(distdir)/cl-t;		\
          cat $(top_srcdir)/build-aux/git-log-footer >> $(distdir)/cl-t;  \
	  rm -f $(distdir)/ChangeLog;					\
	  mv $(distdir)/cl-t $(distdir)/ChangeLog;			\
	fi


stowinstall:
	$(MAKE) $(AM_MAKEFLAGS) install prefix=/usr/local/stow/gnupg
