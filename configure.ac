#############################################################################
#
#	WhiteBoard SIB access component
#
#	Copyright (c) 2007 Nokia Corporation
#
#############################################################################

AC_INIT()
AC_CONFIG_AUX_DIR(config)

AM_INIT_AUTOMAKE(whiteboard-sib-access-nota, 0.4)
AC_CONFIG_HEADERS(config.h)
AC_CONFIG_MACRO_DIR([m4])


#############################################################################
# Constants
#############################################################################

#############################################################################
# Checks for some common programs needed for configure/compile/install
#############################################################################
AM_PROG_CC_C_O
AC_PROG_INSTALL
AC_PROG_AWK
AC_PROG_LIBTOOL
#AC_PROG_RANLIB

#############################################################################
# Check whether debug prints should be used
#############################################################################
AC_ARG_WITH(debug, 	AS_HELP_STRING([--with-debug],
		       [Print additional debug messages (default = no)]),
	[AC_DEFINE([WHITEBOARD_DEBUG],[1],[Print debug messages])],
	[with_debug=no])

AC_ARG_WITH(hin-sp, 	AS_HELP_STRING([--with-hin-sp],
		       [Link to single process H_IN (default = no)]),
	[with_hin_sp=yes],
	[with_hin_sp=no])

AC_ARG_ENABLE(rm,
	AS_HELP_STRING([--enable-rm],
			[enable NoTa ResourceManager service discovery (default = yes)]),
	[
		if test $enableval = no; then
			USE_RM=no
		else
			USE_RM=yes
			AC_DEFINE([USE_RM],[1],[Use NoTA RM])
		fi
	],[
		USE_RM=yes
		AC_DEFINE([USE_RM],[1],[Use NoTA RM])
	]
)
AM_CONDITIONAL(RM, test $USE_RM = yes)



#############################################################################
# Check whether control point tracing should be used
#############################################################################
AC_ARG_WITH(cp-lock-trace, 
	AS_HELP_STRING([--with-cp-lock-trace],
		       [Trace cp locking and fail fast on errors, errors can be found also from /tmp/zenit* (default = no)]),
	[AC_DEFINE([WITH_THREAD_LOCK_TRACE],[1],[Trace cp locking])],
	[with_cp_lock_trace=no])


#############################################################################
# Check for some header files (TODO: Add more?)
#############################################################################
AC_HEADER_STDC
AC_CHECK_HEADERS([limits.h stdlib.h string.h unistd.h])

##############################################################################
# Check for iconv
##############################################################################
#AC_CHECK_FUNCS([iconv])
#AC_CHECK_HEADERS([iconv.h])
#AC_CHECK_LIB([iconv],[main])

##############################################################################
# Check for pthread presence
##############################################################################
AC_CHECK_HEADERS([pthread.h],
		 [],
		 [AC_MSG_ERROR(WhiteBoard node needs pthread library)])
AC_CHECK_LIB([pthread],[main])

##############################################################################
# Check for CyberLink C & AV extension
##############################################################################
#[
#	clinkc >= 1.0-21
#	clinkcav >= 1.0-14
#])

#AC_SUBST(CLINKC_CFLAGS)
#AC_SUBST(CLINKC_LIBS)

##############################################################################
# Check for GNOME environment
##############################################################################
PKG_CHECK_MODULES(GNOME,
[
	glib-2.0 >= 2.8.6
	gobject-2.0 >= 2.8.6
	gthread-2.0 >= 2.8.6
	dbus-1 >= 0.61
	dbus-glib-1 >= 0.61
])

AC_SUBST(GNOME_CFLAGS)
AC_SUBST(GNOME_LIBS)


##############################################################################
# Check for NOTA environment
##############################################################################
if test $with_hin_sp = yes; then
   PKG_CHECK_MODULES(NOTA,
   [
	nota-h_in-sp-3.0,
	nota-stubgen-sp-3.0
   ])
else
   PKG_CHECK_MODULES(NOTA,
   [
	nota-h_in-3.0,
	nota-stubgen-3.0
   ])
fi
AC_SUBST(NOTA_CFLAGS)
AC_SUBST(NOTA_LIBS)


if test $USE_RM = yes; then
nota_incs=`pkg-config --cflags nota-stubgen-3.0 | tr -d ' ' | tr -d '-' | tr -d 'I'`
#AC_CHECK_HEADERS([${nota_incs}stubgen/resourcemng.h],
#		 [],
#		 [AC_MSG_ERROR( Required header stubgen/resourcemng.h not found #install it first.)] )
fi
##############################################################################
# Check for OSSO/Hildon environment
##############################################################################
#case $host in
#	arm-*)
#PKG_CHECK_MODULES(MAEMO, 
#[
#	libosso >= 0.21
#])

#AC_SUBST(MAEMO_CFLAGS)
#AC_SUBST(MAEMO_LIBS)
#	;;
#esac


#PKG_CHECK_MODULES(CONIC, 
#[
#	conic >= 0.13
#])

#AC_SUBST(CONIC_CFLAGS)
#AC_SUBST(CONIC_LIBS)

##############################################################################
# Check for Media Hub library
##############################################################################
PKG_CHECK_MODULES(LIBWHITEBOARD,
[
	libwhiteboard = 2.0-beta1
	libssap_parse_n_gen = 2.0-beta1
])

AC_SUBST(LIBWHITEBOARD_CFLAGS)
AC_SUBST(LIBWHITEBOARD_LIBS)

##############################################################################
# Checks for typedefs, structures, and compiler characteristics.
##############################################################################

AC_C_CONST
AC_C_INLINE
AC_STRUCT_TM

##############################################################################
# Locale stuff
##############################################################################

AC_DEFINE_UNQUOTED([LOCALEDIR], 
		   "${runtime_localedir}", [Runtime locale catalog files path])

##############################################################################
# Check compile target to be able to disable local player in emulator
##############################################################################

case $host in
        i386-* | i686-*)
               CFLAGS="$CFLAGS -Di686"
	       USE_i686=yes
	       USE_ARM=no
               ;;
        arm-*)
               CFLAGS="$CFLAGS -DARM"
	       USE_ARM=yes
	       USE_i686=no
               ;;
esac

AM_CONDITIONAL(ARM, test "x$USE_ARM"="xyes")
AM_CONDITIONAL(i686, test "x$USE_i686"="xyes")

##############################################################################
# Files to create
##############################################################################

AC_OUTPUT(
	Makefile \
	include/Makefile \
	src/Makefile \
)
echo "Debug logs: " ${with_debug}
echo "RM: " ${USE_RM}
echo "With single process H_IN: "${with_hin_sp}