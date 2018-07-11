bin_PROGRAMS = tis

tis_CFLAGS = $(ALL_CFLAGS) -Wall -Werror
tis_LDADD = $(ALL_LIBS)
tis_SOURCES = tis.c cptextinput.c

hildonpluginlib_LTLIBRARIES = libcptextinput.la
libcptextinput_la_CFLAGS = $(ALL_CFLAGS) -Wall -Werror
libcptextinput_la_LDFLAGS = -shared -module -avoid-version -no-undefined
libcptextinput_la_LIBADD = $(ALL_LIBS)
libcptextinput_la_SOURCES = applet_main.c cptextinput.c
