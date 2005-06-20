/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pqxx/compiler.h
 *
 *   DESCRIPTION
 *      Compiler deficiency workarounds for compiling libpqxx itself.
 *      DO NOT INCLUDE THIS FILE when building client programs.
 *
 * Copyright (c) 2002-2005, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#ifndef PQXX_COMPILER_H
#define PQXX_COMPILER_H


// Workarounds & definitions needed to compile libpqxx into a library
#include "pqxx/config-internal-compiler.h"

// Library-private configuration related to libpq version
#include "pqxx/config-internal-libpq.h"

// Macros generated by autoconf/automake/libtool/...
#include "pqxx/config-internal-autotools.h"

#ifdef _WIN32
#ifdef LIBPQXXDLL_EXPORTS
#undef  PQXX_LIBEXPORT
#define PQXX_LIBEXPORT __declspec(dllexport)
#endif	// LIBPQXXDLL_EXPORTS
#elif defined(__GNUC__) && defined(PQXX_HAVE_GCC_VISIBILITY)	// !_WIN32
#define PQXX_LIBEXPORT __attribute__ ((visibility("default")))
#endif	// __GNUC__ && PQXX_HAVE_GCC_VISIBILITY


#include "pqxx/libcompiler.h"

#ifdef PQXX_HAVE_LIMITS
#include <limits>
#else // PQXX_HAVE_LIMITS
#include <climits>
namespace PGSTD
{
/// Work around lacking "limits" header
template<typename T> struct numeric_limits
{
  static T max() throw ();
  static T min() throw ();
};
template<> inline long numeric_limits<long>::max() throw () {return LONG_MAX;}
template<> inline long numeric_limits<long>::min() throw () {return LONG_MIN;}
}
#endif // PQXX_HAVE_LIMITS

#endif

