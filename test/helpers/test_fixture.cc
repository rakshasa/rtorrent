#include "config.h"

#include "test_fixture.h"

#include "torrent/utils/log.h"

#include <iostream>

void
test_fixture::setUp() {
  mock_init();

  log_add_group_output(torrent::LOG_CONNECTION_BIND, "test_output");
  log_add_group_output(torrent::LOG_CONNECTION_FD, "test_output");
}

void
test_fixture::tearDown() {
  mock_cleanup();
}
