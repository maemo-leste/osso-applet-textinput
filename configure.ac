AC_INIT([Control Panel Text Input plugin], patsubst(esyscmd([dpkg-parsechangelog | sed -n '/^Version: \(.*\)$/ {s//\1/;p}']), [
]), [], [Control Panel Text Input plugin])
AM_CONFIG_HEADER(config.h)

AC_CONFIG_MACRO_DIR([m4])
AM_MAINTAINER_MODE
AM_INIT_AUTOMAKE([foreign])

AC_PROG_CC
AC_PROG_INSTALL
AM_PROG_LIBTOOL

AC_HEADER_STDC

AC_PATH_X
AC_PATH_XTRA
AC_SUBST(X_CFLAGS)

PKG_PROG_PKG_CONFIG

#+++++++++++++++
# Misc programs
#+++++++++++++++

AC_SUBST(CFLAGS)

PKG_CHECK_MODULES(ALL, [
			gtk+-2.0 dnl
			gconf-2.0 dnl
			dbus-1 dnl
			hildon-input-method-ui-3.0 dnl
			hildon-1 hildon-control-panel dnl
			])

AC_SUBST(ALL_LIBS)
AC_SUBST(ALL_CFLAGS)

hildoncpdesktopentrydir=`$PKG_CONFIG --variable=plugindesktopentrydir hildon-control-panel`
hildonpluginlibdir=`$PKG_CONFIG --variable=pluginlibdir hildon-control-panel`
himmenudir=`$PKG_CONFIG --variable=himmenudir hildon-input-method-ui-3.0`

AC_SUBST(hildoncpdesktopentrydir)
AC_SUBST(hildonpluginlibdir)
AC_SUBST(himmenudir)

#+++++++++++++++++++
# Directories setup
#+++++++++++++++++++

AC_OUTPUT([
Makefile
src/Makefile
])
