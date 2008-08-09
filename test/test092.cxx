#include <cassert>
#include <cstring>
#include <iostream>
#include <list>

#include <pqxx/pqxx>

#include "test_helpers.hxx"


using namespace PGSTD;
using namespace pqxx;

// Test program for libpqxx.  Test binary parameters to prepared statements.

namespace
{
void test_092(connection_base &C, transaction_base &T)
{
  const char databuf[] = "Test\0data";
  const string data(databuf, sizeof(databuf));
  assert(data.size() > strlen(databuf));

  const string Table = "pqxxbin", Field = "binfield", Stat = "nully";
  T.exec("CREATE TEMP TABLE " + Table + " (" + Field + " BYTEA)");

  if (!C.supports(connection_base::cap_prepared_statements))
  {
    cout << "Backend version does not support prepared statements.  Skipping."
         << endl;
    return;
  }

  C.prepare(Stat, "INSERT INTO " + Table + " VALUES ($1)")
	("BYTEA", pqxx::prepare::treat_binary);
  T.prepared(Stat)(data).exec();

  const result L( T.exec("SELECT length("+Field+") FROM " + Table) );
  PQXX_CHECK_EQUAL(L[0][0].as<size_t>(),
	data.size(),
	"Length of field in database does not match original length.")

  const result R( T.exec("SELECT " + Field + " FROM " + Table) );

  const binarystring roundtrip(R[0][0]);

  PQXX_CHECK_EQUAL(string(roundtrip.str()), data, "Data came back different.")

  PQXX_CHECK_EQUAL(roundtrip.size(),
	data.size(),
	"Binary string reports wrong size.")

  // People seem to like the multi-line invocation style, where you get your
  // invocation object first, then add parameters in separate C++ statements.
  // As John Mudd found, that used to break the code.  Let's test it.
  T.exec("CREATE TEMP TABLE tuple (one INTEGER, two VARCHAR)");

  pqxx::prepare::declaration d(
	C.prepare("maketuple", "INSERT INTO tuple VALUES ($1, $2)") );
  d("INTEGER", pqxx::prepare::treat_direct);
  d("VARCHAR", pqxx::prepare::treat_string);

  pqxx::prepare::invocation i( T.prepared("maketuple") );
  const string f = "frobnalicious";
  i(6);
  i(f);
  i.exec();

  const result t( T.exec("SELECT * FROM tuple") );
  if (t.size() != 1)
    throw logic_error("Expected 1 tuple, got " + to_string(t.size()));
  if (t[0][0].as<string>() != "6")
    throw logic_error("Expected value 6, got " + t[0][0].as<string>());
  if (t[0][1].c_str() != f)
    throw logic_error("Expected string '" + f + "', "
	"got '" + t[0][1].c_str() + "'");
}

} // namespace


int main()
{
  test::TestCase<lazyconnection> test092("test_092", test_092);
  return test::pqxxtest(test092);
}
