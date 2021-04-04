# ===========================================================================
#       https://www.gnu.org/software/autoconf-archive/ax_execinfo.html
# ===========================================================================
#
# SYNOPSIS
#
#   AX_EXECINFO([ACTION-IF-EXECINFO-H-IS-FOUND], [ACTION-IF-EXECINFO-H-IS-NOT-FOUND], [ADDITIONAL-TYPES-LIST])
#
# DESCRIPTION
#
#   Checks for execinfo.h header and if the len parameter/return type can be
#   found from a list, also define backtrace_size_t to that type.
#
#   By default the list of types to try contains int and size_t, but should
#   some yet undiscovered system use e.g. unsigned, the 3rd argument can be
#   used for extensions. I'd like to hear of further suggestions.
#
#   Executes ACTION-IF-EXECINFO-H-IS-FOUND when present and the execinfo.h
#   header is found or ACTION-IF-EXECINFO-H-IS-NOT-FOUND in case the header
#   seems unavailable.
#
#   Also adds -lexecinfo to LIBS on BSD if needed.
#
# LICENSE
#
#   Copyright (c) 2014 Thomas Jahns <jahns@dkrz.de>
#
#   Copying and distribution of this file, with or without modification, are
#   permitted in any medium without royalty provided the copyright notice
#   and this notice are preserved. This file is offered as-is, without any
#   warranty.

#serial 2

AC_DEFUN([AX_EXECINFO],
  [AC_CHECK_HEADERS([execinfo.h])
   AS_IF([test x"$ac_cv_header_execinfo_h" = xyes],
     [AC_CACHE_CHECK([size parameter type for backtrace()],
	[ax_cv_proto_backtrace_type],
	[AC_LANG_PUSH([C])
	 for ax_cv_proto_backtrace_type in size_t int m4_ifnblank([$3],[$3 ])none; do
	   AS_IF([test "${ax_cv_proto_backtrace_type}" = none],
	     [ax_cv_proto_backtrace_type= ; break])
	   AC_COMPILE_IFELSE([AC_LANG_PROGRAM([
#include <execinfo.h>
extern
${ax_cv_proto_backtrace_type} backtrace(void **addrlist, ${ax_cv_proto_backtrace_type} len);
char **backtrace_symbols(void *const *buffer, ${ax_cv_proto_backtrace_type} size);
])],
	   [break])
	 done
	 AC_LANG_POP([C])])])
   AS_IF([test x${ax_cv_proto_backtrace_type} != x],
     [AC_DEFINE_UNQUOTED([backtrace_size_t], [$ax_cv_proto_backtrace_type],
        [Defined to return type of backtrace().])])
   AC_SEARCH_LIBS([backtrace],[execinfo])
   AS_IF([test x"${ax_cv_proto_backtrace_type}" != x -a x"$ac_cv_header_execinfo_h" = xyes -a x"$ac_cv_search_backtrace" != xno],
     [AC_DEFINE([HAVE_BACKTRACE],[1],
        [Defined if backtrace() could be fully identified.])
     ]m4_ifnblank([$1],[$1
]),m4_ifnblank([$2],[$2
]))])
dnl
dnl Local Variables:
dnl mode: autoconf
dnl End:
dnl
