#include "config.h"

#include "test/rpc/test_trust_scope.h"

#include <torrent/exceptions.h>

#include "rpc/rpc_manager.h"

CPPUNIT_TEST_SUITE_REGISTRATION(TestTrustScope);

void
TestTrustScope::test_basics() {
  CPPUNIT_ASSERT(rpc::rpc.is_trusted());

  {
    rpc::trust_scope scope(false);

    CPPUNIT_ASSERT(!rpc::rpc.is_trusted());
  }

  CPPUNIT_ASSERT(rpc::rpc.is_trusted());
}

void
TestTrustScope::test_nested() {
  rpc::trust_scope outer(false);

  CPPUNIT_ASSERT(!rpc::rpc.is_trusted());

  {
    rpc::trust_scope inner(true);

    CPPUNIT_ASSERT(rpc::rpc.is_trusted());
  }

  CPPUNIT_ASSERT(!rpc::rpc.is_trusted());
}

void
TestTrustScope::test_exception() {
  try {
    rpc::trust_scope scope(false);

    throw torrent::input_error("test");
  } catch (torrent::input_error&) {
  }

  CPPUNIT_ASSERT(rpc::rpc.is_trusted());
}
