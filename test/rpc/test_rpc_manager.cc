#include "config.h"

#include "test/rpc/test_rpc_manager.h"

#include <torrent/exceptions.h>

#include "rpc/scgi_task.h"

CPPUNIT_TEST_SUITE_REGISTRATION(TestRpcManager);

// A size limit too small to hold any request rejects every request, including
// the one that would put it back, so it can only be undone by a restart.
void
TestRpcManager::test_size_limit_bounds() {
  CPPUNIT_ASSERT_THROW(m_rpc_manager.set_size_limit(0), torrent::input_error);
  CPPUNIT_ASSERT_THROW(m_rpc_manager.set_size_limit(rpc::RpcManager::min_size_limit - 1), torrent::input_error);
  CPPUNIT_ASSERT_THROW(m_rpc_manager.set_size_limit(rpc::SCgiTask::max_content_size + 1), torrent::input_error);

  CPPUNIT_ASSERT_NO_THROW(m_rpc_manager.set_size_limit(rpc::RpcManager::min_size_limit));
  CPPUNIT_ASSERT_NO_THROW(m_rpc_manager.set_size_limit(rpc::SCgiTask::max_content_size));
}
