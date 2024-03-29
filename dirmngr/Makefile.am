# Makefile.am - dirmngr
# Copyright (C) 2002 Klarälvdalens Datakonsult AB
# Copyright (C) 2004, 2007, 2010 g10 Code GmbH
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

EXTRA_DIST = OAUTHORS ONEWS ChangeLog.1 ChangeLog-2011

bin_PROGRAMS = dirmngr dirmngr-client

if USE_LDAPWRAPPER
libexec_PROGRAMS = dirmngr_ldap
endif

AM_CPPFLAGS = -I$(top_srcdir)/gl -I$(top_srcdir)/intl -I$(top_srcdir)/common

include $(top_srcdir)/am/cmacros.am

AM_CFLAGS = $(LIBGCRYPT_CFLAGS) $(KSBA_CFLAGS) \
            $(LIBASSUAN_CFLAGS) $(GPG_ERROR_CFLAGS) $(NPTH_CFLAGS)

BUILT_SOURCES = no-libgcrypt.c

CLEANFILES = no-libgcrypt.c

if HAVE_W32_SYSTEM
ldap_url = ldap-url.h ldap-url.c
else
ldap_url =
endif

noinst_HEADERS = dirmngr.h crlcache.h crlfetch.h misc.h

dirmngr_SOURCES = dirmngr.c dirmngr.h server.c crlcache.c crlfetch.c	\
	ldapserver.h ldapserver.c certcache.c certcache.h \
	cdb.h cdblib.c ldap.c misc.c dirmngr-err.h w32-ldap-help.h \
	ocsp.c ocsp.h validate.c validate.h ldap-wrapper.h $(ldap_url) \
	ks-action.c ks-action.h ks-engine.h \
        ks-engine-hkp.c ks-engine-http.c ks-engine-finger.c ks-engine-kdns.c

if USE_LDAPWRAPPER
dirmngr_SOURCES += ldap-wrapper.c
else
dirmngr_SOURCES += ldap-wrapper-ce.c  dirmngr_ldap.c
endif


dirmngr_LDADD = $(libcommontlsnpth) $(libcommonpth) \
        ../gl/libgnu.a $(DNSLIBS) $(LIBASSUAN_LIBS) \
	$(LIBGCRYPT_LIBS) $(KSBA_LIBS) $(NPTH_LIBS) \
	$(NTBTLS_LIBS) $(LIBGNUTLS_LIBS) $(LIBINTL) $(LIBICONV)
if !USE_LDAPWRAPPER
dirmngr_LDADD += $(LDAPLIBS)
endif
dirmngr_LDFLAGS = $(extra_bin_ldflags)

if USE_LDAPWRAPPER
dirmngr_ldap_SOURCES = dirmngr_ldap.c $(ldap_url)
dirmngr_ldap_CFLAGS = $(GPG_ERROR_CFLAGS) $(LIBGCRYPT_CFLAGS)
dirmngr_ldap_LDFLAGS =
dirmngr_ldap_LDADD = $(libcommon) no-libgcrypt.o ../gl/libgnu.a \
		     $(GPG_ERROR_LIBS) $(LDAPLIBS) $(LBER_LIBS) $(LIBINTL) \
		     $(LIBICONV)
endif

dirmngr_client_SOURCES = dirmngr-client.c
dirmngr_client_LDADD = $(libcommon) no-libgcrypt.o \
	                ../gl/libgnu.a $(LIBASSUAN_LIBS) \
	               $(GPG_ERROR_LIBS) $(NETLIBS) $(LIBINTL) $(LIBICONV)
dirmngr_client_LDFLAGS = $(extra_bin_ldflags)


no-libgcrypt.c : $(top_srcdir)/tools/no-libgcrypt.c
	cat $(top_srcdir)/tools/no-libgcrypt.c > no-libgcrypt.c


$(PROGRAMS) : $(libcommon) $(libcommonpth) $(libcommontls) $(libcommontlsnpth)
