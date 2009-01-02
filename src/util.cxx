/*-------------------------------------------------------------------------
 *
 *   FILE
 *	util.cxx
 *
 *   DESCRIPTION
 *      Various utility functions for libpqxx
 *
 * Copyright (c) 2003-2009, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#include "pqxx/compiler-internal.hxx"

#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <new>

#ifdef PQXX_HAVE_SYS_SELECT_H
#include <sys/select.h>
#else
#include <sys/types.h>
#if defined(_WIN32)
#include <winsock2.h>
#endif
#endif // PQXX_HAVE_SYS_SELECT_H

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef _WIN32
#include <windows.h>
#endif

#include "libpq-fe.h"

#include "pqxx/except"
#include "pqxx/util"


using namespace PGSTD;
using namespace pqxx::internal;


const char pqxx::internal::sql_begin_work[] = "BEGIN",
      pqxx::internal::sql_commit_work[] = "COMMIT",
      pqxx::internal::sql_rollback_work[] = "ROLLBACK";


void pqxx::internal::freemem_notif(const pqxx::internal::pq::PGnotify *p)
  throw ()
{
#ifdef PQXX_HAVE_PQFREENOTIFY
  PQfreeNotify(const_cast<pqxx::internal::pq::PGnotify *>(p));
#else
  freepqmem(p);
#endif
}


#ifndef PQXX_HAVE_SHARED_PTR
pqxx::internal::refcount::refcount() :
  m_l(this),
  m_r(this)
{
}


pqxx::internal::refcount::~refcount()
{
  loseref();
}


void pqxx::internal::refcount::makeref(refcount &rhs) throw ()
{
  // TODO: Make threadsafe
  m_l = &rhs;
  m_r = rhs.m_r;
  m_l->m_r = m_r->m_l = this;
}


bool pqxx::internal::refcount::loseref() throw ()
{
  // TODO: Make threadsafe
  const bool result = (m_l == this);
  m_r->m_l = m_l;
  m_l->m_r = m_r;
  m_l = m_r = this;
  return result;
}
#endif // PQXX_HAVE_SHARED_PTR


string pqxx::internal::namedclass::description() const
{
  try
  {
    string desc = classname();
    if (!name().empty()) desc += " '" + name() + "'";
    return desc;
  }
  catch (const exception &)
  {
    // Oops, string composition failed!  Probably out of memory.
    // Let's try something easier.
  }
  return name().empty() ? classname() : name();
}


void pqxx::internal::CheckUniqueRegistration(const namedclass *New,
    const namedclass *Old)
{
  if (!New)
    throw internal_error("NULL pointer registered");
  if (Old)
  {
    if (Old == New)
      throw usage_error("Started twice: " + New->description());
    throw usage_error("Started " + New->description() + " "
		      "while " + Old->description() + " still active");
  }
}


void pqxx::internal::CheckUniqueUnregistration(const namedclass *New,
    const namedclass *Old)
{
  if (New != Old)
  {
    if (!New)
      throw usage_error("Expected to close " + Old->description() + ", "
			"but got NULL pointer instead");
    if (!Old)
      throw usage_error("Closed while not open: " + New->description());
    throw usage_error("Closed " + New->description() + "; "
		      "expected to close " + Old->description());
  }
}


void pqxx::internal::freepqmem(const void *p)
{
#ifdef PQXX_HAVE_PQFREEMEM
  PQfreemem(const_cast<void *>(p));
#else
  free(const_cast<void *>(p));
#endif
}


void pqxx::internal::sleep_seconds(int s)
{
  if (s <= 0) return;

#if defined(PQXX_HAVE_SLEEP)
  // Use POSIX.1 sleep() if available
  sleep(s);
#elif defined(_WIN32)
  // Windows has its own Sleep(), which speaks milliseconds
  Sleep(s*1000);
#else
  // If all else fails, use select() on nothing and specify a timeout
  fd_set F;
  FD_ZERO(&F);
  struct timeval timeout;
  timeout.tv_sec = s;
  timeout.tv_usec = 0;
  if (select(0, &F, &F, &F, &timeout) == -1) switch (errno)
  {
  case EINVAL:	// Invalid timeout
	throw range_error("Invalid timeout value: " + to_string(s));
  case EINTR:	// Interrupted by signal
	break;
  case ENOMEM:	// Out of memory
	throw bad_alloc();
  default:
    throw internal_error("select() failed for unknown reason");
  }
#endif
}


namespace
{

void cpymsg(char buf[], const char input[], size_t buflen) throw ()
{
#if defined(PQXX_HAVE_STRLCPY)
  strlcpy(buf, input, buflen);
#else
  strncpy(buf, input, buflen);
  if (buflen) buf[buflen-1] = '\0';
#endif
}

// Single Unix Specification version of strerror_r returns result code
const char *strerror_r_result(int sus_return, char buf[], size_t len) throw ()
{
  switch (sus_return)
  {
  case 0: break;
  case -1: cpymsg(buf, "Unknown error", len); break;
  default:
    cpymsg(buf,
	  "Unexpected result from strerror_r()!  Is it really SUS-compliant?",
	  len);
    break;
  }

  return buf;
}

// GNU version of strerror_r returns error string (which may be anywhere)
const char *strerror_r_result(const char gnu_return[], char[], size_t) throw ()
{
  return gnu_return;
}
}


const char *pqxx::internal::strerror_wrapper(int err, char buf[], size_t len)
	throw ()
{
  if (!buf || len <= 0) return "No buffer provided for error message!";

  const char *res = buf;

#if !defined(PQXX_HAVE_STRERROR_R)
  cpymsg(buf, strerror(err), len);
#else
  // This will pick the appropriate strerror_r() subwrapper using overloading
  // (an idea first suggested by Bart Samwel.  Thanks a bundle, Bart!)
  res = strerror_r_result(strerror_r(err,buf,len), buf, len);
#endif
  return res;
}

